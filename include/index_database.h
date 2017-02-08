
#include "file_mapped_io.h"

#include <cassert>

// TODO: This is error prone. 
//       Maintain HeaderData using something like __COUNTER__ to manage
//       the offsets consistently using the size of the HeaderData?
//       E.g.
//enum HeaderData {
//  FILE_INDENTIFIER = INCREASING_OFFSET(sizeof(u32)),
//  DATABASE_VERSION = INCREASING_OFFSET(sizeof(u32)),
//  FILES_COUNT      = INCREASING_OFFSET(sizeof(u32)),
//
//  TEST_COUNT       = INCREASING_OFFSET(sizeof(u64)),
//  NUMS_OFFSET      = INCREASING_OFFSET(sizeof(u64)),
//  etc...
//};
//
enum HeaderData {
  FILE_INDENTIFIER = 0x0, // 4 bytes
  DATABASE_VERSION = 0x4, // 4 bytes
  FILES_COUNT      = 0x8, // 4 bytes

  TEST_COUNT       = 0xc, // 8 bytes     
  TEST_OFFSET      = 0x14,  // 8 bytes
  // etc...
  HEADER_LAST      = TEST_OFFSET + 8
};

// Responsible for creating and maintaining the format and structure
// of the index database file
class DataBase {
public:
  DataBase(const char* file_path);
  DataBase(const DataBase& other) = delete;
  DataBase& operator=(const DataBase& other) = delete;

  ~DataBase();

  void load();
  void release();

  void build();
  void partialBuild();
  
  template <typename T>
  T readHeaderValue(HeaderData);

  template <typename T>
  void writeHeaderValue(HeaderData, T val);

private:
  FileMapper file_mapper;
  
  struct {
    size_t offset_to_begin;
    size_t map_len;
    void* map_begin;
    
  } current_map;
  
  void* file_begin;
};

// Template definitions

template <typename T>
T DataBase::readHeaderValue(HeaderData h_offset) {
  assert(file_begin != nullptr);
  
  unsigned char* val_ptr = reinterpret_cast<unsigned char*>(file_begin) + h_offset;
  return *reinterpret_cast<T*>(val_ptr);
};

template <typename T>
void DataBase::writeHeaderValue(HeaderData h_offset, T val) {
  assert(file_begin != nullptr);

  unsigned char* val_ptr = reinterpret_cast<unsigned char*>(file_begin) + h_offset;
  *reinterpret_cast<T*>(val_ptr) = val;
}

