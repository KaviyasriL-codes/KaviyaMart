#pragma once

#include <drogon/HttpController.h>

class OrderController : public drogon::HttpController<OrderController>
{
public:

    METHOD_LIST_BEGIN

    // Place a new order
    ADD_METHOD_TO(
        OrderController::placeOrder,
        "/api/orders",
        drogon::Post
    );

    // Get orders for a buyer
    ADD_METHOD_TO(
        OrderController::getBuyerOrders,
        "/api/orders/buyer/{1}",
        drogon::Get
    );

    // Confirm that buyer received the order
    ADD_METHOD_TO(
        OrderController::confirmDelivery,
        "/api/orders/{1}/delivered",
        drogon::Put
    );

    // Get orders containing products belonging to a seller
    ADD_METHOD_TO(
        OrderController::getSellerOrders,
        "/api/orders/seller/{1}",
        drogon::Get
    );

    METHOD_LIST_END


    void placeOrder(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );


    void getBuyerOrders(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int userId
    );


    void confirmDelivery(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int orderId
    );


    void getSellerOrders(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int sellerId
    );
};