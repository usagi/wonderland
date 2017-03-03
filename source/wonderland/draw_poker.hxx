#include "cards_type.hxx"
#include "player_type.hxx"

#include <cmdline.h>
#include <boost/optional.hpp>
#include <boost/timer/timer.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/range/numeric.hpp>
#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/random/random_device.hpp>

#include <deque>
#include <random>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <sstream>
#include <type_traits>

namespace usagi::poker
{
  using namespace std;
  
  class draw_poker_type
  {
    public:
      using shared_player_type = std::shared_ptr< player_type >;
      
    private:
      bool is_initialized;
      size_t play_count_limit = 1000;
      size_t play_count       =    0;
      
      /// @brief ポーカーの掛け金置き場
      size_t pot;
      
      size_t ante_min =  1;
      size_t ante_max = 10;
      size_t ante_max_extended = ante_max;
      
      size_t player_initial_chips = 100;
      
      boost::timer::cpu_timer timer;
      
      bool is_output_logging = true;
      
      ostream* log_target = &cout;
      
      struct player_data_type
      {
        size_t chips = 0;
        bool joined = false;
        cards_type cards;
        
        size_t count_of_royal_straight_flush = 0;
        size_t count_of_straight_flush = 0;
        size_t count_of_four_of_a_kind = 0;
        size_t count_of_full_house = 0;
        size_t count_of_flush = 0;
        size_t count_of_straight = 0;
        size_t count_of_three_of_a_kind = 0;
        size_t count_of_two_pair = 0;
        size_t count_of_one_pair = 0;
        size_t count_of_high_cards = 0;
        
        auto reset( size_t chips_ = 0 )
        {
          chips = chips_;
          joined = false;
          cards.clear();
          
          count_of_royal_straight_flush = 0;
          count_of_straight_flush = 0;
          count_of_four_of_a_kind = 0;
          count_of_full_house = 0;
          count_of_flush = 0;
          count_of_straight = 0;
          count_of_three_of_a_kind = 0;
          count_of_two_pair = 0;
          count_of_one_pair = 0;
          count_of_high_cards = 0;
        }
      };
      
      boost::optional
      < pair
        < shared_ptr< player_type >
        , player_data_type
        >
      > player_mapper;
      
      /// @brief ポーカーの山札
      deque< card_type > pile;
      
      auto log( const std::string& in )
      {
        if ( is_output_logging )
          (*log_target) << "[ " << timer.elapsed().wall << " ] " << in << endl;
      }
      
      auto shuffle_pile()
      {
        log( "[system] shuffle pile begin" );
        // std::random_device は実装により（具体的には mingw ）全く当てにならない場合があり乱数として扱うと危険
        //random_device rd;
        boost::random::random_device rd;
        mt19937_64 rng( rd() );
        shuffle( pile.begin(), pile.end(), rng );
        log( "[system] shuffle pile end" );
      }
      
      auto initialize_pile()
      {
        log( "[system] initialize pile begin" );
        
        pile.clear();
        
        for ( const auto suit : { suit_type::club, suit_type::diamond, suit_type::heart, suit_type::spade } )
          for ( const auto number : { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 } )
            pile.emplace_back( suit, number );
        
        log( "[system] initialize pile end" );
      }
      
      auto initialize_player()
      {
        log( "[system] initialize player begin" );
        
        if ( not player_mapper )
          throw runtime_error( "player is not set" );
        
        player_mapper->second.reset( player_initial_chips );
        
        log( "[system] initialize player end" );
      }
      
      auto initialize()
      {
        log( "[system] initialize begin");
        
        pot = 0;
        play_count = 0;
        initialize_player();
        
        is_initialized = true;
        
        log( "[system] initialize end");
      }
      
      auto take_cards( cards_type& cards )
      {
        log( "[system] take cards begin");
        
        auto number_of_draw = max( min( 5 - cards.size(), static_cast< size_t >( 5 ) ), static_cast< size_t >( 0 ) );
        
        log( "[system] number of draw: "s + to_string( number_of_draw ) );
        
        while ( number_of_draw-- )
        {
          if ( pile.empty() )
            throw runtime_error( "pile is empty" );
          
          auto card = move( pile.front() );
          {
            stringstream s;
            s << "[system] drawed card: " << card;
            log( s.str() );
          }
          pile.pop_front();
          log( "[system] pop pile ( pile size: "s + to_string( pile.size() ) + " cards )" );
          cards.emplace_back( move( card ) );
          log( "[system] push the card to cards" );
        }
        
        log( "[system] take cards end");
      }
      
