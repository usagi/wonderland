#pragma once

#include "cards_type.hxx"

namespace usagi::poker
{
  using namespace std;
  
  class player_type
  {
    public:
      /// @return プレイヤー名を出力する
      virtual auto get_name() -> std::string;
      
      /// @param chips 現在の手持ちのチップ数が引数で通知される
      /// @return アンティを幾ら積んで参加するか出力する。0の場合ゲームには参加できない（パス）。
      virtual auto pay_ante( const size_t chips, const size_t minimum, const size_t maximum ) -> size_t;
      
      /// @param current_cards 現在の手札が引数で通知される
      /// @return 捨てたいカード群を出力する。
      virtual auto discard_cards( const cards_type current_cards ) -> cards_type;
      
      virtual ~player_type() { }
  };
}