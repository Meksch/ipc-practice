cmake_minimum_required(VERSION 3.16)

project(ipc-practice VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

find_library(IPC_RT_LIBRARY rt)
set(IPC_PLATFORM_LIBS "")
if(IPC_RT_LIBRARY)
  list(APPEND IPC_PLATFORM_LIBS "${IPC_RT_LIBRARY}")
endif()

add_library(ipc_common STATIC
  src/packet.cpp
  src/checksum.cpp
  src/shared_memory.cpp
  src/mem_buffer.cpp
)
target_include_directories(ipc_common PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(ipc_common PUBLIC ${IPC_PLATFORM_LIBS})

add_executable(producer src/producer/main.cpp)
target_link_libraries(producer PRIVATE ipc_common ${IPC_PLATFORM_LIBS})

add_executable(consumer src/consumer/main.cpp)
target_link_libraries(consumer PRIVATE ipc_common ${IPC_PLATFORM_LIBS})