      auto discard_cards( cards_type& hands, const cards_type& discards )
      {
        boost::remove_erase_if
        ( hands
        , [&] ( const auto& card ) { return boost::algorithm::any_of_equal( discards, card ); }
        );
      }
      
      /// @TODO 後ほど実装を整理する
      auto play()
      {
        log( "[system] play begin");
        
        if ( not is_initialized )
          throw runtime_error( "the poker game is not initialized" );
        
        ++play_count;
        
        initialize_pile();
        shuffle_pile();
        
        player_mapper->second.cards.clear();
        
        // ante（アンティ：ポーカーゲームの参加費処理）
        {
          // プレイヤーに現在のチップ数を通知し、今回のゲームにアンティを支払い参加するか確認する。
          log( "[game] current ante: minimum = " + to_string( ante_min ) + " chips, maximum = " + to_string( ante_max_extended ) + " chips" );
          const auto ante = 
            min
            ( player_mapper->first->pay_ante( player_mapper->second.chips, ante_min, ante_max_extended )
            , player_mapper->second.chips
            );
          
          // アンティが支払われる場合にはポーカーゲームへプレイヤーを参加させる
          if ( ante )
          {
            // プレイヤーからチップの支払いを受ける
            player_mapper->second.chips = player_mapper->second.chips - ante;
            // プレイヤーから支払われたチップをポットへ入れる
            pot = pot + ante;
            // プレイヤーをポーカーゲームへ参加させる
            player_mapper->second.joined = true;
            log( "[game] the player is joined with pay ante( "s + to_string( ante ) + " chips )" );
          }
          else
            log( "[game] the player is dropped the game without pay ante" );
          log( "[game] the player has "s + to_string( player_mapper->second.chips ) + " chips" );
        }
        
        // player turn
        
        if ( player_mapper->second.joined )
        {
          log( "[game] in the player turn" );
          
          const auto& to_string =
            []( const auto& cards )
            {
              stringstream buffer;
              for ( const auto& card : cards )
                buffer << card;
              return buffer.str();
            };
          
          // プレイヤーにパイルからカードを所定の枚数まで引かせる
          take_cards( player_mapper->second.cards );
          log( "[game] the player has cards: " + to_string( player_mapper->second.cards ) );
          
          // プレイヤーに捨てるカードを選択させる
          const auto discards = player_mapper->first->discard_cards( player_mapper->second.cards );
          log( "[game] the player discard cards: " + to_string( discards ) );
          
          // プレイヤーのカードを捨て、パイルからカードを所定の枚数まで引かせる
          discard_cards( player_mapper->second.cards, discards );
          take_cards( player_mapper->second.cards );
          log( "[game] the player result: " + to_string( player_mapper->second.cards ) );
          
          // あがり役によってポットへゲームシステムから払い出しを行う
          {
            log ( "[game] calculate hands" );
            const auto& cards = player_mapper->second.cards;
            
            // 数字だけの役の判定用に数字だけ取り出してソートした中間処理オブジェクトを用意しておく。
            vector< card_type::number_type > numbers;
            transform( cards.cbegin(), cards.cend(), back_inserter( numbers ), []( const auto& c ){ return c.get_number(); } );
            sort( numbers.begin(), numbers.end() );
            {
              stringstream t;
              t << "[game] sorted numbers in player's cards: ";
              for ( const auto n : numbers )
                t << std::to_string( n ) << ' ';
              log( t.str() );
            }
            
            // flush
            auto is_flush = all_of( cards.cbegin() + 1, cards.cend(), [ suit = cards.front().get_suit() ]( const auto& c ){ return c.get_suit() == suit; } );
            
            // straight
            auto is_straight =
              // 5つの連続した数値のパターン
                  (   numbers.at( 0 ) == numbers.at( 1 ) - 1
                  and numbers.at( 1 ) == numbers.at( 2 ) - 1
                  and numbers.at( 2 ) == numbers.at( 3 ) - 1
                  and numbers.at( 3 ) == numbers.at( 4 ) - 1
                  )
              // A,10,J,Q,K パターン
              or  (   numbers.at( 0 ) == 1
                  and numbers.at( 1 ) == 10
                  and numbers.at( 2 ) == 11
                  and numbers.at( 3 ) == 12
                  and numbers.at( 4 ) == 13
                  )
              ;
            
            // 隣接する数値の位置の差を求める。余計なオブジェクトを残さないためクロージャー化。
            const auto diff_adjacencies = 
              [&]
              {
                // numbers について隣接する数値の位置を検出する
                vector< size_t > adjacencies;
                {
                  auto i = adjacent_find( numbers.cbegin(), numbers.cend() );
                  while ( i != numbers.cend() )
                  {
                    adjacencies.emplace_back( distance( numbers.cbegin(), i ) );
                    i = adjacent_find( i + 1, numbers.cend() );
                  }
                }
                
                // 連続した数値が存在しない空っぽの場合、空っぽの数列をそのまま帰す。
                if ( adjacencies.empty() )
                  return adjacencies;
                
                // 隣接する数値の位置の差を求める
                vector< size_t > diff_adjacencies( adjacencies.size() );
                adjacent_difference( adjacencies.cbegin(), adjacencies.cend(), diff_adjacencies.begin() );
                
                // 先頭要素は diff にならず元の数列の先頭要素がそのまま入っているため、便宜上1区間の連続の扱いとするため1に変更しておく。
                if ( not diff_adjacencies.empty() )
                  diff_adjacencies[ 0 ] = 1;
                
                return diff_adjacencies;
              }();
            
            auto count_of_pairs     = 0;
            auto is_three_of_a_kind = false;
            auto is_four_of_a_kind  = false;
            
            switch ( diff_adjacencies.size() )
            {
              case 1:
                count_of_pairs = 1;
              case 0:
                break;
              default:
              {
                size_t sequence_length = 0;
                
                const auto& fix_sequence =
                  [&]
                  {
                    switch ( sequence_length )
                    {
                      case 1:
                        ++count_of_pairs;
                        // Pair 発生時には他にも Pair あるいは Three of Kind が発生する可能性があるので switch のみ抜ける
                        break;
                      case 2:
                        is_three_of_a_kind = true;
                        // Three of a Kind 発生時には他に Pair が1組存在（ ＝ Full House ）する可能性があるので switch のみ抜ける
                        break;
                      case 3:
                        is_four_of_a_kind = true;
                        // Four of a Kind が発生している場合には他の同数値による役は存在し得ないので処理を抜ける
                        return;
                      default:
                        // Pair, Three of a Kind, Four of a Kind 以外の同数値の連続が検出される事があれば何かが不正な状態になっている
                        throw runtime_error( "detect a strange sequence in calculation of hand" );
                    }
                  };
                
                for ( const auto diff : diff_adjacencies )
                {
                  switch ( diff )
                  {
                    // 隣接した数値の並びが+1つ確認された場合
                    case 1:
                      ++sequence_length;
                      break;
                    // 隣接した数値の並びが一旦途切れた場合
                    default:
                      fix_sequence();
                      sequence_length = 1;
                  }
                }
                
                if ( sequence_length )
                  fix_sequence();
              }
            }
            
            // one pair
            auto is_one_pair = count_of_pairs == 1;
            
            // two pair
            const auto is_two_pair = count_of_pairs == 2;
            
            // full house
            const auto is_full_house = is_one_pair and is_three_of_a_kind;
            if ( is_full_house )
            {
              is_one_pair = false;
              is_three_of_a_kind = false;
            }
            
            // straight flush
            auto is_straight_flush = is_straight and is_flush;
            if ( is_straight_flush )
            {
              is_straight = false;
              is_flush = false;
            }
            
            // royal straight flush
            const auto is_royal_straight_flush = is_straight_flush and numbers.front() == 1 and numbers.back() == 13;
            if ( is_royal_straight_flush )
              is_straight_flush = false;
            
            stringstream s;
            
            // レート計算
            double rate = 1.0;
            
            if ( is_royal_straight_flush )
            {
              rate *= 5000;
              log( "[game] the player hit the hand: ROYAL STRAIGHT FLUSH ( x 5000 )" );
              ++player_mapper->second.count_of_royal_straight_flush;
            }
            else if ( is_straight_flush )
            {
              rate *= 1000;
              log( "[game] the player hit the hand: STRAIGHT FLUSH ( x 1000 )" );
              ++player_mapper->second.count_of_straight_flush;
            }
            else if ( is_four_of_a_kind )
            {
              rate *= 100;
              log( "[game] the player hit the hand: FOUR OF A KIND ( x 100 )" );
              ++player_mapper->second.count_of_four_of_a_kind;
            }
            else if ( is_full_house )
            {
              rate *= 40;
              log( "[game] the player hit the hand: FULL HOUSE ( x 40 )" );
              ++player_mapper->second.count_of_full_house;
            }
            else if ( is_flush )
            {
              rate *= 30;
              log( "[game] the player hit the hand: FLUSH ( x 30 )" );
              ++player_mapper->second.count_of_flush;
            }
            else if ( is_straight )
            {
              rate *= 20;
              log( "[game] the player hit the hand: STRAIGHT ( x 20 )" );
              ++player_mapper->second.count_of_straight;
            }
            else if ( is_three_of_a_kind )
            {
              rate *= 5;
              log( "[game] the player hit the hand: THREE OF A KIND ( x 5 )" );
              ++player_mapper->second.count_of_three_of_a_kind;
            }
            else if ( is_two_pair )
            {
              rate *= 2;
              log( "[game] the player hit the hand: TWO PAIR ( x 2 )" );
              ++player_mapper->second.count_of_two_pair;
            }
            else if ( is_one_pair )
            {
              rate *= 1;
              log( "[game] the player hit the hand: ONE PAIR ( x 1 )" );
              ++player_mapper->second.count_of_one_pair;
            }
            else
            {
              rate = 0;
              log( "[game] the player's hand is HIGH CARDS ( x 0 )" );
              ++player_mapper->second.count_of_high_cards;
            }
            
            pot *= rate;
            
            if ( rate )
            {
              ante_max_extended = pot;
              player_mapper->second.chips += pot;
            }
            else
              ante_max_extended = ante_max;
            
            pot = 0;
          }
        }
        
        log( "[system] play end" );
      }
      
