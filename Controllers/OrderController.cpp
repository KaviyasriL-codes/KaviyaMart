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
            response["message"] =
                "Order must contain products.";

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
        {
            deliveryName =
                (*json)["delivery_name"].asString();
        }

        if (json->isMember("delivery_mobile"))
        {
            deliveryMobile =
                (*json)["delivery_mobile"].asString();
        }

        if (json->isMember("delivery_address"))
        {
            deliveryAddress =
                (*json)["delivery_address"].asString();
        }

        if (json->isMember("delivery_city"))
        {
            deliveryCity =
                (*json)["delivery_city"].asString();
        }

        if (json->isMember("delivery_pincode"))
        {
            deliveryPincode =
                (*json)["delivery_pincode"].asString();
        }

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
        //
        // IMPORTANT:
        // Every product starts as Pending.
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

            // ------------------------------------------------
            // Insert item with Pending status
            // ------------------------------------------------

            transaction.exec_params(
                "INSERT INTO order_items "
                "(order_id, product_id, quantity, price, status) "
                "VALUES ($1, $2, $3, $4, 'Pending')",
                orderId,
                productId,
                quantity,
                price
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

            // ------------------------------------------------
            // Keep old order-level status for compatibility.
            //
            // IMPORTANT:
            // The actual product delivery status is now
            // item["status"] below.
            // ------------------------------------------------

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
                    "oi.status, "
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
                // ITEM DELIVERY STATUS
                // ------------------------------------------------

                if (!item["status"].is_null())
                {
                    itemJson["status"] =
                        item["status"].as<std::string>();
                }
                else
                {
                    itemJson["status"] =
                        "Pending";
                }

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
// SELLER - VIEW ORDERS
// ============================================================

void OrderController::getSellerOrders(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    int sellerId
)
{
    try
    {
        // ----------------------------------------------------
        // Validate seller ID
        // ----------------------------------------------------

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
        // Get orders containing this seller's products
        //
        // orders
        //      ↓
        // order_items
        //      ↓
        // products
        //      ↓
        // users
        //
        // p.seller_id = sellerId
        // means seller sees only their own products.
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

                // Customer information
                "u.name AS customer_name, "
                "u.email AS customer_email, "
                "u.mobile AS customer_mobile, "

                // Delivery information
                "o.delivery_name, "
                "o.delivery_mobile, "
                "o.delivery_address, "
                "o.delivery_city, "
                "o.delivery_pincode, "

                // Product/order item information
                "oi.order_item_id, "
                "oi.product_id, "
                "oi.quantity, "
                "oi.price, "
                "oi.subtotal, "
                "oi.status AS item_status, "
                "p.product_name, "
                "p.description, "
                "p.image_path "

                "FROM orders o "

                "JOIN order_items oi "
                "ON o.order_id = oi.order_id "

                "JOIN products p "
                "ON oi.product_id = p.product_id "

                "JOIN users u "
                "ON o.user_id = u.user_id "

                "WHERE p.seller_id = $1 "

                "ORDER BY o.created_at DESC, "
                "oi.order_item_id",
                sellerId
            );

        // ----------------------------------------------------
        // Response
        // ----------------------------------------------------

        Json::Value response;

        response["success"] = true;
        response["orders"] =
            Json::arrayValue;

        int currentOrderId = -1;

        Json::Value currentOrder;

        // ----------------------------------------------------
        // Build grouped orders
        // ----------------------------------------------------

        for (const auto& row : result)
        {
            int orderId =
                row["order_id"].as<int>();

            // ------------------------------------------------
            // New order
            // ------------------------------------------------

            if (currentOrderId != orderId)
            {
                // Save previous order
                if (currentOrderId != -1)
                {
                    response["orders"].append(
                        currentOrder
                    );
                }

                currentOrderId = orderId;

                currentOrder =
                    Json::Value(Json::objectValue);

                // --------------------------------------------
                // Order information
                // --------------------------------------------

                currentOrder["order_id"] =
                    orderId;

                currentOrder["user_id"] =
                    row["user_id"].as<int>();

                currentOrder["customer_id"] =
                    row["user_id"].as<int>();

                // --------------------------------------------
                // Customer name
                // --------------------------------------------

                if (!row["customer_name"].is_null())
                {
                    currentOrder["customer_name"] =
                        row["customer_name"].as<std::string>();
                }
                else
                {
                    currentOrder["customer_name"] = "";
                }

                // --------------------------------------------
                // Customer email
                // --------------------------------------------

                if (!row["customer_email"].is_null())
                {
                    currentOrder["customer_email"] =
                        row["customer_email"].as<std::string>();
                }
                else
                {
                    currentOrder["customer_email"] = "";
                }

                // --------------------------------------------
                // Customer mobile
                // --------------------------------------------

                if (!row["customer_mobile"].is_null())
                {
                    currentOrder["customer_mobile"] =
                        row["customer_mobile"].as<std::string>();
                }
                else
                {
                    currentOrder["customer_mobile"] = "";
                }

                // --------------------------------------------
                // Order total
                // --------------------------------------------

                currentOrder["total_amount"] =
                    row["total_amount"].as<double>();

                // --------------------------------------------
                // Keep order-level status for compatibility
                // --------------------------------------------

                currentOrder["status"] =
                    row["status"].as<std::string>();

                // --------------------------------------------
                // Created date
                // --------------------------------------------

                currentOrder["created_at"] =
                    row["created_at"].as<std::string>();

                // --------------------------------------------
                // Payment method
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
                // Delivery name
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

                // --------------------------------------------
                // Delivery mobile
                // --------------------------------------------

                if (!row["delivery_mobile"].is_null())
                {
                    currentOrder["delivery_mobile"] =
                        row["delivery_mobile"].as<std::string>();
                }
                else
                {
                    currentOrder["delivery_mobile"] = "";
                }

                // --------------------------------------------
                // Delivery address
                // --------------------------------------------

                if (!row["delivery_address"].is_null())
                {
                    currentOrder["delivery_address"] =
                        row["delivery_address"].as<std::string>();
                }
                else
                {
                    currentOrder["delivery_address"] = "";
                }

                // --------------------------------------------
                // Delivery city
                // --------------------------------------------

                if (!row["delivery_city"].is_null())
                {
                    currentOrder["delivery_city"] =
                        row["delivery_city"].as<std::string>();
                }
                else
                {
                    currentOrder["delivery_city"] = "";
                }

                // --------------------------------------------
                // Delivery PIN
                // --------------------------------------------

                if (!row["delivery_pincode"].is_null())
                {
                    currentOrder["delivery_pincode"] =
                        row["delivery_pincode"].as<std::string>();
                }
                else
                {
                    currentOrder["delivery_pincode"] = "";
                }

                // --------------------------------------------
                // Items array
                // --------------------------------------------

                currentOrder["items"] =
                    Json::arrayValue;
            }

            // ------------------------------------------------
            // Product item
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

            // ------------------------------------------------
            // ITEM DELIVERY STATUS
            // ------------------------------------------------

            if (!row["item_status"].is_null())
            {
                item["status"] =
                    row["item_status"].as<std::string>();
            }
            else
            {
                item["status"] =
                    "Pending";
            }

            // ------------------------------------------------
            // Subtotal
            // ------------------------------------------------

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

            // ------------------------------------------------
            // Description
            // ------------------------------------------------

            if (!row["description"].is_null())
            {
                item["description"] =
                    row["description"].as<std::string>();
            }
            else
            {
                item["description"] = "";
            }

            // ------------------------------------------------
            // Image
            // ------------------------------------------------

            if (!row["image_path"].is_null())
            {
                item["image_path"] =
                    row["image_path"].as<std::string>();
            }
            else
            {
                item["image_path"] = "";
            }

            // ------------------------------------------------
            // Add item to current order
            // ------------------------------------------------

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

        // ----------------------------------------------------
        // Send response
        // ----------------------------------------------------

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


// ============================================================
// SELLER - MARK ONE PRODUCT AS DELIVERED
// ============================================================

void OrderController::markItemDelivered(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback,
    int orderItemId
)
{
    try
    {
        // ----------------------------------------------------
        // Validate order item ID
        // ----------------------------------------------------

        if (orderItemId <= 0)
        {
            Json::Value response;

            response["success"] = false;
            response["message"] =
                "Invalid order item ID.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k400BadRequest);

            callback(result);
            return;
        }

        // ----------------------------------------------------
        // Request JSON
        //
        // Seller frontend sends:
        //
        // {
        //     "seller_id": 2
        // }
        // ----------------------------------------------------

        auto json = req->getJsonObject();

        if (!json ||
            !json->isMember("seller_id"))
        {
            Json::Value response;

            response["success"] = false;
            response["message"] =
                "seller_id is required.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k400BadRequest);

            callback(result);
            return;
        }

        int sellerId =
            (*json)["seller_id"].asInt();

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
        // Check that this product/order item actually belongs
        // to this seller.
        //
        // This prevents Seller A from changing Seller B's
        // product status.
        // ----------------------------------------------------

        pqxx::result check =
            transaction.exec_params(
                "SELECT "
                "oi.order_item_id, "
                "oi.status, "
                "p.seller_id, "
                "p.product_name "
                "FROM order_items oi "
                "JOIN products p "
                "ON oi.product_id = p.product_id "
                "WHERE oi.order_item_id = $1 "
                "AND p.seller_id = $2",
                orderItemId,
                sellerId
            );

        if (check.empty())
        {
            Json::Value response;

            response["success"] = false;
            response["message"] =
                "Order item not found or it does not belong to this seller.";

            auto result =
                HttpResponse::newHttpJsonResponse(response);

            result->setStatusCode(k404NotFound);

            callback(result);
            return;
        }

        std::string currentStatus =
            check[0]["status"].is_null()
                ? "Pending"
                : check[0]["status"].as<std::string>();

        // ----------------------------------------------------
        // Already delivered
        // ----------------------------------------------------

        if (currentStatus == "Delivered")
        {
            transaction.commit();

            Json::Value response;

            response["success"] = true;

            response["message"] =
                "This product is already marked as Delivered.";

            response["order_item_id"] =
                orderItemId;

            response["status"] =
                "Delivered";

            callback(
                HttpResponse::newHttpJsonResponse(response)
            );

            return;
        }

        // ----------------------------------------------------
        // Update ONLY this order item
        // ----------------------------------------------------

        transaction.exec_params(
            "UPDATE order_items "
            "SET status = 'Delivered' "
            "WHERE order_item_id = $1",
            orderItemId
        );

        transaction.commit();

        // ----------------------------------------------------
        // Success
        // ----------------------------------------------------

        Json::Value response;

        response["success"] = true;

        response["message"] =
            "Product marked as Delivered.";

        response["order_item_id"] =
            orderItemId;

        response["status"] =
            "Delivered";

        if (!check[0]["product_name"].is_null())
        {
            response["product_name"] =
                check[0]["product_name"].as<std::string>();
        }

        callback(
            HttpResponse::newHttpJsonResponse(response)
        );
    }
    catch (const std::exception& e)
    {
        LOG_ERROR
            << "Mark item delivered error: "
            << e.what();

        Json::Value response;

        response["success"] = false;

        response["message"] =
            "Failed to update product delivery status.";

        auto result =
            HttpResponse::newHttpJsonResponse(response);

        result->setStatusCode(
            k500InternalServerError
        );

        callback(result);
    }
}