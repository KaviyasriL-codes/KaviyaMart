#include "Database.h"

#include <cstdlib>
#include <stdexcept>
#include <string>

std::unique_ptr<pqxx::connection> Database::connect()
{
    const char* databaseUrl = std::getenv("DATABASE_URL");

    if (databaseUrl == nullptr || std::string(databaseUrl).empty())
    {
        throw std::runtime_error(
            "DATABASE_URL environment variable is not set."
        );
    }

    return std::make_unique<pqxx::connection>(databaseUrl);
}
