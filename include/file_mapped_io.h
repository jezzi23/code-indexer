
#ifndef FILE_MAPPED_IO_
#define FILE_MAPPED_IO_

#ifdef _WIN32
#include "win32/win32.h"

#elif __unix__
#include <unistd.h>
#ifdef _POSIX_VERSION
#include "posix/posix.h"
#endif // _POSIX_VERSION

#else // __unix__
#error Platform not supported.
#endif

#include "types.h"

class FileMapper {
public:
  FileMapper(const char* file_path);
  FileMapper(const FileMapper& other) = delete;
  FileMapper& operator=(const FileMapper& other) = delete;

  ~FileMapper();

  u64 getFileSize();
  void* map(u64 byte_offset, u32 length);
  void unmap(void* mapped_mem, u32 length);
private:
  MapHandle handle; // File maps are done via this handle
};

#endif // FILE_MAPPED_IO_
