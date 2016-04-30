#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

// gdb fails to catch regular assert on windows
#define assert(x) if (!(x)) (*(int*)0) = 1;

#define INF (1.0/0.0)
#define UNUSED(x) ((void)x)

typedef uint8_t u8;
typedef uint32_t u32;

// like std::array, but operator[] is bounds-checked
template <typename T, u32 N>
struct Array {
    T values[N];

    typedef T* iterator;
    typedef const T* const_iterator;

    size_t         size        (     ) const { return N; }
          T&       operator [] (u32 i)       { assert(i < size()); return values[i]; }
    const T&       operator [] (u32 i) const { assert(i < size()); return values[i]; }
          T*       data        (     )       { return values; }
    const T*       data        (     ) const { return values; }
          T&       back        (     )       { return values[size() - 1]; }
    const T&       back        (     ) const { return values[size() - 1]; }
          iterator begin       (     )       { return data(); }
          iterator end         (     )       { return data() + size(); }
    const_iterator begin       (     ) const { return data(); }
    const_iterator end         (     ) const { return data() + size(); }
};

// fixed maximum size vector without dynamic memory allocation
template <typename T, u32 MAX_SIZE>
struct SmallVector {
    Array<T, MAX_SIZE> values;
    u32 n;

    typedef T* iterator;
    typedef const T* const_iterator;

    size_t         size        (     ) const { return n; }
          T&       operator [] (u32 i)       { assert(i < size()); return values[i]; }
    const T&       operator [] (u32 i) const { assert(i < size()); return values[i]; }
          T*       data        (     )       { return values.data(); }
    const T*       data        (     ) const { return values.data(); }
          T&       back        (     )       { return values[size() - 1]; }
    const T&       back        (     ) const { return values[size() - 1]; }
          iterator begin       (     )       { return data(); }
          iterator end         (     )       { return data() + size(); }
    const_iterator begin       (     ) const { return data(); }
    const_iterator end         (     ) const { return data() + size(); }
    bool           empty       (     ) const { return size() == 0; }

    SmallVector(): n(0){}

    void push_back(const T &value){
        assert(n < MAX_SIZE);
        values[n++] = value;
    }

    void pop_back(){
        assert(n > 0);
        n--;
    }
};

u32 rd(){
    static u32 x = 0x12345678;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}
