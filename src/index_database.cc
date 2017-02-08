
#include "index_database.h"


DataBase::DataBase(const char* file_path) : file_mapper(file_path) {
  load();
}

DataBase::~DataBase() {
  release();
}

void DataBase::load() {
  file_begin = file_mapper.map(0, HEADER_LAST);
}

void DataBase::release() {
  file_mapper.unmap(file_begin);
}

