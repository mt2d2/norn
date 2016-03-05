cmake_minimum_required(VERSION 3.2.3)

include(ExternalProject)

project(norn)

option(JIT "enable trace JIT" ON)
if(NOT JIT)
  add_definitions(-DNOJIT)
endif(NOT JIT)

if(JIT)
  set(asmjit_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/asmjit_external)
  set(asmjit_INSTALL_DIR ${asmjit_PREFIX})
  set(asmjit_BINARY_DIR ${asmjit_PREFIX})
  set(asmjit_ARCHIVE ${asmjit_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}asmjit${CMAKE_STATIC_LIBRARY_SUFFIX})
  set(asmjit_CMAKE_ARGS -DASMJIT_STATIC=1 -DCMAKE_INSTALL_PREFIX=${asmjit_INSTALL_DIR})
  ExternalProject_Add(asmjit_external
    PREFIX ${asmjit_PREFIX}
    INSTALL_DIR ${asmjit_INSTALL_DIR}
    BINARY_DIR ${asmjit_BINARY_DIR}
    BUILD_BYPRODUCTS ${asmjit_ARCHIVE}
    GIT_REPOSITORY https://github.com/kobalicek/asmjit.git
    GIT_TAG master
    CMAKE_ARGS ${asmjit_CMAKE_ARGS}
    UPDATE_COMMAND "") # skip update on every build, nuke if desired
endif(JIT)

add_custom_command(
  OUTPUT lang_norn.h
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/lang.norn lang.norn
  COMMAND xxd -i lang.norn lang_norn.h
  DEPENDS lang.norn)

set(lang_src
  lang_norn.h
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

# we need to grab the generated lang_norn.h
target_include_directories(norn PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED)

if(Boost_FOUND)
    target_include_directories(norn PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(norn ${Boost_LIBRARIES})
endif()

if(JIT)
  add_library(asmjit STATIC IMPORTED)
  add_dependencies(norn asmjit_external)
  set_property(TARGET asmjit PROPERTY IMPORTED_LOCATION ${asmjit_ARCHIVE})
  target_link_libraries(norn asmjit)
  add_dependencies(norn asmjit_external)
endif(JIT)
