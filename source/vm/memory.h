#ifndef MEMORY_H
#define MEMORY_H

#include <unordered_set>

#include "variant.h"

// Tagged pointer example from
// http://nikic.github.io/2012/02/02/Pointer-magic-for-efficient-dynamic-value-representations.html
template <typename T, int alignedTo> class TaggedPointer {
private:
  static_assert(alignedTo != 0 && ((alignedTo & (alignedTo - 1)) == 0),
                "Alignment parameter must be power of two");

  // save us some reinterpret_casts with a union
  union {
    T *asPointer;
    intptr_t asBits;
  };

public:
  // for 8 byte alignment tagMask = alignedTo - 1 = 8 - 1 = 7 = 0b111
  // i.e. the lowest three bits are set, which is where the tag is stored
  static const intptr_t tagMask = alignedTo - 1;

  // pointerMask is the exact contrary: 0b...11111000
  // i.e. all bits apart from the three lowest are set, which is where the
  // pointer is stored
  static const intptr_t pointerMask = ~tagMask;

  inline TaggedPointer(void *pointer, int tag = 0) {
    asPointer = static_cast<void **>(pointer);
    asBits |= tag;
  }

  inline TaggedPointer(T *pointer = 0, int tag = 0) { set(pointer, tag); }

  inline void set(T *pointer, int tag = 0) {
    // // make sure that the pointer really is aligned
    // assert((reinterpret_cast<intptr_t>(pointer) & tagMask) == 0);
    // // make sure that the tag isn't too large
    // assert((tag & pointerMask) == 0);

    asPointer = pointer;
    asBits |= tag;
  }

  inline T *getPointer() { return reinterpret_cast<T *>(asBits & pointerMask); }
  inline int getTag() { return asBits & tagMask; }
};

struct AllocatedMemory {
  TaggedPointer<void *, 8> data;

  AllocatedMemory(uint64_t size);
  ~AllocatedMemory();

  void *operator new(std::size_t n) throw(std::bad_alloc);
  void operator delete(void *p) throw();
};

static_assert(sizeof(AllocatedMemory) == sizeof(void *),
              "AllocatedMemory must be sizeof(void*)");

class Memory {
public:
  Memory(int64_t *stack_start, int64_t *memory_start);
  ~Memory();

  Variant *new_lang_array(int size);
  AllocatedMemory *allocate(int64_t size);

  void set_stack(int64_t *stack);
  void set_memory(int64_t *memory);

private:
  std::unordered_set<AllocatedMemory *> allocated;

  bool is_managed(AllocatedMemory *memory) const;
  void mark();
  void sweep();
  void gc();

  int64_t *stack_start;
  int64_t *stack;
  int64_t *memory_start;
  int64_t *memory;
};

extern "C" AllocatedMemory *Memory_allocate(Memory *memory, int64_t size);
extern "C" void Memory_set_stack(Memory *memory, int64_t *stack);
extern "C" void Memory_set_memory(Memory *memory, int64_t *mem);
extern "C" Variant *Memory_new_lang_array(Memory *memory, int size);

#endif // MEMORY_H
