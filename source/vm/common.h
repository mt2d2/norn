#ifndef COMMON_H
#define COMMON_H

#include <string>

// This one provides the compiler about branch hints, so it
// keeps the normal case fast. From Rubinius.
#ifdef __GNUC__
#define likely(x) __builtin_expect((long int)(x), 1)
#define unlikely(x) __builtin_expect((long int)(x), 0)
#else
#define likely(x) x
#define unlikely(x) x
#endif

void raise_error(const std::string &message);

#endif
