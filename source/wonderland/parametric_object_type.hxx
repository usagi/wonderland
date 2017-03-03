#pragma once

#include <usagi/log/easy_logger.hxx>
#include <usagi/json/picojson.hxx>
#include <usagi/mutex.hxx>
#include "uuid.hxx"
#include <memory>
#include <string>
#include <functional>

namespace usagi::wonderland
{
  using namespace std;
  
  using namespace usagi::mutex;
  
  using namespace usagi::json::picojson;
  using namespace usagi::json::picojson::io::filesystem;
  using namespace usagi::json::picojson::rpc::jsonrpc20;
  
  class parametric_object_type
  {
  protected:
    value_type            parameters = make_object_value();
    read_write_mutex_type parameters_mutex;
    function< void() >    storerer = [=]{ store_parameters(); };
    
    virtual auto validate() -> void;
    virtual auto try_validate() -> void final;
  public:
    
    static constexpr auto key_uuid = "uuid";
    static constexpr auto key_category = "category";
    static constexpr auto default_category = "unknown";
    
    virtual ~parametric_object_type() noexcept;
    
    template < typename T >
    auto set_parameter( const string& key, const T& value )
    {
      try
      {
        //write_lock_type lock = write_lock( parameters_mutex );
        set_value( parameters, key, make_value( value ) );
        LOGI << "succeeded category=" << get_category() << " uuid=" << get_uuid() << " key=`" << key << "` value=`" << make_value( value ) << '`';
      }
      catch ( const exception& e )
      { LOGE << "failed: " << e.what() << " category=" << get_category() << " uuid=" << get_uuid() << " key=`" << key << "` value=`" << make_value( value ) << '`'; }
      catch ( ... )
      { LOGE << "failed category=" << get_category() << " uuid=" << get_uuid() << " key=`" << key << "` value=`" << make_value( value ) << '`';; }
    }
    
    template < typename T >
    decltype( auto ) set_parameter( const string& key, T&& value )
    {
      try
      {
        //const write_lock_type lock = write_lock( parameters_mutex );
        set_value( parameters, key, make_value( move( value ) ) );
      }
      catch ( const exception& e )
      { LOGE << "failed: " << e.what() << " category=" << get_category() << " uuid=" << get_uuid() << " key=`" << key << "` value=`" << make_value( value ) << '`'; }
      catch ( ... )
      { LOGE << "failed category=" << get_category() << " uuid=" << get_uuid() << " key=`" << key << "` value=`" << make_value( value ) << '`';; }
    }
    
    template < typename T >
    auto get_parameter_as( const string& key )
    {
      //const read_lock_type lock = read_lock( parameters_mutex );
      return get_value_as< T >( parameters, key );
    }
    
    template < typename T >
    auto get_parameter_as_optional( const string& key )
    {
      //const read_lock_type lock = read_lock( parameters_mutex );
      return get_value_as_optional< T >( parameters, key );
    }
    
    template < typename T >
    auto get_parameter_as( const string& key, const function< T () >& default_factory )
    {
      auto o = get_parameter_as_optional< T >( key );
      
      if ( not o )
      {
        const auto default_value = make_value( default_factory() );
        LOGW << '`' << key << "` is not found then set to default value: `" << default_value << '`';
        set_parameter( key, default_value );
        o = get_parameter_as_optional< T >( key );
        if ( not o )
        {
          LOGE << "failed to set `" << key << "` to default value: `" << default_value << '`';
          return get_value_as< T >( default_value );
        }
      }
      
      return *o;
    }
    
    template < typename T >
    auto get_parameter_as( const string& key, const T& default_value )
    { return get_parameter_as< T >( key, [&]{ return default_value; } ); }
    
    template < typename T >
    auto get_parameter_as( const string& key, T&& default_value )
    { return get_parameter_as< T >( key, [ default_value = move( default_value ) ]{ return default_value; } ); }
    
    virtual auto get_category() -> string;
    virtual auto get_uuid_string() -> string final;
    virtual auto get_uuid() -> uuid_type final;
    virtual auto get_path() -> path_type;
    
    auto load_parameters() -> void;
    auto store_parameters() -> void;
  };
  
}