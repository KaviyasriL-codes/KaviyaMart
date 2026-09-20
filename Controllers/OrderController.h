#pragma once

#include <drogon/HttpController.h>

class OrderController : public drogon::HttpController<OrderController>
{
public:

    METHOD_LIST_BEGIN

    // ============================================================
    // PLACE ORDER
    // ============================================================

    ADD_METHOD_TO(
        OrderController::placeOrder,
        "/api/orders",
        drogon::Post
    );


    // ============================================================
    // BUYER - MY ORDERS
    // ============================================================

    ADD_METHOD_TO(
        OrderController::getBuyerOrders,
        "/api/orders/buyer/{1}",
        drogon::Get
    );


    // ============================================================
    // SELLER - VIEW ORDERS
    // ============================================================

    ADD_METHOD_TO(
        OrderController::getSellerOrders,
        "/api/orders/seller/{1}",
        drogon::Get
    );


    // ============================================================
    // SELLER - MARK INDIVIDUAL PRODUCT AS DELIVERED
    // ============================================================

    ADD_METHOD_TO(
        OrderController::markItemDelivered,
        "/api/order-items/{1}/delivered",
        drogon::Put
    );


    METHOD_LIST_END


    // ============================================================
    // PLACE ORDER
    // ============================================================

    void placeOrder(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );


    // ============================================================
    // BUYER - MY ORDERS
    // ============================================================

    void getBuyerOrders(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int userId
    );


    // ============================================================
    // SELLER - VIEW ORDERS
    // ============================================================

    void getSellerOrders(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int sellerId
    );


    // ============================================================
    // SELLER - MARK ONE PRODUCT AS DELIVERED
    // ============================================================

    void markItemDelivered(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        int orderItemId
    );

};