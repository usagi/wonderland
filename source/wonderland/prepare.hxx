#pragma once

#include <boost/filesystem.hpp>

namespace usagi::wonderland
{
  using path_type = boost::filesystem::path;
  
  auto prepare( const path_type& p ) -> void;
}