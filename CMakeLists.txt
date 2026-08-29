cmake_minimum_required(VERSION 3.20)

project(KaviyaMart)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(OpenSSL REQUIRED)
add_executable(KaviyaMart
    main.cpp

    Controllers/AuthController.cpp
    Controllers/ProductController.cpp
    Controllers/OrderController.cpp

    Services/Database.cpp
)

target_include_directories(KaviyaMart PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(KaviyaMart PRIVATE
    Drogon::Drogon
    libpqxx::pqxx
    PostgreSQL::PostgreSQL
    OpenSSL::Crypto
)