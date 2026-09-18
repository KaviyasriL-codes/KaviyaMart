#include "AdminController.h"
#include "../Services/Database.h"

#include <drogon/drogon.h>
#include <pqxx/pqxx>

#include <string>

using namespace drogon;


// ============================================================
// ADMIN - DASHBOARD STATISTICS
// ============================================================

void AdminController::getStats(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    try
    {
        const std::string adminId =
            req->getParameter("user_id");

        if (adminId.empty())
        {
            Json::Value response;

            response["success"] = false;
            response["message"] =
                "Admin user ID is required.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k400BadRequest);

            callback(result);
            return;
        }


        auto db = Database::connect();

        pqxx::work transaction(*db);


        // --------------------------------------------
        // VERIFY ADMIN
        // --------------------------------------------

        pqxx::result adminResult =
            transaction.exec_params(
                "SELECT role "
                "FROM users "
                "WHERE user_id = $1",
                adminId
            );

        if (adminResult.empty())
        {
            transaction.abort();

            Json::Value response;

            response["success"] = false;
            response["message"] =
                "Admin user not found.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k403Forbidden);

            callback(result);
            return;
        }


        if (adminResult[0]["role"].as<std::string>() != "admin")
        {
            transaction.abort();

            Json::Value response;

            response["success"] = false;
            response["message"] =
                "Admin access required.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k403Forbidden);

            callback(result);
            return;
        }


        // --------------------------------------------
        // TOTAL USERS
        // --------------------------------------------

        pqxx::result usersResult =
            transaction.exec(
                "SELECT COUNT(*) AS total "
                "FROM users"
            );


        // --------------------------------------------
        // TOTAL SELLERS
        // --------------------------------------------

        pqxx::result sellersResult =
            transaction.exec(
                "SELECT COUNT(*) AS total "
                "FROM users "
                "WHERE role = 'seller'"
            );


        // --------------------------------------------
        // TOTAL BUYERS
        // --------------------------------------------

        pqxx::result buyersResult =
            transaction.exec(
                "SELECT COUNT(*) AS total "
                "FROM users "
                "WHERE role = 'buyer'"
            );


        // --------------------------------------------
        // TOTAL PRODUCTS
        // --------------------------------------------

        pqxx::result productsResult =
            transaction.exec(
                "SELECT COUNT(*) AS total "
                "FROM products"
            );


        // --------------------------------------------
        // TOTAL ORDERS
        // --------------------------------------------

        pqxx::result ordersResult =
            transaction.exec(
                "SELECT COUNT(*) AS total "
                "FROM orders"
            );


        // --------------------------------------------
        // TOTAL SALES
        // --------------------------------------------

        pqxx::result salesResult =
            transaction.exec(
                "SELECT COALESCE("
                "SUM(total_amount), 0"
                ") AS total "
                "FROM orders"
            );


        int totalUsers =
            usersResult[0]["total"].as<int>();

        int totalSellers =
            sellersResult[0]["total"].as<int>();

        int totalBuyers =
            buyersResult[0]["total"].as<int>();

        int totalProducts =
            productsResult[0]["total"].as<int>();

        int totalOrders =
            ordersResult[0]["total"].as<int>();

        std::string totalSales =
            salesResult[0]["total"].as<std::string>();


        transaction.commit();


        // --------------------------------------------
        // RESPONSE
        // --------------------------------------------

        Json::Value response;

        response["success"] = true;

        response["total_users"] =
            totalUsers;

        response["total_sellers"] =
            totalSellers;

        response["total_buyers"] =
            totalBuyers;

        response["total_products"] =
            totalProducts;

        response["total_orders"] =
            totalOrders;

        response["total_sales"] =
            totalSales;


        callback(
            HttpResponse::newHttpJsonResponse(response)
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Admin statistics error: "
            << e.what();

        Json::Value response;

        response["success"] = false;
        response["message"] =
            "Failed to load admin statistics.";

        auto result =
            HttpResponse::newHttpJsonResponse(response);

        result->setStatusCode(k500InternalServerError);

        callback(result);
    }
}



// ============================================================
// ADMIN - VIEW SELLERS
// ============================================================

void AdminController::getSellers(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    try
    {
        const std::string adminId =
            req->getParameter("user_id");

        if (adminId.empty())
        {
            Json::Value response;

            response["success"] = false;
            response["message"] =
                "Admin user ID is required.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k400BadRequest);

            callback(result);
            return;
        }


        auto db = Database::connect();

        pqxx::work transaction(*db);


        // Verify admin

        pqxx::result adminResult =
            transaction.exec_params(
                "SELECT role "
                "FROM users "
                "WHERE user_id = $1",
                adminId
            );


        if (
            adminResult.empty() ||
            adminResult[0]["role"].as<std::string>() != "admin"
        )
        {
            transaction.abort();

            Json::Value response;

            response["success"] = false;
            response["message"] =
                "Admin access required.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k403Forbidden);

            callback(result);
            return;
        }


        // Get sellers

        pqxx::result result =
            transaction.exec(
                "SELECT "
                "user_id, "
                "name, "
                "email, "
                "mobile, "
                "created_at "
                "FROM users "
                "WHERE role = 'seller' "
                "ORDER BY created_at DESC"
            );


        Json::Value sellers(
            Json::arrayValue
        );


        for (const auto& row : result)
        {
            Json::Value seller;

            seller["user_id"] =
                row["user_id"].as<int>();

            seller["name"] =
                row["name"].as<std::string>();

            seller["email"] =
                row["email"].as<std::string>();

            seller["mobile"] =
                row["mobile"].is_null()
                    ? ""
                    : row["mobile"].as<std::string>();

            seller["created_at"] =
                row["created_at"].is_null()
                    ? ""
                    : row["created_at"].as<std::string>();

            sellers.append(seller);
        }


        transaction.commit();


        Json::Value response;

        response["success"] = true;

        response["total_sellers"] =
            static_cast<int>(sellers.size());

        response["sellers"] =
            sellers;


        callback(
            HttpResponse::newHttpJsonResponse(response)
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Admin sellers error: "
            << e.what();

        Json::Value response;

        response["success"] = false;
        response["message"] =
            "Failed to load sellers.";

        auto result =
            HttpResponse::newHttpJsonResponse(response);

        result->setStatusCode(k500InternalServerError);

        callback(result);
    }
}



// ============================================================
// ADMIN - VIEW BUYERS
// ============================================================

void AdminController::getBuyers(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    try
    {
        const std::string adminId =
            req->getParameter("user_id");

        if (adminId.empty())
        {
            Json::Value response;

            response["success"] = false;
            response["message"] =
                "Admin user ID is required.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k400BadRequest);

            callback(result);
            return;
        }


        auto db = Database::connect();

        pqxx::work transaction(*db);


        // Verify admin

        pqxx::result adminResult =
            transaction.exec_params(
                "SELECT role "
                "FROM users "
                "WHERE user_id = $1",
                adminId
            );


        if (
            adminResult.empty() ||
            adminResult[0]["role"].as<std::string>() != "admin"
        )
        {
            transaction.abort();

            Json::Value response;

            response["success"] = false;
            response["message"] =
                "Admin access required.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k403Forbidden);

            callback(result);
            return;
        }


        // Get buyers

        pqxx::result result =
            transaction.exec(
                "SELECT "
                "user_id, "
                "name, "
                "email, "
                "mobile, "
                "created_at "
                "FROM users "
                "WHERE role = 'buyer' "
                "ORDER BY created_at DESC"
            );


        Json::Value buyers(
            Json::arrayValue
        );


        for (const auto& row : result)
        {
            Json::Value buyer;

            buyer["user_id"] =
                row["user_id"].as<int>();

            buyer["name"] =
                row["name"].as<std::string>();

            buyer["email"] =
                row["email"].as<std::string>();

            buyer["mobile"] =
                row["mobile"].is_null()
                    ? ""
                    : row["mobile"].as<std::string>();

            buyer["created_at"] =
                row["created_at"].is_null()
                    ? ""
                    : row["created_at"].as<std::string>();

            buyers.append(buyer);
        }


        transaction.commit();


        Json::Value response;

        response["success"] = true;

        response["total_buyers"] =
            static_cast<int>(buyers.size());

        response["buyers"] =
            buyers;


        callback(
            HttpResponse::newHttpJsonResponse(response)
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Admin buyers error: "
            << e.what();

        Json::Value response;

        response["success"] = false;
        response["message"] =
            "Failed to load buyers.";

        auto result =
            HttpResponse::newHttpJsonResponse(response);

        result->setStatusCode(k500InternalServerError);

        callback(result);
    }
}



// ============================================================
// ADMIN - VIEW PRODUCTS
// ============================================================

void AdminController::getProducts(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    try
    {
        const std::string adminId =
            req->getParameter("user_id");

        if (adminId.empty())
        {
            Json::Value response;

            response["success"] = false;
            response["message"] =
                "Admin user ID is required.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k400BadRequest);

            callback(result);
            return;
        }


        auto db = Database::connect();

        pqxx::work transaction(*db);


        // --------------------------------------------
        // VERIFY ADMIN
        // --------------------------------------------

        pqxx::result adminResult =
            transaction.exec_params(
                "SELECT role "
                "FROM users "
                "WHERE user_id = $1",
                adminId
            );


        if (
            adminResult.empty() ||
            adminResult[0]["role"].as<std::string>() != "admin"
        )
        {
            transaction.abort();

            Json::Value response;

            response["success"] = false;
            response["message"] =
                "Admin access required.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k403Forbidden);

            callback(result);
            return;
        }


        // --------------------------------------------
        // GET PRODUCTS
        // --------------------------------------------

        pqxx::result result =
            transaction.exec(
                "SELECT "
                "p.product_id, "
                "p.product_name, "
                "p.category, "
                "p.price, "
                "p.rating, "
                "p.created_at, "
                "u.user_id AS seller_id, "
                "u.name AS seller_name "
                "FROM products p "
                "LEFT JOIN users u "
                "ON p.seller_id = u.user_id "
                "ORDER BY p.created_at DESC"
            );


        Json::Value products(
            Json::arrayValue
        );


        for (const auto& row : result)
        {
            Json::Value product;

            product["product_id"] =
                row["product_id"].as<int>();

            product["product_name"] =
                row["product_name"].as<std::string>();

            product["category"] =
                row["category"].as<std::string>();

            // Keep price as text because PostgreSQL
            // numeric values are returned as text.

            product["price"] =
                row["price"].is_null()
                    ? "0"
                    : row["price"].as<std::string>();

            product["rating"] =
                row["rating"].is_null()
                    ? 0
                    : row["rating"].as<double>();

            product["seller_id"] =
                row["seller_id"].is_null()
                    ? 0
                    : row["seller_id"].as<int>();

            product["seller_name"] =
                row["seller_name"].is_null()
                    ? "Unknown"
                    : row["seller_name"].as<std::string>();

            product["created_at"] =
                row["created_at"].is_null()
                    ? ""
                    : row["created_at"].as<std::string>();

            products.append(product);
        }


        transaction.commit();


        Json::Value response;

        response["success"] = true;

        response["total_products"] =
            static_cast<int>(products.size());

        response["products"] =
            products;


        callback(
            HttpResponse::newHttpJsonResponse(response)
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Admin products error: "
            << e.what();

        Json::Value response;

        response["success"] = false;
        response["message"] =
            "Failed to load products.";

        auto result =
            HttpResponse::newHttpJsonResponse(response);

        result->setStatusCode(k500InternalServerError);

        callback(result);
    }
}



// ============================================================
// ADMIN - VIEW ORDERS
// ============================================================

void AdminController::getOrders(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    try
    {
        const std::string adminId =
            req->getParameter("user_id");

        if (adminId.empty())
        {
            Json::Value response;

            response["success"] = false;
            response["message"] =
                "Admin user ID is required.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k400BadRequest);

            callback(result);
            return;
        }


        auto db = Database::connect();

        pqxx::work transaction(*db);


        // --------------------------------------------
        // VERIFY ADMIN
        // --------------------------------------------

        pqxx::result adminResult =
            transaction.exec_params(
                "SELECT role "
                "FROM users "
                "WHERE user_id = $1",
                adminId
            );


        if (
            adminResult.empty() ||
            adminResult[0]["role"].as<std::string>() != "admin"
        )
        {
            transaction.abort();

            Json::Value response;

            response["success"] = false;
            response["message"] =
                "Admin access required.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k403Forbidden);

            callback(result);
            return;
        }


        // --------------------------------------------
        // GET ORDERS
        // --------------------------------------------

        pqxx::result result =
            transaction.exec(
                "SELECT "
                "o.order_id, "
                "o.total_amount, "
                "o.status, "
                "o.created_at, "
                "u.user_id AS buyer_id, "
                "u.name AS buyer_name, "
                "u.email AS buyer_email, "
                "COUNT(oi.order_item_id) AS item_count "
                "FROM orders o "
                "LEFT JOIN users u "
                "ON o.user_id = u.user_id "
                "LEFT JOIN order_items oi "
                "ON o.order_id = oi.order_id "
                "GROUP BY "
                "o.order_id, "
                "o.total_amount, "
                "o.status, "
                "o.created_at, "
                "u.user_id, "
                "u.name, "
                "u.email "
                "ORDER BY o.created_at DESC"
            );


        Json::Value orders(
            Json::arrayValue
        );


        for (const auto& row : result)
        {
            Json::Value order;

            order["order_id"] =
                row["order_id"].as<int>();

            order["buyer_id"] =
                row["buyer_id"].is_null()
                    ? 0
                    : row["buyer_id"].as<int>();

            order["buyer_name"] =
                row["buyer_name"].is_null()
                    ? "Unknown"
                    : row["buyer_name"].as<std::string>();

            order["buyer_email"] =
                row["buyer_email"].is_null()
                    ? ""
                    : row["buyer_email"].as<std::string>();

            order["total_amount"] =
                row["total_amount"].is_null()
                    ? "0"
                    : row["total_amount"].as<std::string>();

            order["status"] =
                row["status"].is_null()
                    ? "Pending"
                    : row["status"].as<std::string>();

            order["item_count"] =
                row["item_count"].as<int>();

            order["created_at"] =
                row["created_at"].is_null()
                    ? ""
                    : row["created_at"].as<std::string>();

            orders.append(order);
        }


        transaction.commit();


        Json::Value response;

        response["success"] = true;

        response["total_orders"] =
            static_cast<int>(orders.size());

        response["orders"] =
            orders;


        callback(
            HttpResponse::newHttpJsonResponse(response)
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Admin orders error: "
            << e.what();

        Json::Value response;

        response["success"] = false;
        response["message"] =
            "Failed to load orders.";

        auto result =
            HttpResponse::newHttpJsonResponse(response);

        result->setStatusCode(k500InternalServerError);

        callback(result);
    }
}