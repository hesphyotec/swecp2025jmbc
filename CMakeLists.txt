cmake_minimum_required(VERSION 4.0)
project(swecp2025jmbc)

set(CMAKE_CXX_STANDARD 20)

add_executable(swecp2025jmbc
        include/sqlite3.c
        RecSysOOP.cpp
        app.cpp
        include/bcrypt.cpp
        include/blowfish.cpp
)

target_include_directories(swecp2025jmbc PRIVATE
        "C:/Users/blake/swecp2025jmbc/include/"
        "C:/Users/blake/swecp2025jmbc/asio/asio-1.36.0/include/"
        "C:/Users/blake/swecp2025jmbc/asio/asio-1.36.0/"
)

if(WIN32)
    target_compile_definitions(swecp2025jmbc PRIVATE
            ASIO_STANDALONE
            _WIN32_WINNT=0x0601
            WIN32_LEAN_AND_MEAN
            ASIO_HAS_WINDOWS_BCRYPT
    )
    target_link_libraries(swecp2025jmbc PRIVATE
            ws2_32
            wsock32
            bcrypt
            mswsock
    )
endif()