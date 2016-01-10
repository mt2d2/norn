#ifndef FRAME_H
#define FRAME_H

#include <cstdint>

class Block;

struct Frame {
  Frame() {}
  Frame(uint64_t ip, Block *block) : ip(ip), block(block) {}
  uint64_t ip;
  Block *block;
};

#endif
