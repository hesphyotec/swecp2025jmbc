cmake_minimum_required(VERSION 4.0)
project(swecp2025jmbc)

set(CMAKE_CXX_STANDARD 20)

add_executable(swecp2025jmbc
        "C:/Users/blake/sqlite/sqlite3.c"
        RecSysOOP.cpp
)

target_include_directories(swecp2025jmbc PRIVATE "C:/Users/blake/sqlite")
target_include_directories(swecp2025jmbc PRIVATE "C:/Users/blake/rapidjson/rapidjson-1.1.0/include/")
target_include_directories(swecp2025jmbc PRIVATE "C:/Users/blake/swecp2025jmbc/crow/crow-master/include/")
target_include_directories(swecp2025jmbc PRIVATE "C:/Users/blake/swecp2025jmbc/asio/asio-1.36.0/include/")
target_link_libraries(swecp2025jmbc PRIVATE ws2_32)
target_include_directories(swecp2025jmbc PRIVATE "C:/Users/blake/swecp2025jmbc/bgezdb.h")