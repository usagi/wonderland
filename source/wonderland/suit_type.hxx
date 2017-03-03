#pragma once

#include <iostream>
#include <stdexcept>

namespace usagi::poker
{
  using namespace std;
  
  enum class suit_type
    : uint8_t
  { club    = 1 << 4
  , diamond = 1 << 5
  , heart   = 1 << 6
  , spade   = 1 << 7
  };
  
  static inline decltype( auto ) operator<<( ostream& out, const suit_type suit )
  {
    switch ( suit )
    {
      case suit_type::club   : out << "Club";    break;
      case suit_type::heart  : out << "Heart";   break;
      case suit_type::diamond: out << "Diamond"; break;
      case suit_type::spade  : out << "Spade";   break;
      default:
        throw logic_error( "unknown suit" );
    }
    
    return out;
  }
}
