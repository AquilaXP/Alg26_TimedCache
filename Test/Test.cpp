#include <thread>
#include <string>
#include <iostream>

#include "TimedCache.h"

#define T_CHECK_EQUAL( l, r ) if( !((l) == (r)) ) throw std::runtime_error(std::to_string(__LINE__));
#define T_ERROR( msg ) throw std::runtime_error( msg + std::string(" - line = ")+  std::to_string(__LINE__));

void test_set_get()
{
    // Таймер взвинчиваем, чтобы проверить работу set и get
    TimedCache< int, std::string > cache( 10, std::chrono::seconds( 10000 ) );

    std::vector< std::pair< int, std::string > > test_data( size_t( 10 ) );
    int i = 0;
    std::generate( test_data.begin(), test_data.end(),
        [&]()-> std::pair< int, std::string >
    {
        ++i;
        return std::make_pair( i, std::to_string( i ) );
    } );

    for( auto& v : test_data )
        cache.set( v.first, v.second );

    for( auto& v : test_data )
    {
        if( auto fv = cache.get( v.first ); fv.has_value() )
        {
            T_CHECK_EQUAL( fv.value(), v.second );
        }
        else
        {
            T_ERROR( "dont find element from cache!" );
        }
    }

    test_data.erase( test_data.begin() );
    test_data.push_back( std::make_pair( 111, "1234" ) );
    cache.set( test_data.back().first, test_data.back().second );

    for( auto& v : test_data )
    {
        if( auto fv = cache.get( v.first ); fv.has_value() )
        {
            T_CHECK_EQUAL( fv.value(), v.second );
        }
        else
        {
            T_ERROR( "dont find element from cache!" );
        }
    }

    if( cache.get( 110 ).has_value() )
        T_ERROR( "find element, that dont add" );
}

void test_timer()
{
    TimedCache< int, std::string > cache( 100, std::chrono::seconds( 1 ) );

    std::vector< std::pair< int, std::string > > test_data( size_t( 10 ) );
    int i = 0;
    std::generate( test_data.begin(), test_data.end(),
        [&]()-> std::pair< int, std::string >
    {
        ++i;
        return std::make_pair( i, std::to_string( i ) );
    } );

    for( auto& v : test_data )
        cache.set( v.first, v.second );

    // Ждем половину времени
    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    test_data.push_back( std::make_pair( 111, "1234" ) );
    cache.set( test_data.back().first, test_data.back().second );

    // Ждем еще половину, чтобы первые 10 устарели
    std::this_thread::sleep_for( std::chrono::milliseconds( 600 ) );
    // Последний добавленый должен присутствовать, т.к. прошло 600 ms, а данные устаревают через 1000ms
    if( auto fv = cache.get( test_data.back().first ); fv.has_value() )
    {
        T_CHECK_EQUAL( fv.value(), test_data.back().second );
    }
    else
    {
        T_ERROR( "dont find element from cache!" );
    }

    // Первые десять нет, т.к. прошло более 1100ms
    for( size_t i = 0; i < test_data.size() - 1; ++i )
    {
        auto& v = test_data[i];
        if( auto fv = cache.get( v.first ); fv.has_value() )
        {
            T_ERROR( "didnt delete element from cache!" );
        }
    }
}

int main()
{
    try
    {
        test_set_get();
        test_timer();
    }
    catch( const std::exception& err )
    {
        std::cerr << err.what() << std::endl;
        return 1;
    }
    
    return 0;
}
