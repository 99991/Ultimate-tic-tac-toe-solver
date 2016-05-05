#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

#define INF (1.0/0.0)
#define UNUSED(x) ((void)x)
#include <stdint.h>

#ifdef NDEBUG
#define assert(x)
#else
// gdb fails to catch regular assert on windows
#define assert(x) if (!(x)) (*(int*)0) = 1;
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define SIZE_TYPE u32
// like std::array, but operator[] is bounds-checked
template <typename T, u64 N>
struct Array {
    T values[N];

    typedef T* iterator;
    typedef const T* const_iterator;

          T&       operator [] (SIZE_TYPE i)       { assert(i < size()); return values[i]; }
    const T&       operator [] (SIZE_TYPE i) const { assert(i < size()); return values[i]; }
          T*       data        (           )       { return values; }
    const T*       data        (           ) const { return values; }
          T&       back        (           )       { return values[size() - 1]; }
    const T&       back        (           ) const { return values[size() - 1]; }
          iterator begin       (           )       { return data(); }
          iterator end         (           )       { return data() + size(); }
    const_iterator begin       (           ) const { return data(); }
    const_iterator end         (           ) const { return data() + size(); }
    SIZE_TYPE      size        (           ) const { return N; }
};

// fixed maximum size vector without dynamic memory allocation
template <typename T, u64 MAX_SIZE>
struct SmallVector {
    Array<T, MAX_SIZE> values;
    SIZE_TYPE n;

    typedef T* iterator;
    typedef const T* const_iterator;

          T&       operator [] (SIZE_TYPE i)       { assert(i < size()); return values[i]; }
    const T&       operator [] (SIZE_TYPE i) const { assert(i < size()); return values[i]; }
          T*       data        (           )       { return values.data(); }
    const T*       data        (           ) const { return values.data(); }
          T&       back        (           )       { return values[size() - 1]; }
    const T&       back        (           ) const { return values[size() - 1]; }
          iterator begin       (           )       { return data(); }
          iterator end         (           )       { return data() + size(); }
    const_iterator begin       (           ) const { return data(); }
    const_iterator end         (           ) const { return data() + size(); }
    SIZE_TYPE      size        (           ) const { return n; }
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

    void clear(){
        n = 0;
    }
};

u32 rd(){
    static u32 x = 0x12345678;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

template <typename ITERATOR>
void shuffle(ITERATOR a, ITERATOR b){
    u32 n = b - a;
    for (u32 i = 0; i < n - 1; i++){
        u32 j = rd() % (n - i);
        auto tmp = a[i];
        a[i] = a[j];
        a[j] = tmp;
    }
}


