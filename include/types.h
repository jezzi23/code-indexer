
#ifndef TYPES_H_
#define TYPES_H_

#include <cstdint>

// Static has way too many meanings in different contexts
// Why not use "internal_" to imply internal linkage?
#define internal_ static

typedef std::int8_t   s8;
typedef std::int16_t  s16;
typedef std::int32_t  s32;
typedef std::int64_t  s64;

typedef std::uint8_t  u8;
typedef std::uint16_t u16;
typedef std::uint32_t u32;
typedef std::uint64_t u64;

#endif // TYPES_H_
