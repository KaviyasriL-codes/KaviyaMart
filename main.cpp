#include <drogon/drogon.h>
#include "Services/Database.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
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

        return 1;
    }


    // =====================================================
    // Configuration from environment variables
    // =====================================================

    const char* frontendEnv =
        std::getenv("FRONTEND_DIR");

    const char* uploadEnv =
        std::getenv("UPLOAD_DIR");

    const char* portEnv =
        std::getenv("PORT");


    std::string frontendDirectory =
        (frontendEnv != nullptr && std::string(frontendEnv).size() > 0)
            ? frontendEnv
            : "frontend";


    std::string uploadDirectory =
        (uploadEnv != nullptr && std::string(uploadEnv).size() > 0)
            ? uploadEnv
            : frontendDirectory + "/uploads";


    int port = 8080;

    if (portEnv != nullptr && std::string(portEnv).size() > 0)
    {
        try
        {
            port = std::stoi(portEnv);
        }
        catch (...)
        {
            std::cerr
                << "Invalid PORT value. Using 8080."
                << std::endl;
        }
    }


    // =====================================================
    // Drogon application
    // =====================================================

    auto& app = drogon::app();


    // =====================================================
    // Frontend document root
    // =====================================================

    app.setDocumentRoot(frontendDirectory);


    // =====================================================
    // Serve uploaded product images
    // =====================================================

    app.registerHandler(
        "/uploads/{1}",

        [uploadDirectory](
            const drogon::HttpRequestPtr& req,
            std::function<void(
                const drogon::HttpResponsePtr&
            )>&& callback,

            const std::string& filename
        )
        {
            const fs::path uploadDir =
                fs::path(uploadDirectory);


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
                uploadDir / filename;


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
            // Send image
            // -------------------------------------------------

            auto response =
                drogon::HttpResponse::newFileResponse(
                    imagePath.string()
                );


            // -------------------------------------------------
            // Set content type
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
        "0.0.0.0",
        port
    );


    std::cout
        << "KaviyaMart server starting..."
        << std::endl;

    std::cout
        << "Frontend: "
        << frontendDirectory
        << std::endl;

    std::cout
        << "Upload directory: "
        << uploadDirectory
        << std::endl;

    std::cout
        << "Port: "
        << port
        << std::endl;


    app.run();

    return 0;
}
