
#include "file_mapped_io.h"

#include <iostream>

#include "Windows.h"

#include "types.h"
#include "utils.h"

internal_ const u64 file_fill_size = 1024ULL;

FileMapper::FileMapper(const char* file_path) {
  u64 file_size;
  HANDLE file_handle = CreateFile(file_path,
                                  GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ, 
                                  NULL,
                                  OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);

  if (file_handle == INVALID_HANDLE_VALUE) {
    std::cerr << "File handle creation failed: ";
    std::cerr << GetLastError() << std::endl;
  } else if (ERROR_ALREADY_EXISTS == GetLastError()) {
    LARGE_INTEGER file_size_tmp;
    GetFileSizeEx(file_handle, &file_size_tmp);
    file_size = file_size_tmp.QuadPart; 
  } else {
    file_size = 0; 
  }

  u32 low_order_val, high_order_val;

  if (file_size == 0) {
    unpack(file_fill_size, high_order_val, low_order_val);
  } else {
    unpack(file_size, high_order_val, low_order_val);
  }

  HANDLE map_handle = CreateFileMapping(file_handle,
                                        NULL,
                                        PAGE_READWRITE,
                                        high_order_val,
                                        low_order_val,
                                        NULL);

  if (map_handle == NULL) {
    std::cerr << "File map handle creation failed: ";
    std::cerr << GetLastError() << std::endl;
  }
  //TODO: Can file_handle be closed here?
  handle = { map_handle, file_size ? file_size : file_fill_size };
}

FileMapper::~FileMapper() {
  CloseHandle(handle.file_handle);
}

u64 FileMapper::getFileSize() {
  return handle.file_size;
}

void* FileMapper::map(u64 byte_offset, u32 length) {
  u32 low_order;
  u32 high_order;
  unpack(byte_offset, high_order, low_order);

  void* result = MapViewOfFile(handle.file_handle,
                               FILE_MAP_WRITE,
                               high_order,
                               low_order,
                               length);

  if (result == NULL) {
    std::cerr << "Mapping file to memory failed: ";
    std::cerr << GetLastError() << std::endl;
  }
  return result;
}

void FileMapper::unmap(void* mapped_mem) {
  BOOL success = UnmapViewOfFile(mapped_mem);
  if (!success) {
    std::cerr << "Unmapping file from memory failed: ";
    std::cerr << GetLastError() << std::endl;
  }
}

