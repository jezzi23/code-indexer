
#ifndef UTILS_H_
#define UTILS_H_

#include "types.h"

// Only works for little endian architectures
inline void unpack(u64 in_val, u32& out_high_order, u32& out_low_order) {
  out_low_order = static_cast<u32>(in_val);
  out_high_order = static_cast<u32>(in_val >> 32);
}

// For easy type deduced allocations
// e.g. T* a; a = type_deduced_new(a, ...);
template <class T, class... Arg>
void type_deduced_new(T* &p, Arg &&... arg) {
  p = new T(std::forward<Arg>(arg)...);
}

#endif // UTILS_H_
