#pragma once

#include <sstream>
#include <random>
#include <string>
#include <chrono>
#include <iomanip>
#include <numeric>

#include "ICache.h"

class Timer
{
    using select_clock = std::chrono::high_resolution_clock;
public:
    Timer() = default;

    void start()
    {
        t_start = select_clock::now();
    }

    void stop()
    {
        t_end = select_clock::now();
    }

    template< class T >
    size_t t()
    {
        return static_cast<size_t>( std::chrono::duration_cast<T>( t_end - t_start ).count() );
    }

    size_t t()
    {
        return t<std::chrono::milliseconds>();
    }

    int rel()
    {
        auto[baseT, dt] = baseT_dt();

        return static_cast<int>( dt * 100.0 / baseT );
    }

    double speedup()
    {
        auto[baseT, dt] = baseT_dt();

        return double( baseT ) / dt;
    }

    void print( std::ostream& os )
    {
        print( os, "milliseconds" );
    }

    void print( std::ostream& os, const std::string& duration )
    {
        os << "\t" << duration << ": " << std::setw( 7 ) << t<std::chrono::milliseconds>() <<
            "\trelative time:" << std::setw( 4 ) << std::setprecision( 2 ) << rel() << "%" <<
            "\tspeed-up factor: " << std::setw( 2 ) << std::setprecision( 3 ) << ( speedup() > 0.01 ? speedup() : 0.0 );
    }
private:
    std::pair< size_t, size_t > baseT_dt()
    {
        using namespace std::chrono;

        if( t_base.count() == 0 )
        {
            t_base = t_end - t_start;
        }
        size_t dt = static_cast<size_t>( duration_cast<nanoseconds>( t_end - t_start ).count() );
        size_t base = static_cast<size_t>( duration_cast<nanoseconds>( t_base ).count() );

        return std::make_pair( base, dt );
    }

    select_clock::time_point t_start = {};
    select_clock::time_point t_end = {};
    select_clock::duration   t_base = {};
};

class TestPerfomance
{
    size_t m_size;
    size_t m_random_iteration;
    size_t m_max_w_name = 0;
    bool m_debug = false;
public:

    // @param size - размер массива значений для тестриования, диапозон запросов от 0 до size
    // @param random_iteration - число циклов повторения
    void SetParam( size_t size, size_t random_iteration )
    {
        m_size = size;
        m_random_iteration = random_iteration;
    }
    void debug( bool useDebug = true )
    {
        m_debug = useDebug;
    }
    void PushCache( ICache< size_t, std::string >* cache )
    {
        m_Caches.push_back( cache );
        if( cache->name().size() > m_max_w_name )
            m_max_w_name = cache->name().size();
    }
    void Execute( std::ostream& os )
    {
        using namespace std;
        using namespace chrono;

        os << endl << endl;
        os << "Cache Benchmark Tests " << endl;
        os << endl;
        for( auto& cache : m_Caches )
        {
            os << cache->name() << '\n';
        }
        os << endl << endl;

        random( m_random_iteration, m_size, os );
    }

private:
    void random( size_t countIteration, size_t size, std::ostream& os )
    {
        os << countIteration << " times. RANDOM() . Count = " << size << '\n';
        Timer timer;

        for( auto& cache : m_Caches )
        {
            cache->clear();

            size_t cacheMiss = 0;
            std::mt19937_64 mt64( 100 );
            std::uniform_int<size_t> uniform{ 0, size };
            timer.start();

            for( size_t i = 0; i < countIteration; ++i )
            {
                size_t v = uniform( mt64 );
                if( cache->get( v ) == nullptr )
                {
                    cache->set( v, std::to_string( v ) );
                    cacheMiss++;
                }
            }

            timer.stop();
            os << std::setw( m_max_w_name + 1 ) << cache->name();
            timer.print( os );
            os << "\tCache miss: " << cacheMiss << " from " << countIteration << " - " << double( countIteration - cacheMiss ) / countIteration * 100.0 << "%\n";
        }
        os << "\n\n";
    }

    std::vector< size_t > m_data;
    std::vector< ICache< size_t, std::string >* > m_Caches;
};