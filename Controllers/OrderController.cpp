#include "OrderController.h"
#include "../Services/Database.h"

#include <drogon/drogon.h>
#include <pqxx/pqxx>
#include <json/json.h>

using namespace drogon;

// ============================================================
// PLACE ORDER
// ============================================================

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
            response["message"] = "Invalid request data.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k400BadRequest);
            callback(result);
            return;
        }

        // ----------------------------------------------------
        // Required fields
        // ----------------------------------------------------

        if (!json->isMember("user_id") ||
            !json->isMember("total_amount") ||
            !json->isMember("items"))
        {
            Json::Value response;
            response["success"] = false;
            response["message"] =
                "user_id, total_amount and items are required.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k400BadRequest);
            callback(result);
            return;
        }

        int userId =
            (*json)["user_id"].asInt();

        double totalAmount =
            (*json)["total_amount"].asDouble();

        const Json::Value& items =
            (*json)["items"];

        if (userId <= 0)
        {
            Json::Value response;
            response["success"] = false;
            response["message"] = "Invalid user ID.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k400BadRequest);
            callback(result);
            return;
        }

        if (!items.isArray() || items.empty())
        {
            Json::Value response;
            response["success"] = false;
            response["message"] = "Order must contain products.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k400BadRequest);
            callback(result);
            return;
        }

        // ----------------------------------------------------
        // Payment method
        // ----------------------------------------------------

        std::string paymentMethod = "COD";

        if (json->isMember("payment_method") &&
            !(*json)["payment_method"].isNull())
        {
            paymentMethod =
                (*json)["payment_method"].asString();
        }

        // ----------------------------------------------------
        // Delivery details
        // ----------------------------------------------------

        std::string deliveryName = "";
        std::string deliveryMobile = "";
        std::string deliveryAddress = "";
        std::string deliveryCity = "";
        std::string deliveryPincode = "";

        if (json->isMember("delivery_name"))
            deliveryName =
                (*json)["delivery_name"].asString();

        if (json->isMember("delivery_mobile"))
            deliveryMobile =
                (*json)["delivery_mobile"].asString();

        if (json->isMember("delivery_address"))
            deliveryAddress =
                (*json)["delivery_address"].asString();

        if (json->isMember("delivery_city"))
            deliveryCity =
                (*json)["delivery_city"].asString();

        if (json->isMember("delivery_pincode"))
            deliveryPincode =
                (*json)["delivery_pincode"].asString();

        auto db = Database::connect();

        pqxx::work transaction(*db);

        // ----------------------------------------------------
        // Check buyer exists
        // ----------------------------------------------------

        pqxx::result userCheck =
            transaction.exec_params(
                "SELECT user_id "
                "FROM users "
                "WHERE user_id = $1",
                userId
            );

        if (userCheck.empty())
        {
            Json::Value response;
            response["success"] = false;
            response["message"] = "User not found.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k404NotFound);
            callback(result);
            return;
        }

        // ----------------------------------------------------
        // Create order
        // ----------------------------------------------------

        pqxx::result orderResult =
            transaction.exec_params(
                "INSERT INTO orders "
                "(user_id, total_amount, status, payment_method, "
                "delivery_name, delivery_mobile, delivery_address, "
                "delivery_city, delivery_pincode) "
                "VALUES "
                "($1, $2, 'Pending', $3, $4, $5, $6, $7, $8) "
                "RETURNING order_id",
                userId,
                totalAmount,
                paymentMethod,
                deliveryName,
                deliveryMobile,
                deliveryAddress,
                deliveryCity,
                deliveryPincode
            );

        int orderId =
            orderResult[0]["order_id"].as<int>();

        // ----------------------------------------------------
        // Insert order items
        // ----------------------------------------------------

        for (const auto& item : items)
        {
            if (!item.isMember("product_id") ||
                !item.isMember("quantity"))
            {
                throw std::runtime_error(
                    "Invalid order item."
                );
            }

            int productId =
                item["product_id"].asInt();

            int quantity =
                item["quantity"].asInt();

            if (productId <= 0 || quantity <= 0)
            {
                throw std::runtime_error(
                    "Invalid product or quantity."
                );
            }

            // ------------------------------------------------
            // Get current product price
            // ------------------------------------------------

            pqxx::result productResult =
                transaction.exec_params(
                    "SELECT price "
                    "FROM products "
                    "WHERE product_id = $1",
                    productId
                );

            if (productResult.empty())
            {
                throw std::runtime_error(
                    "Product not found."
                );
            }

            double price =
                productResult[0]["price"].as<double>();

            double subtotal =
                price * quantity;

            // ------------------------------------------------
            // Insert item
            // ------------------------------------------------

            transaction.exec_params(
                "INSERT INTO order_items "
                "(order_id, product_id, quantity, price, subtotal) "
                "VALUES ($1, $2, $3, $4, $5)",
                orderId,
                productId,
                quantity,
                price,
                subtotal
            );
        }

        transaction.commit();

        // ----------------------------------------------------
        // Success response
        // ----------------------------------------------------

        Json::Value response;

        response["success"] = true;

        response["message"] =
            "Order placed successfully.";

        response["order_id"] =
            orderId;

        response["status"] =
            "Pending";

        response["payment_method"] =
            paymentMethod;

        response["total_amount"] =
            totalAmount;

        callback(
            HttpResponse::newHttpJsonResponse(response)
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Place order error: "
            << e.what();

        Json::Value response;

        response["success"] = false;

        response["message"] =
            "Failed to place order.";

        auto result =
            HttpResponse::newHttpJsonResponse(response);

        result->setStatusCode(
            k500InternalServerError
        );

        callback(result);
    }
}


