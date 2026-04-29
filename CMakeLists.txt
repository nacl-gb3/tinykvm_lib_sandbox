cmake_minimum_required(VERSION 3.10)
project(tinykvm_lib_sandbox CXX C)

option(FLTO "Link-time optimizations" OFF)
option(SANITIZE "Enable address and ub sanitizers" OFF)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -O2 -ggdb3")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (FLTO)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto=thin")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
endif()
if (SANITIZE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
endif()

add_subdirectory(lib tinykvm)

target_compile_definitions(tinykvm PUBLIC
	TINYKVM_ARCH_${TINYKVM_ARCH}=1
	TINYKVM_ARCH="${TINYKVM_ARCH}")

add_executable(shmdemo
	src/shm.cpp
	src/sandbox.cpp
)
target_link_libraries(shmdemo tinykvm)

add_executable(helloshm
	src/hello_shm.c
)
# target_link_libraries(helloshm "-static")
