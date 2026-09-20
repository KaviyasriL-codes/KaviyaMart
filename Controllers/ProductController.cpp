#include "ProductController.h"
#include "../Services/Database.h"

#include <drogon/drogon.h>
#include <drogon/MultiPart.h>
#include <pqxx/pqxx>

#include <filesystem>
#include <string>
#include <chrono>
#include <cctype>
#include <iostream>
#include <cstdlib>

namespace fs = std::filesystem;

using namespace drogon;


// ============================================================
// Helper: Create JSON response
// ============================================================

static HttpResponsePtr makeJsonResponse(
    const Json::Value& json,
    HttpStatusCode status = k200OK)
{
    auto response = HttpResponse::newHttpJsonResponse(json);
    response->setStatusCode(status);
    return response;
}


// ============================================================
// Helper: Convert extension to lowercase
// ============================================================

static std::string lowerExtension(const std::string& extension)
{
    std::string result = extension;

    for (char& c : result)
    {
        c = static_cast<char>(
            std::tolower(static_cast<unsigned char>(c))
        );
    }

    return result;
}


// ============================================================
// Helper: Generate unique image filename
// ============================================================

static std::string generateImageName(const std::string& extension)
{
    const auto timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

    return "product_" +
           std::to_string(timestamp) +
           extension;
}


// ============================================================
// Helper: Read multipart form parameter
//
// Compatible with older Drogon versions.
// getParameter<T>() is not available in older versions,
// so we use getParameters() instead.
// ============================================================

static std::string getMultipartParameter(
    const MultiPartParser& parser,
    const std::string& key)
{
    const auto& parameters = parser.getParameters();

    auto it = parameters.find(key);

    if (it == parameters.end())
    {
        return "";
    }

    return it->second;
}


// ============================================================
// Helper: Get upload directory
//
// Local Windows:
// C:/capstonekaviya/frontend/uploads
//
// Docker:
// Uses UPLOAD_DIR environment variable.
// ============================================================

static fs::path getUploadDirectory()
{
    const char* uploadEnv = std::getenv("UPLOAD_DIR");

    if (uploadEnv != nullptr &&
        std::string(uploadEnv).size() > 0)
    {
        return fs::path(uploadEnv);
    }

    return fs::path(
        "C:/capstonekaviya/frontend/uploads"
    );
}


// ============================================================
// ADD PRODUCT
// POST /api/products
// ============================================================

