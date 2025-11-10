cmake_minimum_required(VERSION 3.28)
project(swecp2025jmbc)

set(CMAKE_EXE_LINKER_FLAGS "-static")
set(CMAKE_CXX_STANDARD 20)

if(WIN32 AND MINGW)
    set(CMAKE_CXX_FLAGS_DEBUG "-O0")  # No debug symbols at all
    set(CMAKE_C_FLAGS_DEBUG "-O0")
endif()

set(CMAKE_BUILD_TYPE Release)

add_executable(swecp2025jmbc
        include/sqlite3.c
        app.cpp
        include/bcrypt.cpp
        include/blowfish.cpp
#        CW2V2.cpp
#        include/Word2Vec-master/main.cpp
)

target_include_directories(swecp2025jmbc PRIVATE
        include/
        asio/asio-1.36.0/include/
        asio/asio-1.36.0/
        armadillo-15.2.1/
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