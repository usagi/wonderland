#pragma once

#include "suit_type.hxx"

#include <stdexcept>
#include <sstream>

namespace usagi::poker
{
  using namespace std;
  
  struct card_type
  {
    public:
      
      using number_type = uint8_t;
      
    private:
    
      suit_type   suit;
      number_type number;
      
    public:
      card_type( const suit_type s, const number_type n )
        : suit( s )
        , number( n )
      {
        if ( number < 1 or number > 13 )
          throw runtime_error( "card number range error ( number = " + to_string( number ) + " )" );
        switch( s )
        {
          case suit_type::club:
          case suit_type::heart:
          case suit_type::diamond:
          case suit_type::spade:
            break;
          default:
            throw runtime_error( "card suit is invalid error ( suit = " + to_string( static_cast< underlying_type_t< suit_type > >( suit ) ) + " )" );
        }
      }
      
      auto get_suit() const
      { return suit; }
      
      auto get_number() const
      { return number; }
      
      auto operator==( const card_type& target ) const
      { return suit == target.suit and number == target.number; }
      
      struct hasher
      {
        auto operator()( const card_type& card ) const
        {
          return hash< uint8_t >()( static_cast< uint8_t >( card.suit ) + card.number );
        }
      };
  };
  
  decltype( auto ) operator<<( ostream& out, const card_type& card )
  {
    stringstream s;
    s << '{' << card.get_suit() << '-';
    switch( card.get_number() )
    {
      case 1:
        s << 'A';
        break;
      case 11:
        s << 'J';
        break;
      case 12:
        s << 'Q';
        break;
      case 13:
        s << 'K';
        break;
      default:
        s << to_string( card.get_number() );
    }
    s << '}';
    return out << s.str();
  }
}
