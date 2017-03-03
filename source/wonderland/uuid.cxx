#include "uuid.hxx"

namespace usagi::wonderland
{
  namespace
  {
    static auto random_generator = boost::uuids::random_generator();
    static auto string_generator = boost::uuids::string_generator();
  }
  
  auto make_uuid() -> uuid_type
  {
    return random_generator();
  }
  
  auto make_uuid( const string& in ) -> uuid_type
  {
    return string_generator( in );
  }
}