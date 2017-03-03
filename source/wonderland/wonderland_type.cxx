#pragma once

#include "wonderland_type.hxx"
#include "prepare.hxx"

namespace usagi::wonderland
{
  constexpr char*     wonderland_type::default_configuration_path;
  
  constexpr bool      wonderland_type::default_httpd_service;
  constexpr uint16_t  wonderland_type::default_httpd_port;
  constexpr bool      wonderland_type::default_httpd_api_jsonrpc_2_0;
  constexpr bool      wonderland_type::default_httpd_api_cereal_portable_binary;
  
  constexpr char*     wonderland_type::key_configuration;
  
  constexpr char*     wonderland_type::key_configuration_path;
  
  constexpr char*     wonderland_type::key_configuration_httpd_service;
  constexpr char*     wonderland_type::key_configuration_httpd_port;
  constexpr char*     wonderland_type::key_configuration_httpd_api_jsonrpc_2_0;
  constexpr char*     wonderland_type::key_configuration_httpd_api_cereal_portable_binary;
  
  constexpr char*     wonderland_type::key_configuration_user_path;
  
  auto wonderland_type::load_user( const path_type& user_path )
    -> void
  {/*
    try
    { auto u = load( user_path.string() ); }
    catch ( const exception& e )
    { loge << e.what(); }
    catch ( ... )
    { loge << "unknown error"; }*/
  }
  
  auto wonderland_type::load_users()
    -> void
  {
    const auto user_path = path_type( get_parameter_as< string >( key_configuration_user_path, "var/user" ) );
    for ( const auto& p : boost::make_iterator_range( boost::filesystem::directory_iterator( user_path ), { } ) )
      if ( boost::filesystem::is_regular_file( p ) )
        load_user( p );
  }
  
  auto wonderland_type::load_world( const string& world_name )
    -> void
  {
    
  }
  
  auto wonderland_type::set_configuration_path( const path_type& in ) -> void
  { set_parameter( key_configuration_path, in.string() ); }
  
  auto wonderland_type::get_configuration_path() -> path_type
  { return get_parameter_as< string >( key_configuration_path, default_configuration_path ); }
  
  auto wonderland_type::set_httpd_service( const bool in ) -> void
  { set_parameter( key_configuration_httpd_service, in ); }
  
  auto wonderland_type::get_httpd_service() -> bool
  { return get_parameter_as< bool >( key_configuration_httpd_service, default_httpd_service ); }
  
  auto wonderland_type::set_httpd_port( const uint16_t in ) -> void
  { set_parameter( key_configuration_httpd_port, in ); }
  
  auto wonderland_type::get_httpd_port() -> uint16_t
  { return get_parameter_as< uint16_t >( key_configuration_httpd_port, default_httpd_port ); }
  
  auto wonderland_type::set_httpd_api_jsonrpc_2_0( const bool in ) -> void
  { set_parameter( key_configuration_httpd_api_jsonrpc_2_0, in ); }
  
  auto wonderland_type::get_httpd_api_jsonrpc_2_0() -> bool
  { return get_parameter_as< bool >( key_configuration_httpd_api_jsonrpc_2_0, default_httpd_api_jsonrpc_2_0 ); }
  
  auto wonderland_type::set_httpd_api_cereal_portable_binary( const bool in ) -> void
  { set_parameter( key_configuration_httpd_api_cereal_portable_binary, in ); }
  
  auto wonderland_type::get_httpd_api_cereal_portable_binary() -> bool
  { return get_parameter_as< bool >( key_configuration_httpd_api_cereal_portable_binary, default_httpd_api_cereal_portable_binary ); }
  
  auto wonderland_type::start()
    -> void
  {
  }
  
  auto wonderland_type::pause()
    -> void
  {
  /*
  using namespace usagi::json::picojson;
  using namespace usagi::json::picojson::rpc::jsonrpc20;
  
  server_type s;
  
  s.connect
  ( "get_version"
  , []( const value_type& )
    { return
        value_type
        ( make_object
          ( "version"     , VERSION_STRING
          , "application" , APPLICATION_NAME_STRING
          )
        );
    }
  );
  
  if ( p.exist( "request" ) )
  {
    std::cout << s( p.get< string >( "request" ) ) << std::endl;
    std::exit( 0 );
  }
  */
  }
  
  auto wonderland_type::stop()
    -> void
  {
  }
  
  auto wonderland_type::validate()
    -> void
  {
    LOGD << "validate";
    get_httpd_port();
  }
  
  auto wonderland_type::load_parameters()
    -> void
  {
    try
    {
      storerer = [this]{ this->store_parameters(); };
      
      if ( not parameters.is< object_type >() )
        parameters = value_type( object_type() );
      
      set_value( parameters, key_configuration, load( get_configuration_path() ) );
      set_value( parameters, key_category, "." );
    }
    catch ( const exception& e )
    { LOGW << e.what(); }
    catch ( ... )
    { LOGW << "unknown exception"; }
    
    try_validate();
  }
  
  auto wonderland_type::store_parameters()
    -> void
  {
    try_validate();
    
    try
    {
      const auto& p = get_configuration_path();
      
      prepare( p );
      
      store( get_value( parameters, key_configuration ), p );
      LOGI << "save configuration succeeded";
      
      // TODO: users
    }
    catch ( const exception& e )
    { LOGE << e.what(); }
    catch ( ... )
    { LOGE << "unknown exception"; }
  }
}