#ifndef UTILS_PAIR_H
#define UTILS_PAIR_H

#include <utility>

template<typename T1, typename T2>
class Pair {

public:

    T1 first;
    T2 second;

    Pair() = default;

    template<typename U1, typename U2>
    Pair(U1 &&a, U2 &&b) :
            first(std::forward<U1>(a)), second(std::forward<U2>(b)) {}
};

#endif
