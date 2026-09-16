#include "OrderController.h"
#include "../Services/Database.h"

#include <drogon/drogon.h>
#include <pqxx/pqxx>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

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
            response["message"] = "Invalid JSON request.";

            auto result = HttpResponse::newHttpJsonResponse(response);
            result->setStatusCode(k400BadRequest);
            callback(result);
            return;
        }


        // ----------------------------------------------------
        // USER ID
        // ----------------------------------------------------

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


        // ----------------------------------------------------
        // ITEMS
        // ----------------------------------------------------

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


        // ----------------------------------------------------
        // DELIVERY INFORMATION
        // ----------------------------------------------------

        std::string paymentMethod =
            (*json).get("payment_method", "COD").asString();

        std::string deliveryName =
            (*json).get("delivery_name", "").asString();

        std::string deliveryMobile =
            (*json).get("delivery_mobile", "").asString();

        std::string deliveryAddress =
            (*json).get("delivery_address", "").asString();

        std::string deliveryCity =
            (*json).get("delivery_city", "").asString();

        std::string deliveryPincode =
            (*json).get("delivery_pincode", "").asString();


        if (deliveryName.empty() ||
            deliveryMobile.empty() ||
            deliveryAddress.empty() ||
            deliveryCity.empty() ||
            deliveryPincode.empty())
        {
            Json::Value response;
            response["success"] = false;
            response["message"] =
                "Complete delivery information is required.";

            auto result = HttpResponse::newHttpJsonResponse(response);
            result->setStatusCode(k400BadRequest);
            callback(result);
            return;
        }


        // ----------------------------------------------------
        // DATABASE
        // ----------------------------------------------------

        auto db = Database::connect();

        if (!db || !db->is_open())
        {
            Json::Value response;
            response["success"] = false;
            response["message"] = "Database connection failed.";

            auto result = HttpResponse::newHttpJsonResponse(response);
            result->setStatusCode(k500InternalServerError);
            callback(result);
            return;
        }


        pqxx::work transaction(*db);


        // ----------------------------------------------------
        // CHECK USER
        // ----------------------------------------------------

        pqxx::result userCheck =
            transaction.exec_params(
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


        // ----------------------------------------------------
        // CALCULATE TOTAL
        // ----------------------------------------------------

        double totalAmount = 0.0;

        for (const auto& item : (*json)["items"])
        {
            int productId =
                item["product_id"].asInt();

            int quantity =
                item["quantity"].asInt();


            if (productId <= 0 || quantity <= 0)
            {
                Json::Value response;
                response["success"] = false;
                response["message"] =
                    "Invalid product or quantity.";

                auto result =
                    HttpResponse::newHttpJsonResponse(response);

                result->setStatusCode(k400BadRequest);
                callback(result);
                return;
            }


            pqxx::result productResult =
                transaction.exec_params(
                    "SELECT price FROM products "
                    "WHERE product_id = $1",
                    productId
                );


            if (productResult.empty())
            {
                Json::Value response;
                response["success"] = false;
                response["message"] =
                    "Product not found.";

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


        // ----------------------------------------------------
        // CREATE ORDER
        // ----------------------------------------------------

        pqxx::result orderResult =
            transaction.exec_params(
                "INSERT INTO orders "
                "(user_id, total_amount, status, "
                "payment_method, delivery_name, "
                "delivery_mobile, delivery_address, "
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
        // INSERT ORDER ITEMS
        // ----------------------------------------------------

        for (const auto& item : (*json)["items"])
        {
            int productId =
                item["product_id"].asInt();

            int quantity =
                item["quantity"].asInt();


            pqxx::result productResult =
                transaction.exec_params(
                    "SELECT price FROM products "
                    "WHERE product_id = $1",
                    productId
                );


            double price =
                productResult[0]["price"].as<double>();


            double subtotal =
                price * quantity;


            transaction.exec_params(
                "INSERT INTO order_items "
                "(order_id, product_id, quantity, "
                "price, subtotal) "
                "VALUES ($1, $2, $3, $4, $5)",

                orderId,
                productId,
                quantity,
                price,
                subtotal
            );
        }


        // ----------------------------------------------------
        // COMMIT
        // ----------------------------------------------------

        transaction.commit();


        // ----------------------------------------------------
        // RESPONSE
        // ----------------------------------------------------

        Json::Value response;

        response["success"] = true;
        response["message"] =
            "Order placed successfully.";

        response["order_id"] =
            orderId;

        response["total_amount"] =
            totalAmount;

        response["status"] =
            "Pending";

        response["payment_method"] =
            paymentMethod;


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
            response["message"] = "Invalid user ID.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k400BadRequest);
            callback(result);
            return;
        }

        auto db = Database::connect();

        pqxx::work transaction(*db);

        // ----------------------------------------------------
        // Get all orders + products for this buyer
        // ----------------------------------------------------

        pqxx::result orders =
            transaction.exec_params(

                "SELECT "
                "o.order_id, "
                "o.total_amount, "
                "o.status, "
                "o.payment_method, "
                "o.delivery_name, "
                "o.delivery_mobile, "
                "o.delivery_address, "
                "o.delivery_city, "
                "o.delivery_pincode, "
                "o.created_at, "

                "oi.order_item_id, "
                "oi.product_id, "
                "oi.quantity, "
                "oi.price, "
                "oi.subtotal, "

                "p.product_name, "
                "p.description, "
                "p.image_path "

                "FROM orders o "

                "JOIN order_items oi "
                "ON o.order_id = oi.order_id "

                "JOIN products p "
                "ON oi.product_id = p.product_id "

                "WHERE o.user_id = $1 "

                "ORDER BY o.created_at DESC, "
                "o.order_id DESC",

                userId
            );

        Json::Value response;

        response["success"] = true;
        response["orders"] = Json::arrayValue;

        // ----------------------------------------------------
        // Group order items under each order
        // ----------------------------------------------------

        int currentOrderId = -1;
        Json::Value* currentOrder = nullptr;

        for (const auto& row : orders)
        {
            int orderId =
                row["order_id"].as<int>();

            // ------------------------------------------------
            // Create a new order
            // ------------------------------------------------

            if (orderId != currentOrderId)
            {
                Json::Value order;

                order["order_id"] =
                    orderId;

                order["total_amount"] =
                    row["total_amount"].as<double>();

                // Status should normally not be NULL
                if (row["status"].is_null())
                {
                    order["status"] = "Pending";
                }
                else
                {
                    order["status"] =
                        row["status"].as<std::string>();
                }

                // Payment method can be NULL for older orders
                if (row["payment_method"].is_null())
                {
                    order["payment_method"] = "COD";
                }
                else
                {
                    order["payment_method"] =
                        row["payment_method"].as<std::string>();
                }

                // Delivery details can be NULL
                if (row["delivery_name"].is_null())
                {
                    order["delivery_name"] = "";
                }
                else
                {
                    order["delivery_name"] =
                        row["delivery_name"].as<std::string>();
                }

                if (row["delivery_mobile"].is_null())
                {
                    order["delivery_mobile"] = "";
                }
                else
                {
                    order["delivery_mobile"] =
                        row["delivery_mobile"].as<std::string>();
                }

                if (row["delivery_address"].is_null())
                {
                    order["delivery_address"] = "";
                }
                else
                {
                    order["delivery_address"] =
                        row["delivery_address"].as<std::string>();
                }

                if (row["delivery_city"].is_null())
                {
                    order["delivery_city"] = "";
                }
                else
                {
                    order["delivery_city"] =
                        row["delivery_city"].as<std::string>();
                }

                if (row["delivery_pincode"].is_null())
                {
                    order["delivery_pincode"] = "";
                }
                else
                {
                    order["delivery_pincode"] =
                        row["delivery_pincode"].as<std::string>();
                }

                order["created_at"] =
                    row["created_at"].as<std::string>();

                order["items"] =
                    Json::arrayValue;

                response["orders"].append(order);

                currentOrderId = orderId;

                currentOrder =
                    &response["orders"][
                        static_cast<Json::ArrayIndex>(
                            response["orders"].size() - 1
                        )
                    ];
            }

            // ------------------------------------------------
            // Add product item to the current order
            // ------------------------------------------------

            Json::Value item;

            item["order_item_id"] =
                row["order_item_id"].as<int>();

            item["product_id"] =
                row["product_id"].as<int>();

            item["product_name"] =
                row["product_name"].as<std::string>();

            // Description can be NULL
            if (row["description"].is_null())
            {
                item["description"] = "";
            }
            else
            {
                item["description"] =
                    row["description"].as<std::string>();
            }

            // Image path can be NULL
            if (row["image_path"].is_null())
            {
                item["image_path"] = "";
            }
            else
            {
                item["image_path"] =
                    row["image_path"].as<std::string>();
            }

            item["quantity"] =
                row["quantity"].as<int>();

            item["price"] =
                row["price"].as<double>();

            // Subtotal can be NULL for older orders
            if (row["subtotal"].is_null())
            {
                item["subtotal"] =
                    row["price"].as<double>() *
                    row["quantity"].as<int>();
            }
            else
            {
                item["subtotal"] =
                    row["subtotal"].as<double>();
            }

            (*currentOrder)["items"].append(item);
        }

        // ----------------------------------------------------
        // Finish database transaction
        // ----------------------------------------------------

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
            "Failed to load orders.";

        auto result =
            HttpResponse::newHttpJsonResponse(response);

        result->setStatusCode(
            k500InternalServerError
        );

        callback(result);
    }
}
        // ----------------------------------------------------
        // Get all orders + products for this buyer
        // ----------------------------------------------------

        pqxx::result orders =
            transaction.exec_params(

                "SELECT "
                "o.order_id, "
                "o.total_amount, "
                "o.status, "
                "o.payment_method, "
                "o.delivery_name, "
                "o.delivery_mobile, "
                "o.delivery_address, "
                "o.delivery_city, "
                "o.delivery_pincode, "
                "o.created_at, "

                "oi.order_item_id, "
                "oi.product_id, "
                "oi.quantity, "
                "oi.price, "
                "oi.subtotal, "

                "p.product_name, "
                "p.description, "
                "p.image_path "

                "FROM orders o "

                "JOIN order_items oi "
                "ON o.order_id = oi.order_id "

                "JOIN products p "
                "ON oi.product_id = p.product_id "

                "WHERE o.user_id = $1 "

                "ORDER BY o.created_at DESC, "
                "o.order_id DESC",

                userId
            );


        Json::Value response;

        response["success"] = true;
        response["orders"] =
            Json::arrayValue;


        // ----------------------------------------------------
        // Group order items under each order
        // ----------------------------------------------------

        int currentOrderId = -1;
        Json::Value* currentOrder = nullptr;


        for (const auto& row : orders)
        {
            int orderId =
                row["order_id"].as<int>();


            if (orderId != currentOrderId)
            {
                Json::Value order;

                order["order_id"] =
                    orderId;

                order["total_amount"] =
                    row["total_amount"].as<double>();

                order["status"] =
                    row["status"].as<std::string>();

                order["payment_method"] =
                    row["payment_method"].as<std::string>();

                order["delivery_name"] =
                    row["delivery_name"].as<std::string>();

                order["delivery_mobile"] =
                    row["delivery_mobile"].as<std::string>();

                order["delivery_address"] =
                    row["delivery_address"].as<std::string>();

                order["delivery_city"] =
                    row["delivery_city"].as<std::string>();

                order["delivery_pincode"] =
                    row["delivery_pincode"].as<std::string>();

                order["created_at"] =
                    row["created_at"].as<std::string>();

                order["items"] =
                    Json::arrayValue;


                response["orders"].append(order);

                currentOrderId = orderId;

                currentOrder =
                    &response["orders"][
                        static_cast<Json::ArrayIndex>(
                            response["orders"].size() - 1
                        )
                    ];
            }


          Json::Value item;

item["order_item_id"] =
    row["order_item_id"].as<int>();

item["product_id"] =
    row["product_id"].as<int>();

item["product_name"] =
    row["product_name"].as<std::string>();

// Description can be NULL
if (row["description"].is_null())
{
    item["description"] = "";
}
else
{
    item["description"] =
        row["description"].as<std::string>();
}

// Image path can be NULL
if (row["image_path"].is_null())
{
    item["image_path"] = "";
}
else
{
    item["image_path"] =
        row["image_path"].as<std::string>();
}

item["quantity"] =
    row["quantity"].as<int>();

item["price"] =
    row["price"].as<double>();

// Subtotal can be NULL for older orders
if (row["subtotal"].is_null())
{
    item["subtotal"] =
        row["price"].as<double>() *
        row["quantity"].as<int>();
}
else
{
    item["subtotal"] =
        row["subtotal"].as<double>();
}

(*currentOrder)["items"].append(item);

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
            "Failed to load orders.";

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
        // Change Pending → Delivered
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

        pqxx::result orders =
            transaction.exec_params(

                "SELECT "
                "o.order_id, "
                "o.user_id, "
                "o.total_amount, "
                "o.status, "
                "o.payment_method, "
                "o.delivery_name, "
                "o.delivery_mobile, "
                "o.delivery_address, "
                "o.delivery_city, "
                "o.delivery_pincode, "
                "o.created_at, "

                "u.name AS buyer_name, "
                "u.email AS buyer_email, "

                "oi.order_item_id, "
                "oi.product_id, "
                "oi.quantity, "
                "oi.price, "
                "oi.subtotal, "

                "p.product_name, "
                "p.description "

                "FROM orders o "

                "JOIN users u "
                "ON o.user_id = u.user_id "

                "JOIN order_items oi "
                "ON o.order_id = oi.order_id "

                "JOIN products p "
                "ON oi.product_id = p.product_id "

                "WHERE p.seller_id = $1 "

                "ORDER BY o.created_at DESC, "
                "o.order_id DESC",

                sellerId
            );


        Json::Value response;

        response["success"] = true;

        response["orders"] =
            Json::arrayValue;


        for (const auto& row : orders)
        {
            Json::Value order;

            order["order_id"] =
                row["order_id"].as<int>();

            order["user_id"] =
                row["user_id"].as<int>();

            order["buyer_name"] =
                row["buyer_name"].as<std::string>();

            order["buyer_email"] =
                row["buyer_email"].as<std::string>();

            order["product_id"] =
                row["product_id"].as<int>();

            order["product_name"] =
                row["product_name"].as<std::string>();

            order["description"] =
                row["description"].as<std::string>();

            order["quantity"] =
                row["quantity"].as<int>();

            order["price"] =
                row["price"].as<double>();

            order["subtotal"] =
                row["subtotal"].as<double>();

            order["total_amount"] =
                row["total_amount"].as<double>();

            order["status"] =
                row["status"].as<std::string>();

            order["payment_method"] =
                row["payment_method"].as<std::string>();

            order["delivery_name"] =
                row["delivery_name"].as<std::string>();

            order["delivery_mobile"] =
                row["delivery_mobile"].as<std::string>();

            order["delivery_address"] =
                row["delivery_address"].as<std::string>();

            order["delivery_city"] =
                row["delivery_city"].as<std::string>();

            order["delivery_pincode"] =
                row["delivery_pincode"].as<std::string>();

            order["created_at"] =
                row["created_at"].as<std::string>();


            response["orders"].append(order);
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

        auto result =
            HttpResponse::newHttpJsonResponse(response);

        result->setStatusCode(
            k500InternalServerError
        );

        callback(result);
    }
}
