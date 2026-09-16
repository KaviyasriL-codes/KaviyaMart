#include <drogon/drogon.h>
#include "Services/Database.h"

#include <iostream>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

int main()
{
    // =====================================================
    // Test PostgreSQL connection
    // =====================================================

    try
    {
        auto db = Database::connect();

        if (db && db->is_open())
        {
            std::cout
                << "PostgreSQL connection successful!"
                << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "Database connection failed: "
            << e.what()
            << std::endl;
    }


    // =====================================================
    // Drogon application
    // =====================================================

    auto& app = drogon::app();


    // =====================================================
    // Frontend document root
    // =====================================================

    app.setDocumentRoot(
        "C:/capstonekaviya/frontend"
    );


    // =====================================================
    // Serve uploaded product images
    //
    // Example:
    // /uploads/product_123.webp
    //
    // Actual folder:
    // C:/capstonekaviya/frontend/uploads/
    // =====================================================

    app.registerHandler(
        "/uploads/{1}",

        [](
            const drogon::HttpRequestPtr& req,
            std::function<void(
                const drogon::HttpResponsePtr&
            )>&& callback,

            const std::string& filename
        )
        {
            const fs::path uploadDirectory =
                "C:/capstonekaviya/frontend/uploads";


            // -------------------------------------------------
            // Security check
            // -------------------------------------------------

            if (filename.find("..") != std::string::npos ||
                filename.find('/') != std::string::npos ||
                filename.find('\\') != std::string::npos)
            {
                auto response =
                    drogon::HttpResponse::newHttpResponse();

                response->setStatusCode(
                    drogon::k403Forbidden
                );

                response->setBody("Forbidden");

                callback(response);
                return;
            }


            // -------------------------------------------------
            // Build image path
            // -------------------------------------------------

            const fs::path imagePath =
                uploadDirectory / filename;


            // -------------------------------------------------
            // Check file exists
            // -------------------------------------------------

            if (!fs::exists(imagePath) ||
                !fs::is_regular_file(imagePath))
            {
                auto response =
                    drogon::HttpResponse::newHttpResponse();

                response->setStatusCode(
                    drogon::k404NotFound
                );

                response->setBody(
                    "Image not found"
                );

                callback(response);
                return;
            }


            // -------------------------------------------------
            // Send image file
            // -------------------------------------------------

            auto response =
                drogon::HttpResponse::newFileResponse(
                    imagePath.string()
                );


            // -------------------------------------------------
            // Set content type based on extension
            // -------------------------------------------------

            std::string extension =
                imagePath.extension().string();

            if (extension == ".jpg" ||
                extension == ".jpeg")
            {
                response->setContentTypeCode(
                    drogon::CT_IMAGE_JPG
                );
            }
            else if (extension == ".png")
            {
                response->setContentTypeCode(
                    drogon::CT_IMAGE_PNG
                );
            }
            else if (extension == ".webp")
            {
                response->setContentTypeString(
                    "image/webp"
                );
            }


            callback(response);
        }
    );


    // =====================================================
    // Start server
    // =====================================================

    app.addListener(
        "127.0.0.1",
        8080
    );


    std::cout
        << "KaviyaMart server starting..."
        << std::endl;

    std::cout
        << "Open: http://127.0.0.1:8080"
        << std::endl;


    app.run();

    return 0;
}