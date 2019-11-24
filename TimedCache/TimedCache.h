#pragma once

#include <cstdint>

#include <map>
#include <optional>
#include <algorithm>
#include <chrono>
#include <utility>
#include <vector>

template< class K, class T >
class TimedCache
{
    using nsec = uint64_t;
    using timer = std::chrono::high_resolution_clock::time_point;
public:
    template< class Rep, class Period >
    TimedCache( size_t size, const std::chrono::duration< Rep, Period >& relTime )
        : m_maxDTime( std::chrono::duration_cast<std::chrono::nanoseconds>( relTime ).count() ),
        m_size( size )
    {
    }
    TimedCache( const TimedCache& ) = delete;
    TimedCache& operator = ( const TimedCache& ) = delete;

    std::optional< T > get( const K& key )
    {
        // Выполняем поиск
        auto iter = m_data.find( key );
        if( iter == m_data.end() )
            return std::optional< T >();

        nsec dt =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                getCurrTime() - iter->second.second
                ).count();
        // Пройденно время превышает максимальное?
        if( dt >= m_maxDTime )
        {
            // Да, удаляем, возращаем nil
            m_key.erase( iter->second.second );
            m_data.erase( iter );
            return std::optional< T >();
        }

        // Все ок, возращаем объект
        return std::optional< T >( iter->second.first );
    }
    void set( const K& key, const T& value )
    {
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
        m_key.clear();
        m_data.clear();
    }
private:
    timer getCurrTime() const
    {
        return std::chrono::high_resolution_clock::now();
    }

    size_t m_size = 0;
    nsec m_maxDTime{};
    std::map< K, std::pair< T, timer > > m_data;
    std::map< timer, K > m_key;
};
