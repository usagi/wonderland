#pragma once

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <string>

namespace usagi::wonderland
{
  using namespace std;
  
  using uuid_type = boost::uuids::uuid;
  
  auto make_uuid() -> uuid_type;
  auto make_uuid( const string& in ) -> uuid_type;
}