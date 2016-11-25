cmake_minimum_required(VERSION 2.8)
project(Correct)
include(CheckLibraryExists)
include(CheckIncludeFiles)
include(CheckCXXSourceCompiles)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -O0 -march=native -fsanitize=address")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-no_pie,")
else()
  if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
    else()
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
  endif()
  if(CMAKE_BUILD_TYPE STREQUAL "Profiling")
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g3")
  endif()
endif()

find_library(FEC fec)
CHECK_LIBRARY_EXISTS(FEC dotprod "" HAVE_LIBFEC)
check_cxx_source_compiles("
    #include <x86intrin.h>
    int main() {
      __m128i vec;
      return 0;
    }" HAVE_SSE)

if(HAVE_SSE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse4.2")
endif()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -std=c99 -Wpedantic -Wall")
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

include_directories(${CMAKE_SOURCE_DIR}/include)
add_subdirectory(src)

set(INSTALL_HEADERS "${CMAKE_BINARY_DIR}/include/correct.h")

add_custom_target(correct-h ALL COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/include/correct.h ${CMAKE_BINARY_DIR}/include/correct.h)

if(HAVE_SSE)
  set(correct_obj_files $<TARGET_OBJECTS:correct-reed-solomon> $<TARGET_OBJECTS:correct-convolutional> $<TARGET_OBJECTS:correct-convolutional-sse>)
  set(INSTALL_HEADERS ${INSTALL_HEADERS} ${CMAKE_BINARY_DIR}/include/correct-sse.h)
  add_custom_target(correct-sse-h ALL COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/include/correct-sse.h ${CMAKE_BINARY_DIR}/include/correct-sse.h)
  add_definitions(-DHAVE_SSE=1)
else()
  set(correct_obj_files $<TARGET_OBJECTS:correct-reed-solomon> $<TARGET_OBJECTS:correct-convolutional>)
endif()
add_library(correct SHARED ${correct_obj_files})
add_library(correct_static ${correct_obj_files})
set_target_properties(correct_static PROPERTIES OUTPUT_NAME "correct")

add_subdirectory(util)
add_subdirectory(tests)
add_subdirectory(tools)
# add_subdirectory(benchmarks)

install(TARGETS correct correct_static
        DESTINATION lib)
install(FILES ${INSTALL_HEADERS} DESTINATION "${CMAKE_INSTALL_PREFIX}/include")

add_library(fec_shim_static EXCLUDE_FROM_ALL src/fec_shim.c ${correct_obj_files})
set_target_properties(fec_shim_static PROPERTIES OUTPUT_NAME "fec")
add_library(fec_shim_shared SHARED EXCLUDE_FROM_ALL src/fec_shim.c ${correct_obj_files})
set_target_properties(fec_shim_shared PROPERTIES OUTPUT_NAME "fec")
add_custom_target(fec-shim-h COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/include/fec_shim.h ${CMAKE_BINARY_DIR}/include/fec.h)
add_custom_target(shim DEPENDS fec_shim_static fec_shim_shared fec-shim-h)

install(TARGETS fec_shim_static fec_shim_shared
        DESTINATION lib
        OPTIONAL)
install(FILES ${CMAKE_BINARY_DIR}/include/fec.h DESTINATION "${CMAKE_INSTALL_PREFIX}/include" OPTIONAL)
