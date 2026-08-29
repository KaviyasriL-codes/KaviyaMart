#include <drogon/drogon.h>
#include "Services/Database.h"

#include <iostream>

int main()
{
    try
    {
        auto db = Database::connect();

        if (db && db->is_open())
        {
            std::cout << "PostgreSQL connection successful!"
                      << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Database connection failed: "
                  << e.what()
                  << std::endl;
    }

    drogon::app()
        .addListener("127.0.0.1", 8080)
        .setDocumentRoot("C:/capstonekaviya/frontend")
        .run();

    return 0;
}