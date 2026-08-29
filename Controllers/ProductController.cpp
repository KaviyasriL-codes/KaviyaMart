#include "ProductController.h"
#include "../Services/Database.h"

#include <drogon/drogon.h>

#include <filesystem>
#include <string>

using namespace drogon;

namespace
{
    const std::string uploadDirectory =
        "C:/capstonekaviya/frontend/uploads";


    Json::Value makeResponse(
        bool success,
        const std::string& message)
    {
        Json::Value response;

        response["success"] = success;
        response["message"] = message;

        return response;
    }
}


// =====================================================
// ADD PRODUCT
// =====================================================

void ProductController::addProduct(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
)
{
    try
    {
        // ---------------------------------------------
        // Parse multipart/form-data
        // ---------------------------------------------

        MultiPartParser parser;

        if (parser.parse(req) != 0)
        {
            Json::Value response =
                makeResponse(
                    false,
                    "Invalid multipart form data."
                );

            auto httpResponse =
                HttpResponse::newHttpJsonResponse(response);

            httpResponse->setStatusCode(
                k400BadRequest
            );

            callback(httpResponse);

            return;
        }


        // ---------------------------------------------
        // Get normal form fields
        // ---------------------------------------------

        const auto& parameters =
            parser.getParameters();

        const auto& files =
            parser.getFiles();


        if (files.empty())
        {
            Json::Value response =
                makeResponse(
                    false,
                    "Please select a product image."
                );

            auto httpResponse =
                HttpResponse::newHttpJsonResponse(response);

            httpResponse->setStatusCode(
                k400BadRequest
            );

            callback(httpResponse);

            return;
        }


        // ---------------------------------------------
        // Read seller ID
        // ---------------------------------------------

        if (parameters.find("seller_id") ==
            parameters.end())
        {
            Json::Value response =
                makeResponse(
                    false,
                    "Seller ID is missing."
                );

            auto httpResponse =
                HttpResponse::newHttpJsonResponse(response);

            httpResponse->setStatusCode(
                k400BadRequest
            );

            callback(httpResponse);

            return;
        }


        int sellerId =
            std::stoi(
                parameters.at("seller_id")
            );


        // ---------------------------------------------
        // Read product fields
        // ---------------------------------------------

        std::string productName;

        std::string category;

        std::string description;

        double price = 0.0;


        if (parameters.find("product_name") !=
            parameters.end())
        {
            productName =
                parameters.at("product_name");
        }


        if (parameters.find("category") !=
            parameters.end())
        {
            category =
                parameters.at("category");
        }


        if (parameters.find("description") !=
            parameters.end())
        {
            description =
                parameters.at("description");
        }


        if (parameters.find("price") !=
            parameters.end())
        {
            price =
                std::stod(
                    parameters.at("price")
                );
        }


        // ---------------------------------------------
        // Validate fields
        // ---------------------------------------------

        if (sellerId <= 0 ||
            productName.empty() ||
            category.empty() ||
            price < 0)
        {
            Json::Value response =
                makeResponse(
                    false,
                    "Invalid product information."
                );

            auto httpResponse =
                HttpResponse::newHttpJsonResponse(response);

            httpResponse->setStatusCode(
                k400BadRequest
            );

            callback(httpResponse);

            return;
        }


        // ---------------------------------------------
        // Verify seller
        // ---------------------------------------------

        auto db =
            Database::connect();

        pqxx::work transaction(*db);


        pqxx::result sellerCheck =
            transaction.exec_params(
                "SELECT user_id "
                "FROM users "
                "WHERE user_id = $1 "
                "AND role = 'seller'",
                sellerId
            );


        if (sellerCheck.empty())
        {
            transaction.abort();

            Json::Value response =
                makeResponse(
                    false,
                    "Invalid seller."
                );

            auto httpResponse =
                HttpResponse::newHttpJsonResponse(response);

            httpResponse->setStatusCode(
                k403Forbidden
            );

            callback(httpResponse);

            return;
        }


        // ---------------------------------------------
        // Validate image
        // ---------------------------------------------

        const auto& uploadedFile =
            files[0];


        std::string originalFilename =
            uploadedFile.getFileName();


        std::string extension =
            std::filesystem::path(
                originalFilename
            ).extension().string();


        // Convert extension to lowercase

        for (char& c : extension)
        {
            c =
                static_cast<char>(
                    std::tolower(
                        static_cast<unsigned char>(c)
                    )
                );
        }


        if (extension != ".jpg" &&
            extension != ".jpeg" &&
            extension != ".png" &&
            extension != ".webp")
        {
            transaction.abort();

            Json::Value response =
                makeResponse(
                    false,
                    "Only JPG, JPEG, PNG and WEBP images are allowed."
                );

            auto httpResponse =
                HttpResponse::newHttpJsonResponse(response);

            httpResponse->setStatusCode(
                k400BadRequest
            );

            callback(httpResponse);

            return;
        }


        // ---------------------------------------------
        // Create upload directory
        // ---------------------------------------------

        std::filesystem::create_directories(
            uploadDirectory
        );


        // ---------------------------------------------
        // Generate unique filename
        // ---------------------------------------------

        const std::string uniqueFilename =
            "product_" +
            std::to_string(
                std::chrono::system_clock::now()
                    .time_since_epoch()
                    .count()
            ) +
            extension;


        const std::string fullPath =
            uploadDirectory +
            "/" +
            uniqueFilename;


        // ---------------------------------------------
        // Save uploaded image
        // ---------------------------------------------

        uploadedFile.saveAs(
            fullPath
        );


        // Browser-accessible path

        const std::string imagePath =
            "/uploads/" +
            uniqueFilename;


        // ---------------------------------------------
        // Insert product
        // ---------------------------------------------

        pqxx::result result =
            transaction.exec_params(
                "INSERT INTO products "
                "(seller_id, product_name, category, "
                "price, description, image_path) "
                "VALUES ($1, $2, $3, $4, $5, $6) "
                "RETURNING product_id",
                sellerId,
                productName,
                category,
                price,
                description,
                imagePath
            );


        transaction.commit();


        // ---------------------------------------------
        // Success response
        // ---------------------------------------------

        Json::Value response;

        response["success"] = true;

        response["message"] =
            "Product added successfully.";

        response["product_id"] =
            result[0]["product_id"].as<int>();

        response["image_path"] =
            imagePath;


        callback(
            HttpResponse::newHttpJsonResponse(
                response
            )
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Add product error: "
            << e.what();


        Json::Value response =
            makeResponse(
                false,
                "Failed to add product."
            );


        auto httpResponse =
            HttpResponse::newHttpJsonResponse(
                response
            );

        httpResponse->setStatusCode(
            k500InternalServerError
        );

        callback(httpResponse);
    }
}



// =====================================================
// GET ALL PRODUCTS
// =====================================================

void ProductController::getProducts(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
)
{
    try
    {
        auto db =
            Database::connect();

        pqxx::work transaction(*db);

        pqxx::result result =
            transaction.exec(
                "SELECT product_id, seller_id, "
                "product_name, category, price, "
                "description, image_path, rating "
                "FROM products "
                "ORDER BY created_at DESC"
            );

        transaction.commit();


        Json::Value products(
            Json::arrayValue
        );


        for (const auto& row : result)
        {
            Json::Value product;


            product["product_id"] =
                row["product_id"].as<int>();


            product["seller_id"] =
                row["seller_id"].as<int>();


            product["product_name"] =
                row["product_name"].c_str();


            product["category"] =
                row["category"].c_str();


            product["price"] =
                row["price"].as<double>();


            product["description"] =
                row["description"].is_null()
                    ? ""
                    : row["description"].c_str();


            product["image_path"] =
                row["image_path"].is_null()
                    ? ""
                    : row["image_path"].c_str();


            product["rating"] =
                row["rating"].is_null()
                    ? 0
                    : row["rating"].as<double>();


            products.append(product);
        }


        Json::Value response;

        response["success"] = true;

        response["products"] =
            products;


        callback(
            HttpResponse::newHttpJsonResponse(
                response
            )
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Get products error: "
            << e.what();


        Json::Value response;

        response["success"] = false;

        response["message"] =
            "Failed to load products.";


        callback(
            HttpResponse::newHttpJsonResponse(
                response
            )
        );
    }
}



// =====================================================
// GET SELLER PRODUCTS
// =====================================================

void ProductController::getSellerProducts(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    int sellerId
)
{
    try
    {
        auto db =
            Database::connect();

        pqxx::work transaction(*db);


        pqxx::result result =
            transaction.exec_params(
                "SELECT product_id, seller_id, "
                "product_name, category, price, "
                "description, image_path, rating "
                "FROM products "
                "WHERE seller_id = $1 "
                "ORDER BY created_at DESC",
                sellerId
            );


        transaction.commit();


        Json::Value products(
            Json::arrayValue
        );


        for (const auto& row : result)
        {
            Json::Value product;


            product["product_id"] =
                row["product_id"].as<int>();


            product["seller_id"] =
                row["seller_id"].as<int>();


            product["product_name"] =
                row["product_name"].c_str();


            product["category"] =
                row["category"].c_str();


            product["price"] =
                row["price"].as<double>();


            product["description"] =
                row["description"].is_null()
                    ? ""
                    : row["description"].c_str();


            product["image_path"] =
                row["image_path"].is_null()
                    ? ""
                    : row["image_path"].c_str();


            product["rating"] =
                row["rating"].is_null()
                    ? 0
                    : row["rating"].as<double>();


            products.append(product);
        }


        Json::Value response;

        response["success"] = true;

        response["products"] =
            products;


        callback(
            HttpResponse::newHttpJsonResponse(
                response
            )
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Get seller products error: "
            << e.what();


        Json::Value response;

        response["success"] = false;

        response["message"] =
            "Failed to load seller products.";


        callback(
            HttpResponse::newHttpJsonResponse(
                response
            )
        );
    }
}