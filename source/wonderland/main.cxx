//#define DISABLE_USAGI_LOG_EASY_LOGGER

#include "wonderland_type.hxx"

#include <usagi/json/picojson.hxx>
#include <cmdline.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <map>
#include <memory>
#include <algorithm>
#include <cstdlib>

#define APPLICATION_NAME_STRING "AI from Wonderland"
#define VERSION_STRING          "0.0.0"

namespace
{
  using namespace std;
  using namespace usagi::wonderland;
  
  [[noreturn]]
  static inline
  auto show_version_information()
  {
    std::cout
      << APPLICATION_NAME_STRING
      << ' '
      << VERSION_STRING
      << std::endl
      ;
    
    std::exit( 0 );
  }
  
  auto make_wonderland
  ( int    count_of_parameters
  , char** parameters
  )
  {
    auto wonderland = make_shared< wonderland_type >();
    
    constexpr auto configuration_path               = "configuration.path";
    constexpr auto httpd_service                    = "httpd.service";
    constexpr auto httpd_port                       = "httpd.port";
    constexpr auto httpd_api_jsonrpc_2_0            = "httpd.api.jsonrpc_2_0";
    constexpr auto httpd_api_cereal_portable_binary = "httpd.api.cereal_portable_binary";
    
    cmdline::parser p;
    
    p.add( "version", '\0', "show version information." );
    
    p.add< string >( configuration_path               ,   0, "configuration path"               , false, wonderland_type::default_configuration_path              );
    
    p.add< bool   >( httpd_service                    , 'H', "HTTPd service state"              , false, wonderland_type::default_httpd_service                   );
    p.add< int    >( httpd_port                       , 'p', "HTTPd port number"                , false, wonderland_type::default_httpd_port                      , cmdline::range( 1, 65535 ) );
    p.add< bool   >( httpd_api_jsonrpc_2_0            , 'J', "HTTPd API JSONRPC-2.0"            , false, wonderland_type::default_httpd_api_jsonrpc_2_0           );
    p.add< bool   >( httpd_api_cereal_portable_binary , 'C', "HTTPd API CEREAL portable binary" , false, wonderland_type::default_httpd_api_cereal_portable_binary);
    
    p.parse_check( count_of_parameters, parameters );
    
    if ( p.exist( "version" ) )
      show_version_information();
    
    if ( p.exist( configuration_path ) )
      wonderland->set_configuration_path( p.get< string >( configuration_path ) );
    
    wonderland->load_parameters();
    
#define WONDERLAND_TMP( TYPE, NAME ) \
    if ( p.exist( NAME ) ) \
      wonderland->set_ ## NAME ( p.get< TYPE >( NAME ) );
    
    WONDERLAND_TMP( bool, httpd_service                     )
    WONDERLAND_TMP( int , httpd_port                        )
    WONDERLAND_TMP( bool, httpd_api_jsonrpc_2_0             )
    WONDERLAND_TMP( bool, httpd_api_cereal_portable_binary  )
    
#undef WONDERLAND_TMP
    
    return wonderland;
  }
}

#include <usagi/http.hxx>
#include "darty_js.hxx"
#include "darty_ai_type.hxx"
#include "draw_poker.hxx"

namespace
{
  using namespace std;
  using namespace usagi::poker;
  using namespace usagi::json::picojson;
  
