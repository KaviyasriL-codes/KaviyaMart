#include "Database.h"

std::unique_ptr<pqxx::connection> Database::connect()
{
    const std::string connectionString =
        "host=localhost "
        "port=5432 "
        "dbname=capstone_kaviyadb "
        "user=postgres "
        "password=kali";

    return std::make_unique<pqxx::connection>(connectionString);
}