#pragma once

#include <cstdint>

#include <atomic>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <map>
#include <unordered_map>
#include <optional>
#include <algorithm>
#include <chrono>
#include <utility>
#include <vector>

// Вспомогательный класс, который позволяет усыплять и будить поток
class Sleeper
{
    std::mutex m_lock;
    std::condition_variable m_cond;
    std::atomic_bool flag = ATOMIC_VAR_INIT( false );
public:
    template< class Rep, class Period >
    void sleep( const std::chrono::duration< Rep, Period >& relTime )
    {
        std::unique_lock ul( m_lock );
        m_cond.wait_for( ul, relTime, [&](){ return flag.load(); } );
        flag = false;
    }

    void wakeUp()
    {
        flag = true;
        m_cond.notify_one();
    }
};

template< class K, class T >
class TimedCache
{
    using tick = std::chrono::nanoseconds;
    using timer = std::chrono::high_resolution_clock::time_point;
public:
    template< class Rep, class Period >
    TimedCache( size_t size, const std::chrono::duration< Rep, Period >& relTime )
        : m_maxDTime( std::chrono::duration_cast<std::chrono::nanoseconds>( relTime ) ),
        m_size( size )
    {
        startClenaer();
    }
    TimedCache( const TimedCache& ) = delete;
    TimedCache& operator = ( const TimedCache& ) = delete;
    ~TimedCache()
    {
        stopCleaner();
    }

    std::optional< T > get( const K& key )
    {
        std::lock_guard lg( m_lock );
        // Выполняем поиск
        auto iter = m_data.find( key );
        if( iter == m_data.end() )
            return std::optional< T >();

        auto dt =
            std::chrono::duration_cast<tick>(
                getCurrTime() - iter->second.second );

        T data = std::move( iter->second.first );
        // Да, удаляем
        m_key.erase( iter->second.second );
        m_data.erase( iter );

        // Пройденно время превышает максимальное?
        if( dt >= m_maxDTime )
        {
            if( m_data.size() == 0 )
                stopCleaner();
            return std::optional< T >();
        }

        auto currTime = getCurrTime();
        auto [obj, state] = m_data.insert( std::make_pair( key, std::make_pair( std::move( data) , currTime ) ) );
        m_key.insert( std::make_pair( currTime, key ) );

        // Все ок, возращаем объект
        return std::optional< T >( obj->second.first );
    }
    void set( const K& key, const T& value )
    {
        bool wakeup = false;
        {
            std::lock_guard lg( m_lock );
           // Чистим, если размер кэша превышен
            if( m_data.size() == m_size )
            {
                // В m_key у нас отсортированы по времени, берем первый.
                auto old = m_key.begin();
                m_data.erase( old->second );
                m_key.erase( old );
            }
            auto curTime = getCurrTime();
            m_key[curTime] = key;
            m_data[key] = std::make_pair( value, curTime );
            if( m_data.size() == 1 )
                wakeup = true;
        }
        // Появились новые объекты, надо разбудить поток
        if( wakeup )
            wakeUp();
    }
    size_t size() const
    {
        return m_data.size();
    }
    size_t capacity() const
    {
        return m_size;
    }
    void clear()
    {
        std::lock_guard lg( m_lock );
        m_key.clear();
        m_data.clear();
    }
private:
    void wakeUp()
    {
        m_sleeper.wakeUp();
    }
    timer getCurrTime() const
    {
        return std::chrono::high_resolution_clock::now();
    }
    void startClenaer()
    {
        m_working = true;
        m_cleaner = std::thread( &TimedCache<K, T>::updateCached, this );
    }
    void stopCleaner()
    {
        m_working = false;
        m_sleeper.wakeUp();
        if( m_cleaner.joinable() )
            m_cleaner.join();
    }
    void updateCached()
    {
        tick timeForSleep;
        while( m_working )
        {
            {
                timeForSleep = std::chrono::duration_cast< tick >( std::chrono::hours( 10 ) );
                std::lock_guard lg( m_lock );
                auto curTime = getCurrTime();
                // если ключей нет, засыпаем на большой период
                while( !m_key.empty() )
                {
                    auto first = m_key.begin();
                    auto&[time, key] = *first;
                    auto dt = std::chrono::duration_cast<tick>( curTime - time );
                    if( dt >= m_maxDTime )
                    {
                        m_data.erase( key );
                        m_key.erase( first );
                    }
                    else
                    {
                        timeForSleep = m_maxDTime - dt;
                        // все следующие объекты "достаточно свежие"
                        break;
                    }
                }
            }
            // спим, до следующего "протукшего" объекта или надолго, т.к. их нету
            m_sleeper.sleep( timeForSleep );
        }
    }

    Sleeper m_sleeper;
    std::thread m_cleaner;
    std::atomic_bool m_working = ATOMIC_VAR_INIT(false);

    mutable std::mutex m_lock;
    size_t m_size = 0;
    tick m_maxDTime{};
    std::unordered_map< K, std::pair< T, timer > > m_data;
    std::map< timer, K > m_key;
};
