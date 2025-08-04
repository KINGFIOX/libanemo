cmake_minimum_required(VERSION 3.12)

project(libanemo)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON) 

include_directories(SYSTEM "include")

file(GLOB_RECURSE "LIBVIO_SRCS" "src/libvio/*.cc")
file(GLOB_RECURSE "LIBCPU_SRCS" "src/libcpu/*.cc")
file(GLOB_RECURSE "LIBSDB_SRCS" "src/libsdb/*.cc")

add_library(anemo STATIC "${LIBCPU_SRCS}" "${LIBVIO_SRCS}" "${LIBSDB_SRCS}")
set_target_properties(anemo PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_executable(quick_start src/examples/quick_start.cc)
target_link_libraries(quick_start PRIVATE anemo)
add_executable(riscv_minimal src/examples/riscv_minimal.cc)
target_link_libraries(riscv_minimal PRIVATE anemo)
