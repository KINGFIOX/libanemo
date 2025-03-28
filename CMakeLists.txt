cmake_minimum_required(VERSION 3.12)

project(libcpu)

set(libvioDir $ENV{LIBVIO_HOME} CACHE STRING "Path to libvio")
add_subdirectory(${libvioDir} libvio.dir)
include_directories(${libvioDir}/include)

include_directories(SYSTEM "include")

file(GLOB_RECURSE "NEMUCORE_SRCS" "src/rv32i_nemu/*.cc")

add_library(nemucore STATIC "${NEMUCORE_SRCS}")
set_target_properties(nemucore PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(difftest STATIC src/difftest.cc)
set_target_properties(difftest PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_executable(nemu-minimal src/main.cc)
target_link_libraries(nemu-minimal PRIVATE nemucore vio)
