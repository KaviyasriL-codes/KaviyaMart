cmake_minimum_required(VERSION 3.20)

project(KaviyaMart)

# ============================================================
# KaviyaMart uses C++20
# ============================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# Dependencies
# ============================================================

find_package(Drogon CONFIG REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(OpenSSL REQUIRED)

# ============================================================
# libpqxx
# ============================================================

find_path(PQXX_INCLUDE_DIR
    NAMES pqxx/pqxx
    PATHS
        /usr/local/include
        /usr/include
    NO_DEFAULT_PATH
)

find_library(PQXX_LIBRARY
    NAMES pqxx
    PATHS
        /usr/local/lib
        /usr/lib/x86_64-linux-gnu
    NO_DEFAULT_PATH
)

if(NOT PQXX_INCLUDE_DIR OR NOT PQXX_LIBRARY)

    message(FATAL_ERROR
        "Could not find libpqxx.\n"
        "Include directory: ${PQXX_INCLUDE_DIR}\n"
        "Library: ${PQXX_LIBRARY}"
    )

endif()

message(STATUS "libpqxx include: ${PQXX_INCLUDE_DIR}")
message(STATUS "libpqxx library: ${PQXX_LIBRARY}")

# ============================================================
# Application
# ============================================================

add_executable(KaviyaMart

    main.cpp

    Controllers/AuthController.cpp
    Controllers/ProductController.cpp
    Controllers/OrderController.cpp
    Controllers/AdminController.cpp

    Services/Database.cpp
)

# ============================================================
# Include directories
# ============================================================

target_include_directories(KaviyaMart PRIVATE

    ${CMAKE_CURRENT_SOURCE_DIR}

    ${PQXX_INCLUDE_DIR}
)

# ============================================================
# Libraries
# ============================================================

target_link_libraries(KaviyaMart PRIVATE

    Drogon::Drogon

    ${PQXX_LIBRARY}

    PostgreSQL::PostgreSQL

    OpenSSL::Crypto
)