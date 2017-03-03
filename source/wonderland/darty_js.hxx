#include <usagi/json/picojson.hxx>

#include <fstream>
#include <sstream>
#include <mutex>

#include <cstdlib>
#include <cstdio>

namespace usagi::wonderland
{
  using namespace std;
  
  namespace
  {
    auto make_temporary_directory()
    {
      using namespace boost::filesystem;
      
      if ( not exists( "tmp" ) )
      {
        boost::system::error_code e;
        if ( not create_directory( "tmp", e ) or e )
        {
          constexpr auto message = "couldn't create a temporary directory ( tmp )";
          LOGE << message;
          throw runtime_error( message );
        }
      }
      
      const auto file = path( "tmp" ) / unique_path();
      
      LOGD << "temporary file = " << file.string();
    }
  }
  
  /// @warning いろいろダメなやっつけ実装、試験実装用。実用では使わない。後で綺麗なV8のラッパーを実装してこれは [[deprecated]] にする
  auto darty_js_from_file
  ( const string& js_file_path
  , const string& function
  , const value_type& parameter = value_type()
  ) -> value_type
  {
    try
    {
      make_temporary_directory();
      
      const auto tmp = boost::filesystem::path( "tmp" ) / boost::filesystem::unique_path();
      const auto out = boost::filesystem::path( "tmp" ) / boost::filesystem::unique_path();
      
      LOGD << "temporary file (tmp) = " << tmp;
      LOGD << "temporary file (out) = " << out;
      
      stringstream a;
      
      a << ifstream( js_file_path ).rdbuf()
        << "console.log( JSON.stringify( " << function << "( " << ( parameter.is< null_type >() ? "" : parameter.serialize() ) << " ) ) )"
        ;
      
      const auto b = a.str();
      ofstream( tmp.string() ).write( b.c_str(), b.size() );
      
      system( ( "node " + tmp.string() + " > " + out.string() ).c_str() );
      
      stringstream c;
      c << ifstream( out.string() ).rdbuf();
      
      value_type d;
      const auto e = picojson::parse( d, c.str() );
      
      if ( not e.empty() )
        LOGE << "JSON parse error: " << e;
      
      if ( not boost::filesystem::remove( tmp ) )
        LOGE << "couldn't remove a temporary file (tmp): " << tmp.string();
      
      if ( not boost::filesystem::remove( out ) )
        LOGE << "couldn't remove a temporary file (out): " << out.string();
      
      return d;
    }
    catch ( ... )
    { return { }; }
  }
  
  auto darty_js_from_string
  ( const string& js_code
  , const string& function
  , const value_type& parameter = value_type()
  ) -> value_type
  {
    LOGW << parameter;
    static std::mutex m;
    std::lock_guard< decltype( m ) > l( m );
    
    make_temporary_directory();
    
    const auto file = boost::filesystem::path( "tmp" ) / boost::filesystem::unique_path();
    
    LOGD << "temporary file = " << file.string();
    
    std::ofstream( file.string() ).write( js_code.c_str(), js_code.size() );
    
    const value_type r = darty_js_from_file( file.string(), function, parameter );
    
    if ( not boost::filesystem::remove( file ) )
      LOGE << "couldn't remove a temporary file: " << file.string();
    
    return r;
  }
  
}