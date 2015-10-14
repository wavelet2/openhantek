#pragma once

#include <vector>

template <class T>
std::vector<T>& operator<<(std::vector<T>& v, T x) {
    v.push_back(x);
    return v;
}
