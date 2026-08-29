#include "OrderController.h"
#include "../Services/Database.h"

#include <drogon/drogon.h>
#include <pqxx/pqxx>

using namespace drogon;

void OrderController::placeOrder(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback
)
{
    try
    {
        auto json = req->getJsonObject();

        if (!json)
        {
            Json::Value response;
            response["success"] = false;
            response["message"] = "Invalid JSON request.";

            auto result = HttpResponse::newHttpJsonResponse(response);
            result->setStatusCode(k400BadRequest);
            callback(result);
            return;
        }

        int userId = (*json)["user_id"].asInt();

        if (userId <= 0)
        {
            Json::Value response;
            response["success"] = false;
            response["message"] = "Invalid user ID.";

            auto result = HttpResponse::newHttpJsonResponse(response);
            result->setStatusCode(k400BadRequest);
            callback(result);
            return;
        }

        if (!(*json).isMember("items") ||
            !(*json)["items"].isArray() ||
            (*json)["items"].empty())
        {
            Json::Value response;
            response["success"] = false;
            response["message"] = "Order items are required.";

            auto result = HttpResponse::newHttpJsonResponse(response);
            result->setStatusCode(k400BadRequest);
            callback(result);
            return;
        }

        auto db = Database::connect();
        pqxx::work transaction(*db);

        double totalAmount = 0.0;

        // Check that the user exists
        pqxx::result userCheck = transaction.exec_params(
            "SELECT user_id FROM users WHERE user_id = $1",
            userId
        );

        if (userCheck.empty())
        {
            Json::Value response;
            response["success"] = false;
            response["message"] = "User not found.";

            auto result = HttpResponse::newHttpJsonResponse(response);
            result->setStatusCode(k404NotFound);
            callback(result);
            return;
        }

        // Calculate total
        for (const auto& item : (*json)["items"])
        {
            int productId = item["product_id"].asInt();
            int quantity = item["quantity"].asInt();

            if (productId <= 0 || quantity <= 0)
            {
                Json::Value response;
                response["success"] = false;
                response["message"] = "Invalid product or quantity.";

                auto result =
                    HttpResponse::newHttpJsonResponse(response);

                result->setStatusCode(k400BadRequest);
                callback(result);
                return;
            }

            pqxx::result productResult = transaction.exec_params(
                "SELECT price FROM products WHERE product_id = $1",
                productId
            );

            if (productResult.empty())
            {
                Json::Value response;
                response["success"] = false;
                response["message"] = "Product not found.";

                auto result =
                    HttpResponse::newHttpJsonResponse(response);

                result->setStatusCode(k404NotFound);
                callback(result);
                return;
            }

            double price =
                productResult[0]["price"].as<double>();

            totalAmount += price * quantity;
        }

        // Create order
        pqxx::result orderResult = transaction.exec_params(
            "INSERT INTO orders "
            "(user_id, total_amount) "
            "VALUES ($1, $2) "
            "RETURNING order_id",
            userId,
            totalAmount
        );

        int orderId =
            orderResult[0]["order_id"].as<int>();

        // Insert order items
        for (const auto& item : (*json)["items"])
        {
            int productId = item["product_id"].asInt();
            int quantity = item["quantity"].asInt();

            pqxx::result productResult = transaction.exec_params(
                "SELECT price FROM products WHERE product_id = $1",
                productId
            );

            double price =
                productResult[0]["price"].as<double>();

            transaction.exec_params(
                "INSERT INTO order_items "
                "(order_id, product_id, quantity, price) "
                "VALUES ($1, $2, $3, $4)",
                orderId,
                productId,
                quantity,
                price
            );
        }

        transaction.commit();

        Json::Value response;

        response["success"] = true;
        response["message"] = "Order placed successfully.";
        response["order_id"] = orderId;
        response["total_amount"] = totalAmount;

        callback(
            HttpResponse::newHttpJsonResponse(response)
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Place order error: " << e.what();

        Json::Value response;

        response["success"] = false;
        response["message"] = "Failed to place order.";

        auto result =
            HttpResponse::newHttpJsonResponse(response);

        result->setStatusCode(k500InternalServerError);

        callback(result);
    }
}