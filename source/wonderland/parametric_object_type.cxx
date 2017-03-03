#include "parametric_object_type.hxx"
#include "prepare.hxx"

namespace usagi::wonderland
{
  parametric_object_type::~parametric_object_type() noexcept { try { storerer(); } catch ( ... ) { } }
  
  auto parametric_object_type::validate() -> void
  { }
  
  auto parametric_object_type::try_validate() -> void
  {
    try
    {
      validate();
      LOGI << "validation succeeded";
    }
    catch ( const exception& e )
    { LOGE << "validation error: " << e.what() << " `" << parameters << '`'; }
    catch ( ... )
    { LOGE << "validation error `" << parameters << '`'; }
  }
  
  auto parametric_object_type::get_category() -> string
  { return get_parameter_as< string >( key_category, default_category ); }
  
  auto parametric_object_type::get_uuid_string() -> string
  { return get_parameter_as< string >( key_uuid, []{ return to_string( make_uuid() ); } ); }
  
  auto parametric_object_type::get_uuid() -> uuid_type
  { return make_uuid( get_uuid_string() ); }
  
  auto parametric_object_type::get_path() -> path_type
  {
    auto c = get_category();
    replace( c.begin(), c.end(), '.', '/' );
    return path_type( "var/" + c + '/' + get_uuid_string() + ".json" );
  }
  
  auto parametric_object_type::load_parameters() -> void
  {
    try
    {
      parameters = load( get_path() );
      LOGI << "loading succeeded";
    }
    catch ( const exception& e )
    { LOGW << e.what(); }
    catch ( ... )
    { LOGW << "unknown exception"; }
    
    try_validate();
  }
  
  auto parametric_object_type::store_parameters() -> void
  {
    try_validate();
    
    try
    {
      const auto p = get_path();
      LOGD << "path=" << p;
      prepare( p );
      store( parameters, p );
      LOGI << "store succeeded";
    }
    catch ( const exception& e )
    { LOGE << e.what(); }
    catch ( ... )
    { LOGE << "unknown exception"; }
  }
  
}