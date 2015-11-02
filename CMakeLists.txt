cmake_minimum_required(VERSION 3.1)

project(norn)

option(JIT "enable method JIT" ON)
if(NOT JIT)
  add_definitions(-DNOJIT)
endif(NOT JIT)

set(lang_src
  source/lexer.cpp
  source/parser.cpp
  source/type.cpp
  source/tree.cpp
  source/generate.cpp
  source/main.cpp)
set(vm_src
  source/vm/common.cpp
  source/vm/opcode.cpp
  source/vm/instruction.cpp
  source/vm/block.cpp
  source/vm/program.cpp
  source/vm/optimizer.cpp
  source/vm/memory.cpp
  source/vm/machine.cpp
  source/vm/ir.cpp
  source/vm/trace.cpp)
add_executable(norn ${vm_src} ${lang_src})
set_property(TARGET norn PROPERTY CXX_STANDARD 11)

if(JIT)
  add_subdirectory(source/vm/AsmJit)
  target_link_libraries(norn asmjit)
endif(JIT)
