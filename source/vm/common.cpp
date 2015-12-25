#include "common.h"

#include <string>
#include <iostream>
#include <cstdlib> // exit

#ifndef _WIN32
#include <execinfo.h>
#include <cstdlib>
/* Obtain a backtrace and print it to stdout. */
void print_trace() {
  void *array[10];
  size_t size = backtrace(array, 10);
  char **strings = backtrace_symbols(array, size);

  for (size_t i = 0; i < size; i++)
    std::cerr << strings[i] << std::endl;

  free(strings);
}
#else
void print_trace() {}
#endif

void raise_error(const std::string &message) {
  std::cerr << message << std::endl;
  print_trace();
  exit(1);
}
