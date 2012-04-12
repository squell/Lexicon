#pragma once

// C++ meta-program to determine the amount of padding (at the end) 
// only works for non-pod types T

template<class T, size_t N=sizeof(size_t)-1> 
struct padding {
    struct mix : T { char padding[N]; };
    static size_t const amount 
     = sizeof(T)==sizeof(mix)? N : padding<T,N-1>::amount;

    operator size_t() const { return amount; }
};

template<class T> struct padding<T,0> { static size_t const amount = 0; };
