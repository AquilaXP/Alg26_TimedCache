#include <iostream>
#include <string>
#include <thread>

#include "TimedCache.h"
#include "TestPerfomance.h"
#include "ICache.h"

#include "Poco/LRUCache.h"

class CacheTimedCached : public ICache< size_t, std::string >
{
public:
    template< class Rep, class Period >
    CacheTimedCached( size_t size, const std::chrono::duration< Rep, Period >& relTime )
        : m_cache( size, relTime ), m_dt( std::chrono::duration_cast<std::chrono::milliseconds>( relTime ) )
    {
    }
    CacheTimedCached( CacheTimedCached& ) = delete;

    std::string* get( const size_t& key ) override
    {
        if( m_value = m_cache.get( key ); m_value.has_value() )
        {
            return &( m_value.value() );
        }
        return nullptr;
    }
    void set( const size_t& key, const std::string& value ) override
    {
        m_cache.set( key, value );
    }
    std::string name() const override
    {
        return "TimedCache(" + std::to_string(capacity()) + "; dt= "+ std::to_string( m_dt.count() ) + "ms)";
    }
    size_t capacity() const override
    {
        return m_cache.capacity();
    }
    void clear() override
    {
        m_cache.clear();
    }
private:
    std::chrono::milliseconds m_dt;
    std::optional< std::string > m_value;
    TimedCache< size_t, std::string > m_cache;
};

class CachePoco : public ICache< size_t, std::string >
{
public:
    CachePoco( size_t size )
        : m_capacity( size ), m_cache( size )
    {}
    ~CachePoco() = default;
    std::string* get( const size_t& key ) override
    {
        m_string = m_cache.get( key );
        return m_string.get();
    }
    void set( const size_t& key, const std::string& value ) override
    {
        m_cache.add( key, value );
    }
    std::string name() const override
    {
        return "Poco(" + std::to_string( capacity() ) + ")";
    }
    size_t capacity() const override
    {
        return m_capacity;
    }
    void clear() override
    {
        m_cache.clear();
    }
private:
    size_t m_capacity;
    Poco::SharedPtr< std::string > m_string;
    Poco::LRUCache< size_t, std::string > m_cache;
};

int main()
{
    std::unique_ptr<ICache< size_t, std::string >> ctc1K{
        new CacheTimedCached( size_t( 1000 ), std::chrono::milliseconds( 1 ) )
    };
    std::unique_ptr<ICache< size_t, std::string >> ctc10K{
        new CacheTimedCached( size_t(10000), std::chrono::milliseconds( 1 ) )
    };
    std::unique_ptr<ICache< size_t, std::string >> ctc1K10{
        new CacheTimedCached( size_t( 1000 ), std::chrono::milliseconds( 10 ) )
    };
    std::unique_ptr<ICache< size_t, std::string >> ctc10K10{
        new CacheTimedCached( size_t( 10000 ), std::chrono::milliseconds( 10 ) )
    };
    std::unique_ptr<ICache< size_t, std::string >> ctc1K100{
        new CacheTimedCached( size_t( 1000 ), std::chrono::milliseconds( 100 ) )
    };
    std::unique_ptr<ICache< size_t, std::string >> ctc10K100{
        new CacheTimedCached( size_t( 10000 ), std::chrono::milliseconds( 100 ) )
    };
    std::unique_ptr<ICache< size_t, std::string >> ctc1K1000{
    new CacheTimedCached( size_t( 1000 ), std::chrono::milliseconds( 1000 ) )
    };
    std::unique_ptr<ICache< size_t, std::string >> ctc10K1000{
        new CacheTimedCached( size_t( 10000 ), std::chrono::milliseconds( 1000 ) )
    };
    std::unique_ptr<ICache< size_t, std::string >> pocoLRU1K{
        new CachePoco( size_t( 1000 ) )
    };
    std::unique_ptr<ICache< size_t, std::string >> pocoLRU10K{
        new CachePoco( size_t( 10000 ) )
    };
    {
        /// Кэш размером 1000
        TestPerfomance test;
        test.PushCache( pocoLRU1K.get() );
        test.PushCache( ctc1K.get() );
        test.PushCache( ctc1K10.get() );
        test.PushCache( ctc1K100.get() );
        test.PushCache( ctc1K1000.get() );
        test.SetParam( 10000, 1000000 );

        test.Execute( std::cout );
    }
    {
        /// Кэш размером 10000
        TestPerfomance test;
        test.PushCache( pocoLRU10K.get() );
        test.PushCache( ctc10K.get() );
        test.PushCache( ctc10K10.get() );
        test.PushCache( ctc10K100.get() );
        test.PushCache( ctc10K1000.get() );
        test.SetParam( 10000, 1000000 );

        test.Execute( std::cout );
    }
    {
        /// Кэш размером 1000
        TestPerfomance test;
        test.PushCache( pocoLRU1K.get() );
        test.PushCache( ctc1K.get() );
        test.PushCache( ctc1K10.get() );
        test.PushCache( ctc1K100.get() );
        test.PushCache( ctc1K1000.get() );
        test.SetParam( 100000, 1000000 );

        test.Execute( std::cout );
    }
    {
        /// Кэш размером 10000
        TestPerfomance test;
        test.PushCache( pocoLRU10K.get() );
        test.PushCache( ctc10K.get() );
        test.PushCache( ctc10K10.get() );
        test.PushCache( ctc10K100.get() );
        test.PushCache( ctc10K1000.get() );
        test.SetParam( 100000, 1000000 );

        test.Execute( std::cout );
    }
    return 0;
}
