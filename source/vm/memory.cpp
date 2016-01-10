#include "memory.h"

#include <cstdlib>

#include "common.h"
#include "variant.h"
#include "frame.h"

#define GC_THRESHOLD 100

AllocatedMemory::AllocatedMemory(uint64_t size)
    : data(TaggedPointer<void *, 8>(malloc(size), 0)) {}

AllocatedMemory::~AllocatedMemory() { free(this->data.getPointer()); }

void *AllocatedMemory::operator new(std::size_t n) throw() {
  return malloc(n);
}

void AllocatedMemory::operator delete(void *p) throw() { free(p); }

Memory::Memory(int64_t *stack_start, int64_t *memory_start)
    : allocated(std::unordered_set<AllocatedMemory *>()),
      stack_start(stack_start), stack(nullptr), memory_start(memory_start),
      memory(nullptr) {}

Memory::~Memory() {
  // free any unfreed memory at end
  for (const auto *elem : allocated)
    delete elem;
}

AllocatedMemory *Memory::allocate(int64_t size) {
  if (allocated.size() == GC_THRESHOLD)
    gc();

  const auto ret = this->allocated.insert(new AllocatedMemory(size));
  if (!ret.second)
    raise_error("allocation failure, not unique");

  return *ret.first;
}

void Memory::set_stack(int64_t *stack) { this->stack = stack; }

void Memory::set_memory(int64_t *memory) { this->memory = memory; }

Variant *Memory::new_lang_array(int64_t size) {
  size += 1;

  // Variant* ret = reinterpret_cast<Variant*>(this->allocate(size *
  // sizeof(Variant)).data);
  Variant *ret = new Variant[size];
  ret[0].l = size;

  return ret;
}

bool Memory::is_managed(AllocatedMemory *memory) const {
  return allocated.find(memory) != allocated.end();
}

void Memory::mark() {
  // check stack
  for (; stack != stack_start; --stack) {
    auto *allocd = reinterpret_cast<AllocatedMemory *>(*stack);
    if (is_managed(allocd))
      allocd->data.set(allocd->data.getPointer(), true);
  }

  // check memory
  for (; memory != memory_start; --memory) {
    auto *allocd = reinterpret_cast<AllocatedMemory *>(*memory);
    if (is_managed(allocd))
      allocd->data.set(allocd->data.getPointer(), true);
  }
}

void Memory::sweep() {
  for (auto it = allocated.begin(); it != allocated.end();) {
    auto *allocd = *it;

    if (!allocd->data.getTag()) {
      delete allocd;
      it = allocated.erase(it);
    } else {
      allocd->data.set(allocd->data.getPointer(), false);
      ++it;
    }
  }
}

void Memory::gc() {
  mark();
  sweep();
}

AllocatedMemory *Memory_allocate(Memory *memory, int64_t size) {
  return memory->allocate(size);
}

void Memory_set_stack(Memory *memory, int64_t *stack) {
  memory->set_stack(stack);
}

void Memory_set_memory(Memory *memory, int64_t *mem) {
  memory->set_memory(mem);
}

Variant *Memory_new_lang_array(Memory *memory, int size) {
  return memory->new_lang_array(size);
}
