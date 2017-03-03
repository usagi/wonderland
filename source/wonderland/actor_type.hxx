#pragma once

namespace usagi::wonderland
{
  class actor_type
    : public enable_shared_from_this< actor_type >
    , protected parameters_type
  {
  public:
    
    auto validate()
      -> void override
    {
      if ( not parameters.is< object_type >() )
        parameters = make_object_value( "id", to_string( make_uuid() ) );
      else
        try
        { get_id(); }
        catch ( ... )
        { set_value( parameters, "id", to_string( make_uuid() ) ); }
    }
    
    virtual auto get_id() -> uuid_type final
    { return make_uuid( get_value_as< string >( parameters, "id" ) ); }
    
    virtual auto get_name() -> string final
    { return get_value_as< string >( parameters, "name" ); }
    
    virtual auto get_path()
      -> path_type
    {
      return path_type( "x/y/z.json" );
    }
    
    virtual auto load()
      -> void
      final
    {
      parameters = usagi::json::picojson::io::filesystem::load( get_path() );
    }
    
    virtual auto store()
      -> void
      final
    {
      const auto& p = get_path();
      auto d = p;
      d.remove_filename();
      
      boost::system::error_code e1;
      const auto r1 = boost::filesystem::exists( d, e1 );
      if ( not r1 or e1 )
      {
        boost::system::error_code e2;
        const auto r2 = boost::filesystem::create_directories( d, e2 );
        if ( not r2 or e2 )
          throw std::runtime_error( "failed to create directories: " + d.string() );
      }
      else
      {
        boost::system::error_code e3;
        const auto r3 = boost::filesystem::is_directory( d, e3 );
        if ( not r3 or e3 )
          throw std::runtime_error( "path is not directory: " + d.string() );
      }
      
      usagi::json::picojson::io::filesystem::store( parameters, p );
    }
  };
  
}