  class js_player_type
    : public player_type
  {
      darty_ai_type ai;
      
    public:
      js_player_type( const darty_ai_type& ai_ ): ai( ai_ ) { }
    
      auto get_name() -> std::string override { return ai.author + "/" + ai.name; }
      
      auto pay_ante( const size_t my_chips, const size_t minimum, const size_t maximum ) -> size_t override
      {
        try
        {
          return
            get_value_as< size_t >
            ( darty_js_from_string
              ( ai.code
              , "pay_ante"
              , make_object_value
                ( "my_chips", my_chips
                , "minimum" , minimum
                , "maximum" , maximum
                )
              )
            );
        }
        catch ( ... )
        { }
        
        return 0;
      }
      
      auto discard_cards( const cards_type cards ) -> cards_type override
      {
        try
        {
          array_type a;
          
          for ( const auto& c: cards )
          {
            stringstream buffer;
            switch ( c.get_suit() )
            { case suit_type::club    : buffer << 'c'; break;
              case suit_type::heart   : buffer << 'h'; break;
              case suit_type::diamond : buffer << 'd'; break;
              case suit_type::spade   : buffer << 's'; break;
            }
            buffer << to_string( c.get_number() );
            a.emplace_back( make_value( buffer.str() ) );
          }
          
          const value_type p = make_value( a );
          
          const auto json_cards = darty_js_from_string( ai.code, "discard_cards", p ).get< array_type >();
          
          cards_type r;
          
          for ( const value_type& json_card : json_cards )
          {
            const auto c = json_card.get< std::string >();
            suit_type s;
            if ( c.empty() )
            {
              LOGW << "empty card";
              continue;
            }
            switch ( c[0] )
            { case 'c': s = suit_type::club; break;
              case 'h': s = suit_type::heart; break;
              case 'd': s = suit_type::diamond; break;
              case 's': s = suit_type::spade; break;
            }
            auto n = static_cast< card_type::number_type >( stoul( c.substr( 1 ) ) );
            r.emplace_back( move( s ), move( n) );
          }
          
          return r;
        }
        catch ( ... )
        { }
        
        return { };
      }
  };
  
}

