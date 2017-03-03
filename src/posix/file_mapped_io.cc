
#include "file_mapped_io.h"

#include <iostream>

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

FileMapper::FileMapper(const char* file_path) {
  mode_t file_handle_mode = O_CREAT; //Create only if exists, else use existing
  int file_handle = open(file_path,
                    O_RDWR,
                    file_handle_mode);

  if (file_handle == -1) {
    std::cerr << "File handle creation failed";
  } else {
    handle.file_handle = file_handle;
  }

  struct stat file_attributes;
  if (stat(file_path, &file_attributes) != 0) {
    handle.file_size = 0;
  } else {
    handle.file_size = static_cast<u64>(file_attributes.st_size);
  }
}

FileMapper::~FileMapper() {
  if (handle.file_handle != -1) {
    int cleanup_success = close(handle.file_handle);
    if (cleanup_success) {
      std::cerr << "Closing file handle failed.";
    }
  }
}

u64
FileMapper::getFileSize() {
  return handle.file_size; 
}

void*
FileMapper::map(u64 byte_offset, u32 length) {
  void* mapped_mem = mmap(nullptr,
                          length,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED,
                          handle.file_handle,
                          byte_offset);
  return mapped_mem != MAP_FAILED ? mapped_mem : nullptr; 
}

void FileMapper::unmap(void* mapped_mem, u32 length) {
  munmap(mapped_mem, length);
}