// ============================================================
// BUYER - MY ORDERS
// ============================================================

void OrderController::getBuyerOrders(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    int userId
)
{
    try
    {
        if (userId <= 0)
        {
            Json::Value response;

            response["success"] = false;
            response["message"] =
                "Invalid user ID.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k400BadRequest);
            callback(result);
            return;
        }

        auto db = Database::connect();

        pqxx::work transaction(*db);

        // ----------------------------------------------------
        // Get all orders for this buyer
        // ----------------------------------------------------

        pqxx::result orders =
            transaction.exec_params(
                "SELECT "
                "o.order_id, "
                "o.total_amount, "
                "o.status, "
                "o.created_at, "
                "o.payment_method, "
                "o.delivery_name, "
                "o.delivery_mobile, "
                "o.delivery_address, "
                "o.delivery_city, "
                "o.delivery_pincode "
                "FROM orders o "
                "WHERE o.user_id = $1 "
                "ORDER BY o.created_at DESC",
                userId
            );

        Json::Value response;

        response["success"] = true;
        response["orders"] =
            Json::arrayValue;

        for (const auto& order : orders)
        {
            Json::Value orderJson;

            int orderId =
                order["order_id"].as<int>();

            orderJson["order_id"] =
                orderId;

            orderJson["total_amount"] =
                order["total_amount"].as<double>();

            orderJson["status"] =
                order["status"].as<std::string>();

            orderJson["created_at"] =
                order["created_at"].as<std::string>();

            // ------------------------------------------------
            // Payment method
            // ------------------------------------------------

            if (!order["payment_method"].is_null())
            {
                orderJson["payment_method"] =
                    order["payment_method"].as<std::string>();
            }
            else
            {
                orderJson["payment_method"] =
                    "COD";
            }

            // ------------------------------------------------
            // Delivery details
            // ------------------------------------------------

            if (!order["delivery_name"].is_null())
            {
                orderJson["delivery_name"] =
                    order["delivery_name"].as<std::string>();
            }
            else
            {
                orderJson["delivery_name"] = "";
            }

            if (!order["delivery_mobile"].is_null())
            {
                orderJson["delivery_mobile"] =
                    order["delivery_mobile"].as<std::string>();
            }
            else
            {
                orderJson["delivery_mobile"] = "";
            }

            if (!order["delivery_address"].is_null())
            {
                orderJson["delivery_address"] =
                    order["delivery_address"].as<std::string>();
            }
            else
            {
                orderJson["delivery_address"] = "";
            }

            if (!order["delivery_city"].is_null())
            {
                orderJson["delivery_city"] =
                    order["delivery_city"].as<std::string>();
            }
            else
            {
                orderJson["delivery_city"] = "";
            }

            if (!order["delivery_pincode"].is_null())
            {
                orderJson["delivery_pincode"] =
                    order["delivery_pincode"].as<std::string>();
            }
            else
            {
                orderJson["delivery_pincode"] = "";
            }

            // ------------------------------------------------
            // Get products in this order
            // ------------------------------------------------

            pqxx::result items =
                transaction.exec_params(
                    "SELECT "
                    "oi.order_item_id, "
                    "oi.product_id, "
                    "oi.quantity, "
                    "oi.price, "
                    "oi.subtotal, "
                    "p.product_name, "
                    "p.description, "
                    "p.image_path "
                    "FROM order_items oi "
                    "JOIN products p "
                    "ON oi.product_id = p.product_id "
                    "WHERE oi.order_id = $1 "
                    "ORDER BY oi.order_item_id",
                    orderId
                );

            orderJson["items"] =
                Json::arrayValue;

            for (const auto& item : items)
            {
                Json::Value itemJson;

                itemJson["order_item_id"] =
                    item["order_item_id"].as<int>();

                itemJson["product_id"] =
                    item["product_id"].as<int>();

                itemJson["product_name"] =
                    item["product_name"].as<std::string>();

                itemJson["quantity"] =
                    item["quantity"].as<int>();

                itemJson["price"] =
                    item["price"].as<double>();

                // ------------------------------------------------
                // Subtotal
                // ------------------------------------------------

                if (!item["subtotal"].is_null())
                {
                    itemJson["subtotal"] =
                        item["subtotal"].as<double>();
                }
                else
                {
                    itemJson["subtotal"] =
                        item["price"].as<double>() *
                        item["quantity"].as<int>();
                }

                // ------------------------------------------------
                // Description
                // ------------------------------------------------

                if (!item["description"].is_null())
                {
                    itemJson["description"] =
                        item["description"].as<std::string>();
                }
                else
                {
                    itemJson["description"] = "";
                }

                // ------------------------------------------------
                // Image
                // ------------------------------------------------

                if (!item["image_path"].is_null())
                {
                    itemJson["image_path"] =
                        item["image_path"].as<std::string>();
                }
                else
                {
                    itemJson["image_path"] = "";
                }

                orderJson["items"].append(itemJson);
            }

            response["orders"].append(orderJson);
        }

        transaction.commit();

        callback(
            HttpResponse::newHttpJsonResponse(response)
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Get buyer orders error: "
            << e.what();

        Json::Value response;

        response["success"] = false;

        response["message"] =
            "Failed to load buyer orders.";

        auto result =
            HttpResponse::newHttpJsonResponse(response);

        result->setStatusCode(
            k500InternalServerError
        );

        callback(result);
    }
}


// ============================================================
// BUYER - CONFIRM DELIVERY
// ============================================================

void OrderController::confirmDelivery(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    int orderId
)
{
    try
    {
        if (orderId <= 0)
        {
            Json::Value response;

            response["success"] = false;
            response["message"] =
                "Invalid order ID.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k400BadRequest);
            callback(result);
            return;
        }

        auto json = req->getJsonObject();

        if (!json ||
            !json->isMember("user_id"))
        {
            Json::Value response;

            response["success"] = false;
            response["message"] =
                "User ID is required.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k400BadRequest);
            callback(result);
            return;
        }

        int userId =
            (*json)["user_id"].asInt();

        if (userId <= 0)
        {
            Json::Value response;

            response["success"] = false;
            response["message"] =
                "Invalid user ID.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k400BadRequest);
            callback(result);
            return;
        }

        auto db = Database::connect();

        pqxx::work transaction(*db);

        // ----------------------------------------------------
        // Check order belongs to buyer
        // ----------------------------------------------------

        pqxx::result check =
            transaction.exec_params(
                "SELECT order_id, status, created_at "
                "FROM orders "
                "WHERE order_id = $1 "
                "AND user_id = $2",
                orderId,
                userId
            );

        if (check.empty())
        {
            Json::Value response;

            response["success"] = false;
            response["message"] =
                "Order not found.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k404NotFound);
            callback(result);
            return;
        }

        std::string status =
            check[0]["status"].as<std::string>();

        if (status == "Delivered")
        {
            Json::Value response;

            response["success"] = true;
            response["message"] =
                "Order is already marked as Delivered.";

            response["status"] =
                "Delivered";

            callback(
                HttpResponse::newHttpJsonResponse(response)
            );

            return;
        }

        // ----------------------------------------------------
        // Change Pending -> Delivered
        // ----------------------------------------------------

        transaction.exec_params(
            "UPDATE orders "
            "SET status = 'Delivered' "
            "WHERE order_id = $1 "
            "AND user_id = $2",
            orderId,
            userId
        );

        transaction.commit();

        Json::Value response;

        response["success"] = true;

        response["message"] =
            "Order marked as Delivered.";

        response["order_id"] =
            orderId;

        response["status"] =
            "Delivered";

        callback(
            HttpResponse::newHttpJsonResponse(response)
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Confirm delivery error: "
            << e.what();

        Json::Value response;

        response["success"] = false;

        response["message"] =
            "Failed to update delivery status.";

        auto result =
            HttpResponse::newHttpJsonResponse(response);

        result->setStatusCode(
            k500InternalServerError
        );

        callback(result);
    }
}


// ============================================================
// SELLER - ORDERS / SALES
// ============================================================

void OrderController::getSellerOrders(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    int sellerId
)
{
    try
    {
        if (sellerId <= 0)
        {
            Json::Value response;

            response["success"] = false;
            response["message"] =
                "Invalid seller ID.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k400BadRequest);
            callback(result);
            return;
        }

        auto db = Database::connect();

        pqxx::work transaction(*db);

        // ----------------------------------------------------
        // Get orders containing seller's products
        // ----------------------------------------------------

        pqxx::result result =
            transaction.exec_params(
                "SELECT "
                "o.order_id, "
                "o.user_id, "
                "o.total_amount, "
                "o.status, "
                "o.created_at, "
                "o.payment_method, "
                "o.delivery_name, "
                "o.delivery_mobile, "
                "o.delivery_address, "
                "o.delivery_city, "
                "o.delivery_pincode, "
                "oi.order_item_id, "
                "oi.product_id, "
                "oi.quantity, "
                "oi.price, "
                "oi.subtotal, "
                "p.product_name, "
                "p.description "
                "FROM orders o "
                "JOIN order_items oi "
                "ON o.order_id = oi.order_id "
                "JOIN products p "
                "ON oi.product_id = p.product_id "
                "WHERE p.seller_id = $1 "
                "ORDER BY o.created_at DESC, "
                "oi.order_item_id",
                sellerId
            );

        Json::Value response;

        response["success"] = true;
        response["orders"] =
            Json::arrayValue;

        int currentOrderId = -1;
        Json::Value currentOrder;

        for (const auto& row : result)
        {
            int orderId =
                row["order_id"].as<int>();

            // ------------------------------------------------
            // New order
            // ------------------------------------------------

            if (currentOrderId != orderId)
            {
                if (currentOrderId != -1)
                {
                    response["orders"].append(
                        currentOrder
                    );
                }

                currentOrderId = orderId;

                currentOrder =
                    Json::Value(Json::objectValue);

                currentOrder["order_id"] =
                    orderId;

                currentOrder["user_id"] =
                    row["user_id"].as<int>();

                currentOrder["total_amount"] =
                    row["total_amount"].as<double>();

                currentOrder["status"] =
                    row["status"].as<std::string>();

                currentOrder["created_at"] =
                    row["created_at"].as<std::string>();

                // --------------------------------------------
                // Payment
                // --------------------------------------------

                if (!row["payment_method"].is_null())
                {
                    currentOrder["payment_method"] =
                        row["payment_method"].as<std::string>();
                }
                else
                {
                    currentOrder["payment_method"] =
                        "COD";
                }

                // --------------------------------------------
                // Delivery
                // --------------------------------------------

                if (!row["delivery_name"].is_null())
                {
                    currentOrder["delivery_name"] =
                        row["delivery_name"].as<std::string>();
                }
                else
                {
                    currentOrder["delivery_name"] = "";
                }

                if (!row["delivery_mobile"].is_null())
                {
                    currentOrder["delivery_mobile"] =
                        row["delivery_mobile"].as<std::string>();
                }
                else
                {
                    currentOrder["delivery_mobile"] = "";
                }

                if (!row["delivery_address"].is_null())
                {
                    currentOrder["delivery_address"] =
                        row["delivery_address"].as<std::string>();
                }
                else
                {
                    currentOrder["delivery_address"] = "";
                }

                if (!row["delivery_city"].is_null())
                {
                    currentOrder["delivery_city"] =
                        row["delivery_city"].as<std::string>();
                }
                else
                {
                    currentOrder["delivery_city"] = "";
                }

                if (!row["delivery_pincode"].is_null())
                {
                    currentOrder["delivery_pincode"] =
                        row["delivery_pincode"].as<std::string>();
                }
                else
                {
                    currentOrder["delivery_pincode"] = "";
                }

                currentOrder["items"] =
                    Json::arrayValue;
            }

            // ------------------------------------------------
            // Item
            // ------------------------------------------------

            Json::Value item;

            item["order_item_id"] =
                row["order_item_id"].as<int>();

            item["product_id"] =
                row["product_id"].as<int>();

            item["product_name"] =
                row["product_name"].as<std::string>();

            item["quantity"] =
                row["quantity"].as<int>();

            item["price"] =
                row["price"].as<double>();

            if (!row["subtotal"].is_null())
            {
                item["subtotal"] =
                    row["subtotal"].as<double>();
            }
            else
            {
                item["subtotal"] =
                    row["price"].as<double>() *
                    row["quantity"].as<int>();
            }

            if (!row["description"].is_null())
            {
                item["description"] =
                    row["description"].as<std::string>();
            }
            else
            {
                item["description"] = "";
            }

            currentOrder["items"].append(item);
        }

        // ----------------------------------------------------
        // Add final order
        // ----------------------------------------------------

        if (currentOrderId != -1)
        {
            response["orders"].append(
                currentOrder
            );
        }

        transaction.commit();

        callback(
            HttpResponse::newHttpJsonResponse(response)
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Get seller orders error: "
            << e.what();

        Json::Value response;

        response["success"] = false;

        response["message"] =
            "Failed to load seller orders.";

        auto responseHttp =
            HttpResponse::newHttpJsonResponse(response);

        responseHttp->setStatusCode(
            k500InternalServerError
        );

        callback(responseHttp);
    }
}