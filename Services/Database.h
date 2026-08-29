#pragma once

#include <memory>
#include <pqxx/pqxx>

class Database {
public:
    static std::unique_ptr<pqxx::connection> connect();
};