auto main
( int    count_of_parameters
, char** parameters
) -> int
{
  using namespace std;
  using namespace usagi::wonderland;
  
  /*
  if ( auto wonderland = make_wonderland( count_of_parameters, parameters ) )
  {
    LOGI << "start wonderland";
    wonderland->start();
  }
  //*/
  
  using namespace usagi;
  
  atomic< bool > is_running;
  
  // とりあえず今回はスコア順の表示が多いのでこれで。
  // 実際問題としてはスコアソートはそれはそれ、AIsはそれはそれで管理したり、ユーザーと絡めていく必要がある。
  // score -> ai{name,author,code,result}
  multimap< uint64_t, darty_ai_type, greater< uint64_t > > ais;
  std::mutex ais_mutex;
  
  {
    cmdline::parser p;
    p.add< int    >( "port"   , 'p', "HTTPd port number", false, 55555, cmdline::range( 1, 65535 ) );
    p.add< string >( "address", 'a', "bind address; if you need wildcard then set 127.0.0.1", false, "127.0.0.1" );
    p.parse_check( count_of_parameters, parameters );
    
    auto s = http::server_type::make_unique( p.get< int >( "port" ), p.get< string >( "address" ) );
    
    s->only_localhost = false;
    /*
    s->add_handler
    ( // path
      "/exit"
    , [&]
      ( const auto& method, const auto& path, const auto& headers, const auto& body )
        mutable
        -> http::handler_return_type
      {
        LOGI << "access /exit";
        is_running = false;
        return { { { "content-type"s, "text/plain"s } }, "exit"s };
      }
    );
    */
    const auto make_ranking =
      [&]
      {
        stringstream ranking_buffer;
        
        {
          size_t   current_rank = 0;
          uint64_t before_chips = -1;
          for ( auto i = ais.begin(); i != ais.end(); ++i )
          {
            if ( before_chips != i->first )
            {
              ++current_rank;
              before_chips = i->first;
            }
            
            ranking_buffer
              << current_rank << " 位 ( " << i->first << " [chips] ) "
                "<a href=\"/ai/" << i->second.author << "/" << i->second.name << "\">" << i->second.author << "/" << i->second.name << "</a><br />\n";
          }
        }
        
        return ranking_buffer.str();
      };
    
    s->add_handler
    ( // path
      "/example/nothing-man.js"
    , [&]
      ( const auto& method, const auto& path, const auto& headers, const auto& body )
        mutable
        -> http::handler_return_type
      {
        LOGI << "access /example/nothing-man.js";
        
        return 
          { // headers
            { { "content-type"s, "application/javascript"s }
            }
            // body
          , u8R"(function get_name()
{ return "Heart-man" }

function get_author()
{ return "usagi" }

function pay_ante( params )
{ return 1 }

function discard_cards( drew_cards )
{ return [] })"
          };
      }
    );
    
    s->add_handler
    ( // path
      "/example/heart-man.js"
    , [&]
      ( const auto& method, const auto& path, const auto& headers, const auto& body )
        mutable
        -> http::handler_return_type
      {
        LOGI << "access /example/heart-man.js";
        
        return 
          { // headers
            { { "content-type"s, "application/javascript"s }
            }
            // body
          , u8R"(function get_name()
{ return "Heart-man" }

function get_author()
{ return "usagi" }

function pay_ante( params )
{
  return 1
}

function discard_cards( drew_cards )
{
  return drew_cards.filter( function( a ){ return /^[^h][0-9]+$/.test( a ) } )
})"
          };
      }
    );
        
    s->add_handler
    ( // path
      "/"
    , [&]
      ( const auto& method, const auto& path, const auto& headers, const auto& body )
        mutable
        -> http::handler_return_type
      {
        LOGI << "access /";
        
        return 
          { // headers
            { { "content-type"s, "text/html"s }
            }
            // body
          , u8R"(<h1>POKER AI 試作2号くん</h1>
<article>
  <h2>Ranking</h2>
)" + make_ranking() + u8R"(
</article>
<article>
  <h2>Entry AI</h2>
  <form method="POST" action="/entry" enctype="text/plain">
    <textarea name="code" cols="80" rows="20"></textarea>
    <input type="submit" />
  </form>
  <h2>tips</h2>
  <ul>
    <li style="color:red">謎のバグがあり、コードが大きいと送信をぽちってから1分以上待っても終わらない事があります。その場合もたいてい数秒待てばエラーが無い限りエントリー完了していますので、一旦ESCキーでリクエストを中止するなどした後、別タブでトップページを開いてランキングに追加されていないか確認してみるなどお願いします＞＜</li>
    <li>コードは ASCII 文字のみでお願いします。（ 試作の手抜き実装の仕様上の誤動作が懸念されるため ）</li>
    <li>サンプルのAIコードはこちら → 1. <a href="/example/nothing-man.js">Nothing-man</a> 2. <a href="/example/heart-man.js">usagi/Hart-man</a></li>
    <li>name は他の方と被らないものをヒューマンブレインでチョイスをお願いします。</li>
    <li>name と author は [a-zA-Z][0-9][-_]+ でお願いします。</li>
    <li>get_name と get_author の結果が同じ AI を投稿すると update 動作になります。</li>
    <li>pay_ante には JavaScript の Object 型の引数が1つ params が渡され、
      params["my_chips"] で現在の所持チップ枚数、 
      params["minimum"] でプレイに必要な最小のチップ枚数、 
      params["maximum"] でプレイに掛けられる最大のチップ枚数 
      を読み出せます。
      所持チップ枚数よりも多くのチップを掛けた場合（ return 999999 など ）は所持枚数と掛けられる最大のチップ枚数のうち、より大きな方の枚数でチップを掛ける動作になります。
    </li>
    <li>discard_cars には JavaScript の Array 型の引数が1つ drew_cards が渡されます。
      この Array には配られた手札が5枚入っています。（ 例: [ "h1", "h2", "d5", "s13", "c10" ] ）
      カードは文字列で扱い、スート（記号）を {ハート, ダイヤ, スペード, クラブ} → {h,d,s,c} 、数値を 1,2,3,..,11,12,13 で結合して用います。（ 例: ハートのキング → "h13" ）
      return で「捨てたいカード」を返します。残したいカードではないので注意して下さい。（ "discard" です。 ）
    </li>
    <li>編集はできるだけお好みのテキストエディターで行い、投稿時にコピペして送信して下さい。編集していたコードが消えてウボアーしても仕様です＞＜</li>
    <li>JavaScript のヒント
      <ul>
        <li>JavaScript のテスト環境はお使いのウェブブラウザーで適当なページを開いている状態で、たいてい F12 キーを押すと出て来る「開発者コンソール機能」的なものを使えば十分に楽しめます。（Chrome,Firefox,IE11などで使えます。）</li>
        <li>変数は var x = 123 などとゆるーく作れます。</li>
        <li>Array のリテラルは [ 1, 2, 3 ] のように書きます。</li>
        <li>Object のリテラルは { key1: "value1", key2: "value2" } のように書きます。</li>
        <li>Array のオブジェクトには [1,2,3,4,5].filter( function( a ){ return a % 2 } ) のようにオブジェクトのメソッド（メンバー関数）に filter にファンクターを渡して [1,3,5] を得たり、
          .map( function( a ){ return a % 2 } ) として [1,0,1,0,1] を得たり、
          .reduce( function( a, b ){ return a + b } ) として 15 を得たり
          できます。
        <li>RegExp （正規表現）のリテラルは /^[^h][0-9]+$/ のように書きます。 
          var r = /^[^h][0-9]+$/ とすると変数に入れる事もできます。 
          /^[^h][0-9]+$/.test( "h13" ) とすると .test で与えた引数文字列（変数を入れてもOK）に正規表現がマッチした場合に true 、それ以外の場合に false を返してくれます。
        </li>
        <li>数値は 32-bit 整数か 64-bit 浮動小数点数（IEEE754/Binary64）で扱わないと痛い目に遭うかもしれません。 </li>
        <li>C++ みたいにステートメントの区切りに ; を付ける必要はありません（必要はないだけで付けた方が安全だと言語の仕様書には書いてあります。）</li>
        <li>詳しい事は <a href="http://www.ecma-international.org/publications/standards/Ecma-262.htm">ECMA-262</a> を見ると結構読み易く書いてあります。言語仕様書入門にお勧めの一冊。 </li>
      </ul>
    </li>
  </ul>
</article>
)"s
          }
          ;
      }
    );
    
    s->add_handler
    ( // path
      "/entry"
    , [&]
      ( const auto& method, const auto& path, const auto& headers, const auto& body )
        mutable
        -> http::handler_return_type
      {
        LOGI << "access /entry";
        
        LOGD << "body: size = " << body.size() << " data = " << body;
        
        if ( body.size() < 6 )
          return { { { "content-type"s, "text/plain"s } }, "error: invalid posting"s };
        
        // code を取り出し。
        // 本当は post を受ける mime に応じてもうちょっとまともに処理してあげる事になる。
        auto code = body.substr( 5 );
        
        LOGD << "code = " << code;
        
        string name;
        {
          const value_type v = darty_js_from_string( code, "get_name" );
          try
          {
            name = v.get< string >();
            if ( name.empty() )
              throw;
            // TODO: regex
          }
          catch ( ... )
          {
            return { { { "content-type"s, "text/plain"s } }, "error: invalid definition in `get_name`"s };
          }
          
        }
        
        LOGD << "name checked";
        
        string author;
        {
          const value_type v = darty_js_from_string( code, "get_author" );
          try
          {
            author = v.get< string >();
            if ( author.empty() )
              throw;
            // TODO: regex
          }
          catch ( ... )
          {
            return { { { "content-type"s, "text/plain"s } }, "error: invalid definition in `get_author`"s };
          }
          
        }
        
        LOGD << "author checked";
        
        {
          const value_type pay_ante =
            darty_js_from_string
            ( code
            , "pay_ante"
            , make_object_value
              ( "my_chips", 100
              , "minimum" ,   1
              , "maximum" , 100
              )
            );
          
          if ( not pay_ante.is< number_type >() )
            return { { { "content-type"s, "text/plain"s } }, "error: invalid definition in `pay_ante`"s };
        }
        
        LOGD << "pay_ante checked";
        
        {
          const value_type discard_cards = darty_js_from_string( code, "discard_cards"  , make_array_value( "h1", "d2", "c3", "s4", "h13" ) );
          
          if ( not discard_cards.is< array_type >() )
            return { { { "content-type"s, "text/plain"s } }, "error: invalid definition in `discard_cards`"s };
        }
        
        LOGD << "discard_cards checked";
        
        // TODO: ここでポーカーエンジンに突っ込んでバトる
        poker::draw_poker_type draw_poker;
        
        draw_poker.set_play_count_limit( 100 );
        draw_poker.set_player_initial_chips( 100 );
        draw_poker.set_ante_min( 1 );
        draw_poker.set_ante_max( 100 );
        draw_poker.set_logging( true );
        
        stringstream log_buffer;
        draw_poker.set_log_target( log_buffer );
        
        {
          darty_ai_type ai;
          ai.name   = name;
          ai.author = author;
          ai.code   = code;
          
          draw_poker.set_player( make_shared< js_player_type >( move( ai ) ) );
        }
        
        draw_poker();
        
        const auto result = log_buffer.str();
        const auto chips = draw_poker.get_player_chips();
        
        // ここまで来たらエントリー成功
        
        LOGI << "name = "   << name;
        LOGI << "author = " << author;
        LOGI << "result = " << result;
        LOGI << "chips = " << chips;
        
        // ここから ais の操作を伴うのでロック
        std::lock_guard< decltype( ais_mutex ) > l( ais_mutex );
        
        {
          auto i =
            find_if
            ( ais.begin()
            , ais.end()
            , [&]( const auto& p ){ return p.second.name == name and p.second.author == author; }
            );
          
          if ( i != ais.end() )
          {
            // 既に登録済みの ais から {name,author} が一致した場合は update する
            LOGI << "entry process: update";
            auto ai = move( i->second );
            ais.erase( i );
            ai.code   = code;
            ai.result = result;
            ais.emplace( chips, move( ai ) );
          }
          else
          {
            // 新規登録
            LOGI << "entry process: new";
            darty_ai_type ai;
            ai.name   = name;
            ai.author = author;
            ai.code   = code;
            ai.result = result;
            ais.emplace( chips, move( ai ) );
            
            s->add_handler
            ( // path
              "/ai/" + author + "/" + name
            , [&, author = author, name = name]
              ( const auto& method, const auto& path, const auto& headers, const auto& body )
                mutable
                -> http::handler_return_type
              {
                LOGD << author;
                LOGD << name;
                
                std::lock_guard< decltype( ais_mutex ) > l( ais_mutex );
                const auto i = find_if( ais.begin(), ais.end(), [&]( const auto& p ){ return p.second.author == author and p.second.name == name; } );
                
                if ( i == ais.end() )
                  return { { { "content-type"s, "text/plain"s } }, "error: not found"s };
                
                const auto& ai = i->second;
                
                return 
                  { // headers
                    { { "content-type"s, "text/html"s }
                    }
                    // body
                  , u8R"(<h1>POKER AI 試作2号くん</h1>
<article>
  <h2>AI: )" + ai.author + "/" + ai.name + u8R"(</h2>
  <pre><code>)" + ai.code + u8R"(</code></pre>
  <pre><code>)" + ai.result + u8R"(</code>/pre>
</article>
<nav>
  <ul>
    <a href="/"><li>top</li></a>
  </ul>
</nav>
)"s
                  };
                
              }
            );
          }
        }
        
        size_t rank = 0;
        
        {
          size_t   current_rank = 0;
          uint64_t before_chips = -1;
          for ( auto i = ais.begin(); i != ais.end(); ++i )
          {
            if ( before_chips != i->first )
            {
              ++current_rank;
              before_chips = i->first;
            }
            
            if ( i->second.author == author and i->second.name == name )
            {
              rank = current_rank;
              break;
            }
          }
        }
        
        return 
          { // headers
            { { "content-type"s, "text/html"s }
            }
            // body
          , u8R"(<h1>POKER AI 試作2号くん</h1>
<article>
  <h2>Entry</h2>
  <p>)<pre><code>)"s + code + u8R"(</code></pre></p>
</article>
<article>
  <h2>Result</h2>
  <article>
    <h3>Chips</h3>
    <p> )"s + to_string( chips ) + u8R"( [chips]</p>
  </article>
  <article>
    <h3>Rank</h3>
    <p> )"s + to_string( rank ) + u8R"( 位 （ 現在 )"s + to_string( ais.size() ) + u8R"( の AI が登録されています。 ）</p>
  </article>
  <article>
    <h3>Game</h3>
    <pre><code>)"s + result + u8R"(</code></pre>
  </article>
</article>
<nav>
  <ul>
    <a href="/"><li>top</li></a>
  </ul>
</nav>
)"s
          }
          ;
      }
    );
    
    LOGI << "start server";
    
    is_running = true;
    
    thread( [&]{ (*s)(); } ).detach();
    
    while( is_running )
      this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
  }
  LOGI << "server was stopped";
  
  //LOGD << darty_js( "a.js", "f", make_array_value( 1, 2, 3, 4, 5 ) );
  
  LOGI << "to exit";
}