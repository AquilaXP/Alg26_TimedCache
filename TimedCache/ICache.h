#pragma once

template< class K, class T >
class ICache
{
public:
    virtual ~ICache() = default;
    virtual void set( const K& key, const T& value ) = 0;
    virtual T* get( const K& key ) = 0;
    virtual std::string name() const = 0;
    virtual size_t capacity() const = 0;
    virtual void clear() = 0;
};
