#include <thread>
#include <string>
#include <iostream>
#include <random>

#include "TimedCache.h"

#define T_CHECK_EQUAL( l, r ) if( !((l) == (r)) ) throw std::runtime_error(std::to_string(__LINE__));
#define T_ERROR( msg ) throw std::runtime_error( msg + std::string(" - line = ")+  std::to_string(__LINE__));

void test_set_get()
{
    std::cout << __func__ << std::endl;
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
    std::cout << __func__ << std::endl;
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

void test_update()
{
    std::cout << __func__ << std::endl;
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

    // зарпашиваем один и тот же объект, его время должно обновлятся,
    // поэтому он всегда будет.
    for( size_t i = 0; i < 10; ++i )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
        if( !cache.get( 1 ).has_value() )
            T_ERROR( "dont update timer!" );
    }
    // Ждем чтобы все объекты "протухли"
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

    // Теперь недолжно быть ни одного объекта
    for( size_t i = 0; i < test_data.size(); ++i )
    {
        auto& v = test_data[i];
        if( auto fv = cache.get( v.first ); fv.has_value() )
        {
            T_ERROR( "didnt delete element from cache!" );
        }
    }
}

void test_thread_cleaner()
{
    std::cout << __func__ << std::endl;
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

    // Ждем чтобы все объекты "протухли"
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

    if( cache.size() != 0 )
        T_ERROR( "cleaner doesn't work!" );
}

void test_multithreading()
{
    std::cout << __func__ << std::endl;
    TimedCache< int, std::string > cache( 1000, std::chrono::seconds( 1 ) );

    std::vector< std::pair< int, std::string > > test_data( size_t( 10 ) );
    int i = 0;
    std::generate( test_data.begin(), test_data.end(),
        [&]()-> std::pair< int, std::string >
    {
        ++i;
        return std::make_pair( i, std::to_string( i ) );
    } );

    std::vector< std::thread > threads;
    struct Worker
    {
        Worker( TimedCache<int, std::string>& cache, size_t countIteration )
            : m_cache( cache ), m_CountIteration( countIteration )
        {}
        Worker( Worker&& other ) 
            : m_cache( other.m_cache ), m_CountIteration(other.m_CountIteration)
        {}
        ~Worker()
        {
            if( m_thread.joinable() )
                m_thread.join();
        }
        void start()
        {
            m_thread = std::thread( &Worker::execute, this );
        }
        void execute()
        {
            std::mt19937 mt{ std::default_random_engine() };
            std::uniform_int_distribution dist( 0, 1000 );
            for( size_t i = 0; i < m_CountIteration; ++i )
            {
                int v = dist( mt );
                if( !m_cache.get( v ).has_value() )
                    m_cache.set( v, std::to_string( v ) );
            }
        }
    private:
        size_t m_CountIteration = 0;
        TimedCache<int, std::string>& m_cache;
        std::thread m_thread;
    };
    
    std::vector< Worker > workers;
    for( size_t i = 0; i < std::thread::hardware_concurrency(); ++i )
    {
        workers.emplace_back( cache, 10000 );
    }
    std::for_each( std::begin( workers ), std::end( workers ), std::mem_fn( &Worker::start ) );
    workers.clear();
    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
    if( cache.size() != 0 )
        T_ERROR( "incorect work cache!" );
}

int main()
{
    try
    {
        test_set_get();
        test_timer();
        test_update();
        test_thread_cleaner();
        test_multithreading();
    }
    catch( const std::exception& err )
    {
        std::cerr << err.what() << std::endl;
        return 1;
    }
    
    return 0;
}
