
#ifndef UTILS_H_
#define UTILS_H_

#include "types.h"

// Only works for little endian architectures
inline void unpack(u64 in_val, u32& out_high_order, u32& out_low_order) {
  out_low_order = static_cast<u32>(in_val);
  out_high_order = static_cast<u32>(in_val >> 32);
}

#endif // UTILS_H_
