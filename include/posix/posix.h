
#ifndef POSIX_H_
#define POSIX_H_

#include "types.h"

struct MapHandle {
  int file_handle;
  u64 file_size;
};

#endif // POSIX_H_

