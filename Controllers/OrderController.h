#pragma once

#include <drogon/HttpController.h>

class OrderController : public drogon::HttpController<OrderController>
{
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(OrderController::placeOrder, "/api/orders", drogon::Post);
    METHOD_LIST_END

    void placeOrder(
        const drogon::HttpRequestPtr& req,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );
};