void ProductController::addProduct(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    try
    {
        MultiPartParser parser;

        if (parser.parse(req) != 0)
        {
            Json::Value result;

            result["success"] = false;
            result["message"] =
                "Unable to process product form.";

            callback(
                makeJsonResponse(
                    result,
                    k400BadRequest
                )
            );

            return;
        }


        // ----------------------------------------------------
        // Read form fields
        // ----------------------------------------------------

        std::string sellerIdText =
            getMultipartParameter(
                parser,
                "seller_id"
            );

        std::string productName =
            getMultipartParameter(
                parser,
                "product_name"
            );

        std::string category =
            getMultipartParameter(
                parser,
                "category"
            );

        std::string priceText =
            getMultipartParameter(
                parser,
                "price"
            );

        std::string description =
            getMultipartParameter(
                parser,
                "description"
            );


        if (sellerIdText.empty() ||
            productName.empty() ||
            category.empty() ||
            priceText.empty() ||
            description.empty())
        {
            Json::Value result;

            result["success"] = false;
            result["message"] =
                "All product fields are required.";

            callback(
                makeJsonResponse(
                    result,
                    k400BadRequest
                )
            );

            return;
        }


        int sellerId = 0;
        double price = 0.0;


        try
        {
            sellerId = std::stoi(sellerIdText);
            price = std::stod(priceText);
        }
        catch (...)
        {
            Json::Value result;

            result["success"] = false;
            result["message"] =
                "Invalid seller ID or price.";

            callback(
                makeJsonResponse(
                    result,
                    k400BadRequest
                )
            );

            return;
        }


        if (price < 0)
        {
            Json::Value result;

            result["success"] = false;
            result["message"] =
                "Price cannot be negative.";

            callback(
                makeJsonResponse(
                    result,
                    k400BadRequest
                )
            );

            return;
        }


        // ----------------------------------------------------
        // Check image
        // ----------------------------------------------------

        const auto& files = parser.getFiles();

        if (files.empty())
        {
            Json::Value result;

            result["success"] = false;
            result["message"] =
                "Product image is required.";

            callback(
                makeJsonResponse(
                    result,
                    k400BadRequest
                )
            );

            return;
        }


        const HttpFile& imageFile = files[0];

        std::string originalFilename =
            imageFile.getFileName();

        fs::path originalPath(originalFilename);

        std::string extension =
            lowerExtension(
                originalPath.extension().string()
            );


        if (extension != ".jpg" &&
            extension != ".jpeg" &&
            extension != ".png" &&
            extension != ".webp")
        {
            Json::Value result;

            result["success"] = false;
            result["message"] =
                "Only JPG, JPEG, PNG and WEBP images are allowed.";

            callback(
                makeJsonResponse(
                    result,
                    k400BadRequest
                )
            );

            return;
        }


        // ----------------------------------------------------
        // Upload directory
        // ----------------------------------------------------

        const fs::path uploadDirectory =
            getUploadDirectory();


        try
        {
            fs::create_directories(uploadDirectory);
        }
        catch (const std::exception& e)
        {
            Json::Value result;

            result["success"] = false;
            result["message"] =
                std::string(
                    "Could not create upload directory: "
                ) + e.what();

            callback(
                makeJsonResponse(
                    result,
                    k500InternalServerError
                )
            );

            return;
        }


        // ----------------------------------------------------
        // Generate unique filename
        // ----------------------------------------------------

        std::string newFileName =
            generateImageName(extension);

        fs::path fullFilePath =
            uploadDirectory / newFileName;


        // ----------------------------------------------------
        // Save image
        // ----------------------------------------------------

        try
        {
            imageFile.saveAs(
                fullFilePath.string()
            );
        }
        catch (const std::exception& e)
        {
            Json::Value result;

            result["success"] = false;
            result["message"] =
                std::string(
                    "Could not save product image: "
                ) + e.what();

            callback(
                makeJsonResponse(
                    result,
                    k500InternalServerError
                )
            );

            return;
        }


        std::string imagePath =
            "/uploads/" + newFileName;


        // ----------------------------------------------------
        // Database
        // ----------------------------------------------------

        auto db = Database::connect();

        if (!db || !db->is_open())
        {
            std::error_code ec;
            fs::remove(fullFilePath, ec);

            Json::Value result;

            result["success"] = false;
            result["message"] =
                "Database connection failed.";

            callback(
                makeJsonResponse(
                    result,
                    k500InternalServerError
                )
            );

            return;
        }


        // ----------------------------------------------------
        // Check seller
        // ----------------------------------------------------

        pqxx::work transaction(*db);

        pqxx::result sellerResult =
            transaction.exec_params(
                "SELECT user_id, role "
                "FROM users "
                "WHERE user_id = $1",
                sellerId
            );


        if (sellerResult.empty())
        {
            transaction.abort();

            std::error_code ec;
            fs::remove(fullFilePath, ec);

            Json::Value result;

            result["success"] = false;
            result["message"] =
                "Seller account not found.";

            callback(
                makeJsonResponse(
                    result,
                    k404NotFound
                )
            );

            return;
        }


        std::string role =
            sellerResult[0]["role"].as<std::string>();


        if (role != "seller")
        {
            transaction.abort();

            std::error_code ec;
            fs::remove(fullFilePath, ec);

            Json::Value result;

            result["success"] = false;
            result["message"] =
                "Only seller accounts can add products.";

            callback(
                makeJsonResponse(
                    result,
                    k403Forbidden
                )
            );

            return;
        }


        // ----------------------------------------------------
        // Insert product
        // ----------------------------------------------------

        pqxx::result insertedProduct =
            transaction.exec_params(
                "INSERT INTO products "
                "(seller_id, product_name, category, price, "
                "description, image_path) "
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


        int productId =
            insertedProduct[0]["product_id"].as<int>();


        // ----------------------------------------------------
        // Success
        // ----------------------------------------------------

        Json::Value result;

        result["success"] = true;
        result["message"] =
            "Product added successfully.";

        result["product_id"] =
            productId;

        result["image_path"] =
            imagePath;

        callback(
            makeJsonResponse(
                result,
                k201Created
            )
        );
    }
    catch (const std::exception& e)
    {
        Json::Value result;

        result["success"] = false;
        result["message"] =
            std::string(
                "Failed to add product: "
            ) + e.what();

        callback(
            makeJsonResponse(
                result,
                k500InternalServerError
            )
        );
    }
}


// ============================================================
// GET ALL PRODUCTS
// GET /api/products
// ============================================================

void ProductController::getProducts(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    try
    {
        auto db = Database::connect();

        if (!db || !db->is_open())
        {
            Json::Value result;

            result["success"] = false;
            result["message"] =
                "Database connection failed.";

            callback(
                makeJsonResponse(
                    result,
                    k500InternalServerError
                )
            );

            return;
        }


        pqxx::work transaction(*db);

        pqxx::result products =
            transaction.exec(
                "SELECT "
                "product_id, "
                "seller_id, "
                "product_name, "
                "category, "
                "price, "
                "description, "
                "image_path, "
                "rating "
                "FROM products "
                "ORDER BY product_id DESC"
            );

        transaction.commit();


        Json::Value result;

        result["success"] = true;
        result["products"] =
            Json::arrayValue;


        for (const auto& row : products)
        {
            Json::Value product;

            product["product_id"] =
                row["product_id"].as<int>();

            product["seller_id"] =
                row["seller_id"].as<int>();

            product["product_name"] =
                row["product_name"].as<std::string>();

            product["category"] =
                row["category"].as<std::string>();

            product["price"] =
                row["price"].as<double>();

            product["description"] =
                row["description"].as<std::string>();


            if (row["image_path"].is_null())
            {
                product["image_path"] = "";
            }
            else
            {
                product["image_path"] =
                    row["image_path"].as<std::string>();
            }


            if (row["rating"].is_null())
            {
                product["rating"] = 0;
            }
            else
            {
                product["rating"] =
                    row["rating"].as<double>();
            }


            result["products"].append(product);
        }


        callback(
            makeJsonResponse(result)
        );
    }
    catch (const std::exception& e)
    {
        Json::Value result;

        result["success"] = false;
        result["message"] =
            std::string(
                "Failed to load products: "
            ) + e.what();

        callback(
            makeJsonResponse(
                result,
                k500InternalServerError
            )
        );
    }
}


// ============================================================
// GET SELLER PRODUCTS
// GET /api/products/seller/{sellerId}
// ============================================================

void ProductController::getSellerProducts(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    int sellerId)
{
    try
    {
        auto db = Database::connect();

        if (!db || !db->is_open())
        {
            Json::Value result;

            result["success"] = false;
            result["message"] =
                "Database connection failed.";

            callback(
                makeJsonResponse(
                    result,
                    k500InternalServerError
                )
            );

            return;
        }


        pqxx::work transaction(*db);

        pqxx::result products =
            transaction.exec_params(
                "SELECT "
                "product_id, "
                "seller_id, "
                "product_name, "
                "category, "
                "price, "
                "description, "
                "image_path, "
                "rating "
                "FROM products "
                "WHERE seller_id = $1 "
                "ORDER BY product_id DESC",
                sellerId
            );

        transaction.commit();


        Json::Value result;

        result["success"] = true;
        result["products"] =
            Json::arrayValue;


        for (const auto& row : products)
        {
            Json::Value product;

            product["product_id"] =
                row["product_id"].as<int>();

            product["seller_id"] =
                row["seller_id"].as<int>();

            product["product_name"] =
                row["product_name"].as<std::string>();

            product["category"] =
                row["category"].as<std::string>();

            product["price"] =
                row["price"].as<double>();

            product["description"] =
                row["description"].as<std::string>();


            if (row["image_path"].is_null())
            {
                product["image_path"] = "";
            }
            else
            {
                product["image_path"] =
                    row["image_path"].as<std::string>();
            }


            if (row["rating"].is_null())
            {
                product["rating"] = 0;
            }
            else
            {
                product["rating"] =
                    row["rating"].as<double>();
            }


            result["products"].append(product);
        }


        callback(
            makeJsonResponse(result)
        );
    }
    catch (const std::exception& e)
    {
        Json::Value result;

        result["success"] = false;
        result["message"] =
            std::string(
                "Failed to load seller products: "
            ) + e.what();

        callback(
            makeJsonResponse(
                result,
                k500InternalServerError
            )
        );
    }
}


// ============================================================
// GET SINGLE PRODUCT
// GET /api/products/{productId}
// ============================================================

void ProductController::getProduct(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    int productId)
{
    try
    {
        auto db = Database::connect();

        if (!db || !db->is_open())
        {
            Json::Value result;

            result["success"] = false;
            result["message"] =
                "Database connection failed.";

            callback(
                makeJsonResponse(
                    result,
                    k500InternalServerError
                )
            );

            return;
        }


        pqxx::work transaction(*db);

        pqxx::result products =
            transaction.exec_params(
                "SELECT "
                "product_id, "
                "seller_id, "
                "product_name, "
                "category, "
                "price, "
                "description, "
                "image_path, "
                "rating "
                "FROM products "
                "WHERE product_id = $1",
                productId
            );

        transaction.commit();


        if (products.empty())
        {
            Json::Value result;

            result["success"] = false;
            result["message"] =
                "Product not found.";

            callback(
                makeJsonResponse(
                    result,
                    k404NotFound
                )
            );

            return;
        }


        const auto& row = products[0];

        Json::Value product;

        product["product_id"] =
            row["product_id"].as<int>();

        product["seller_id"] =
            row["seller_id"].as<int>();

        product["product_name"] =
            row["product_name"].as<std::string>();

        product["category"] =
            row["category"].as<std::string>();

        product["price"] =
            row["price"].as<double>();

        product["description"] =
            row["description"].as<std::string>();


        if (row["image_path"].is_null())
        {
            product["image_path"] = "";
        }
        else
        {
            product["image_path"] =
                row["image_path"].as<std::string>();
        }


        if (row["rating"].is_null())
        {
            product["rating"] = 0;
        }
        else
        {
            product["rating"] =
                row["rating"].as<double>();
        }


        Json::Value result;

        result["success"] = true;
        result["product"] = product;

        callback(
            makeJsonResponse(result)
        );
    }
    catch (const std::exception& e)
    {
        Json::Value result;

        result["success"] = false;
        result["message"] =
            std::string(
                "Failed to load product: "
            ) + e.what();

        callback(
            makeJsonResponse(
                result,
                k500InternalServerError
            )
        );
    }
}


// ============================================================
// UPDATE PRODUCT
// PUT /api/products/{productId}
// ============================================================

void ProductController::updateProduct(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    int productId)
{
    std::string newImagePath;
    std::string oldImagePath;
    fs::path newImageFilePath;

    bool newImageSaved = false;


    try
    {
        // ----------------------------------------------------
        // Parse multipart form
        // ----------------------------------------------------

        MultiPartParser parser;

        if (parser.parse(req) != 0)
        {
            Json::Value result;

            result["success"] = false;
            result["message"] =
                "Unable to process product update form.";

            callback(
                makeJsonResponse(
                    result,
                    k400BadRequest
                )
            );

            return;
        }


        // ----------------------------------------------------
        // Read form fields
        // ----------------------------------------------------

        std::string sellerIdText =
            getMultipartParameter(
                parser,
                "seller_id"
            );

        std::string productName =
            getMultipartParameter(
                parser,
                "product_name"
            );

        std::string category =
            getMultipartParameter(
                parser,
                "category"
            );

        std::string priceText =
            getMultipartParameter(
                parser,
                "price"
            );

        std::string description =
            getMultipartParameter(
                parser,
                "description"
            );


        if (sellerIdText.empty() ||
            productName.empty() ||
            category.empty() ||
            priceText.empty() ||
            description.empty())
        {
            Json::Value result;

            result["success"] = false;
            result["message"] =
                "All product fields are required.";

            callback(
                makeJsonResponse(
                    result,
                    k400BadRequest
                )
            );

            return;
        }


        int sellerId = 0;
        double price = 0.0;


        try
        {
            sellerId = std::stoi(sellerIdText);
            price = std::stod(priceText);
        }
        catch (...)
        {
            Json::Value result;

            result["success"] = false;
            result["message"] =
                "Invalid seller ID or price.";

            callback(
                makeJsonResponse(
                    result,
                    k400BadRequest
                )
            );

            return;
        }


        if (price < 0)
        {
            Json::Value result;

            result["success"] = false;
            result["message"] =
                "Price cannot be negative.";

            callback(
                makeJsonResponse(
                    result,
                    k400BadRequest
                )
            );

            return;
        }


        // ----------------------------------------------------
        // Connect DB
        // ----------------------------------------------------

        auto db = Database::connect();

        if (!db || !db->is_open())
        {
            Json::Value result;

            result["success"] = false;
            result["message"] =
                "Database connection failed.";

            callback(
                makeJsonResponse(
                    result,
                    k500InternalServerError
                )
            );

            return;
        }


        // ----------------------------------------------------
        // Check product + ownership
        // ----------------------------------------------------

        pqxx::work transaction(*db);

        pqxx::result existingProduct =
            transaction.exec_params(
                "SELECT "
                "product_id, "
                "seller_id, "
                "image_path "
                "FROM products "
                "WHERE product_id = $1",
                productId
            );


        if (existingProduct.empty())
        {
            transaction.abort();

            Json::Value result;

            result["success"] = false;
            result["message"] =
                "Product not found.";

            callback(
                makeJsonResponse(
                    result,
                    k404NotFound
                )
            );

            return;
        }


        int existingSellerId =
            existingProduct[0]["seller_id"].as<int>();


        if (existingSellerId != sellerId)
        {
            transaction.abort();

            Json::Value result;

            result["success"] = false;
            result["message"] =
                "You are not allowed to edit this product.";

            callback(
                makeJsonResponse(
                    result,
                    k403Forbidden
                )
            );

            return;
        }


        if (!existingProduct[0]["image_path"].is_null())
        {
            oldImagePath =
                existingProduct[0]["image_path"]
                    .as<std::string>();
        }


        // ----------------------------------------------------
        // Check whether a new image was uploaded
        // ----------------------------------------------------

        const auto& files = parser.getFiles();

        bool hasNewImage =
            !files.empty();


        // ----------------------------------------------------
        // Save new image if provided
        // ----------------------------------------------------

        if (hasNewImage)
        {
            const HttpFile& imageFile =
                files[0];

            std::string originalFilename =
                imageFile.getFileName();

            fs::path originalPath(
                originalFilename
            );

            std::string extension =
                lowerExtension(
                    originalPath.extension().string()
                );


            if (extension != ".jpg" &&
                extension != ".jpeg" &&
                extension != ".png" &&
                extension != ".webp")
            {
                transaction.abort();

                Json::Value result;

                result["success"] = false;
                result["message"] =
                    "Only JPG, JPEG, PNG and WEBP images are allowed.";

                callback(
                    makeJsonResponse(
                        result,
                        k400BadRequest
                    )
                );

                return;
            }


            // ------------------------------------------------
            // Upload directory
            // ------------------------------------------------

            const fs::path uploadDirectory =
                getUploadDirectory();


            try
            {
                fs::create_directories(
                    uploadDirectory
                );
            }
            catch (const std::exception& e)
            {
                transaction.abort();

                Json::Value result;

                result["success"] = false;
                result["message"] =
                    std::string(
                        "Could not create upload directory: "
                    ) + e.what();

                callback(
                    makeJsonResponse(
                        result,
                        k500InternalServerError
                    )
                );

                return;
            }


            // ------------------------------------------------
            // Unique filename
            // ------------------------------------------------

            std::string newFileName =
                generateImageName(extension);

            newImageFilePath =
                uploadDirectory / newFileName;

            newImagePath =
                "/uploads/" + newFileName;


            // ------------------------------------------------
            // Save image
            // ------------------------------------------------

            try
            {
                std::cout
                    << "Saving edited product image to: "
                    << newImageFilePath.string()
                    << std::endl;

                imageFile.saveAs(
                    newImageFilePath.string()
                );

                newImageSaved = true;
            }
            catch (const std::exception& e)
            {
                transaction.abort();

                Json::Value result;

                result["success"] = false;
                result["message"] =
                    std::string(
                        "Could not save new product image: "
                    ) + e.what();

                callback(
                    makeJsonResponse(
                        result,
                        k500InternalServerError
                    )
                );

                return;
            }
        }


        // ----------------------------------------------------
        // Update product
        // ----------------------------------------------------

        if (hasNewImage)
        {
            transaction.exec_params(
                "UPDATE products "
                "SET product_name = $1, "
                "category = $2, "
                "price = $3, "
                "description = $4, "
                "image_path = $5 "
                "WHERE product_id = $6",
                productName,
                category,
                price,
                description,
                newImagePath,
                productId
            );
        }
        else
        {
            transaction.exec_params(
                "UPDATE products "
                "SET product_name = $1, "
                "category = $2, "
                "price = $3, "
                "description = $4 "
                "WHERE product_id = $5",
                productName,
                category,
                price,
                description,
                productId
            );
        }


        transaction.commit();


        // ----------------------------------------------------
        // Delete old image AFTER successful DB update
        // ----------------------------------------------------

        if (hasNewImage &&
            !oldImagePath.empty())
        {
            try
            {
                std::string oldFilename =
                    oldImagePath;

                const std::string prefix =
                    "/uploads/";


                if (oldFilename.rfind(prefix, 0) == 0)
                {
                    oldFilename =
                        oldFilename.substr(
                            prefix.length()
                        );
                }


                // Security check
                if (oldFilename.find("..") ==
                        std::string::npos &&
                    oldFilename.find('/') ==
                        std::string::npos &&
                    oldFilename.find('\\') ==
                        std::string::npos)
                {
                    fs::path oldFile =
                        getUploadDirectory() /
                        oldFilename;


                    if (fs::exists(oldFile) &&
                        fs::is_regular_file(oldFile))
                    {
                        fs::remove(oldFile);

                        std::cout
                            << "Deleted old product image: "
                            << oldFile.string()
                            << std::endl;
                    }
                }
            }
            catch (const std::exception& e)
            {
                std::cerr
                    << "Could not delete old product image: "
                    << e.what()
                    << std::endl;
            }
        }


        // ----------------------------------------------------
        // Success
        // ----------------------------------------------------

        Json::Value result;

        result["success"] = true;

        result["message"] =
            "Product updated successfully.";

        result["product_id"] =
            productId;


        if (hasNewImage)
        {
            result["image_path"] =
                newImagePath;
        }
        else
        {
            result["image_path"] =
                oldImagePath;
        }


        callback(
            makeJsonResponse(result)
        );
    }
    catch (const std::exception& e)
    {
        // ----------------------------------------------------
        // Remove newly uploaded image if DB update failed
        // ----------------------------------------------------

        if (newImageSaved &&
            !newImageFilePath.empty())
        {
            try
            {
                if (fs::exists(newImageFilePath))
                {
                    fs::remove(
                        newImageFilePath
                    );
                }
            }
            catch (...)
            {
                // Ignore cleanup failure
            }
        }


        Json::Value result;

        result["success"] = false;

        result["message"] =
            std::string(
                "Failed to update product: "
            ) + e.what();


        callback(
            makeJsonResponse(
                result,
                k500InternalServerError
            )
        );
    }
}