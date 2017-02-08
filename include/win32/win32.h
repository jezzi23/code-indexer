#ifndef WIN32_H_
#define WIN32_H_

#include "Windows.h"

#include "types.h"

struct MapHandle {
  HANDLE file_handle;
  u64    file_size;
};

#endif // WIN32_H_