      auto print_result()
      {
        log( "[system] print result begin" );
        
        stringstream buffer;
        buffer
          << "--------------------------------\n"
             "poker result ( play count: " << play_count << " )\n"
             "--------------------------------" << endl
          ;
        
        {
          if ( not player_mapper )
          {
            buffer << "player is not set" << endl;
            return;
          }
          
          buffer
            << "name: "  << player_mapper->first->get_name()                                             << "\n"
               "chip: "  << player_mapper->second.chips                                                  << "\n"
               "--------------------------------"                                                        << "\n"
               "count of ROYAL STRAIGHT FLUSH = " << player_mapper->second.count_of_royal_straight_flush << "\n"
               "count of STRAIGHT FLUSH       = " << player_mapper->second.count_of_straight_flush       << "\n"
               "count of FOUR OF A KIND       = " << player_mapper->second.count_of_four_of_a_kind       << "\n"
               "count of FULL HOUSE           = " << player_mapper->second.count_of_full_house           << "\n"
               "count of FLUSH                = " << player_mapper->second.count_of_flush                << "\n"
               "count of STRAIGHT             = " << player_mapper->second.count_of_straight             << "\n"
               "count of THREE OF A KIND      = " << player_mapper->second.count_of_three_of_a_kind      << "\n"
               "count of TWO PAIR             = " << player_mapper->second.count_of_two_pair             << "\n"
               "count of ONE PAIR             = " << player_mapper->second.count_of_one_pair             << "\n"
               "count of HIGH CARDS           = " << player_mapper->second.count_of_high_cards           << "\n"
               "--------------------------------"                                                        << endl
            ;
        }
        
        log( buffer.str() );
        
        log( "[system] print result end");
      }
      
    public:
      
      auto set_log_target( ostream& o )
      {
        log_target = &o;
      }
      
      auto get_player_chips()
      {
        return player_mapper->second.chips;
      }
      
      auto set_player( const shared_player_type& in )
      {
        player_mapper = make_pair( in, player_data_type() );
        is_initialized = false;
      }
      
      auto set_play_count_limit( const size_t in )
      {
        play_count_limit = in;
      }
      
      auto set_player_initial_chips( const size_t in )
      {
        player_initial_chips = in;
      }
      
      auto set_ante_min( const size_t in )
      {
        ante_min = max( in, static_cast< size_t >( 1 ) );
      }
      
      auto set_ante_max( const size_t in )
      {
        ante_max = max( in, ante_min );
      }
      
      auto set_logging( const bool in )
      {
        is_output_logging = in;
      }
      
      auto operator()()
      {
        if ( not is_initialized )
          initialize();
        
        while ( play_count < play_count_limit )
          play();
        
        print_result();
      }
  };
}
