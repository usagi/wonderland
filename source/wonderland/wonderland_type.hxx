#pragma once

#include "parametric_object_type.hxx"
#include <usagi/log/easy_logger.hxx>
#include <boost/filesystem.hpp>

namespace usagi::wonderland
{
  class wonderland_type
    : public parametric_object_type
  {
    //boost::unordered_map< uuid_type, value_type > users;
    //boost::unordered_map< string, shared_ptr< world_type > > worlds;
    
    auto load_user( const path_type& user_path ) -> void;
    auto load_users() -> void;
    
    auto load_world( const string& world_name ) -> void;
    
  public:
    
    static constexpr char*    default_configuration_path                = "var/configuration/wonderland.json";
    
    static constexpr bool     default_httpd_service                     = true;
    static constexpr uint16_t default_httpd_port                        = 65535;
    static constexpr bool     default_httpd_api_jsonrpc_2_0             = true;
    static constexpr bool     default_httpd_api_cereal_portable_binary  = true;
    
    static constexpr char* key_configuration                                  = "configuration";
    static constexpr char* key_configuration_path                             = "configuration.path";
    static constexpr char* key_configuration_httpd_service                    = "configuration.httpd.service";
    static constexpr char* key_configuration_httpd_port                       = "configuration.httpd.port";
    static constexpr char* key_configuration_httpd_api_jsonrpc_2_0            = "configuration.httpd.api.jsonrpc_2_0";
    static constexpr char* key_configuration_httpd_api_cereal_portable_binary = "configuration.httpd.api.cereal_portable_binary";
    static constexpr char* key_configuration_user_path                        = "configuration.user.path";
    
    auto set_configuration_path( const path_type& in ) -> void;
    auto get_configuration_path() -> path_type;
    
    auto set_httpd_service( const bool in ) -> void;
    auto get_httpd_service() -> bool;
    
    auto set_httpd_port( const uint16_t in ) -> void;
    auto get_httpd_port() -> uint16_t;
    
    auto set_httpd_api_jsonrpc_2_0( const bool in ) -> void;
    auto get_httpd_api_jsonrpc_2_0() -> bool;
    
    auto set_httpd_api_cereal_portable_binary( const bool in ) -> void;
    auto get_httpd_api_cereal_portable_binary() -> bool;
    
    auto start() -> void;
    auto pause() -> void;
    auto stop()  -> void;
    
    auto validate() -> void override;
    
    auto load_parameters()  -> void;
    auto store_parameters() -> void;
    
  };
}