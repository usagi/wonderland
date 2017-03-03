#include "prepare.hxx"
#include <usagi/log/easy_logger.hxx>

namespace usagi::wonderland
{
  auto prepare( const path_type& p )
    -> void
  {
    using namespace boost::filesystem;
    
    LOGI << "prepare: " << p.string();
    
    const auto pp = p.parent_path();
    
    if ( not exists( pp ) )
    {
      LOGW << "create: " << pp;
      create_directories( pp );
    }
    
    LOGI << "succeeded";
  }
}