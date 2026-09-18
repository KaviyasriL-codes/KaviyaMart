#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class AdminController
    : public drogon::HttpController<AdminController>
{
public:

    METHOD_LIST_BEGIN

        // ==========================================
        // ADMIN DASHBOARD
        // ==========================================

        ADD_METHOD_TO(
            AdminController::getStats,
            "/api/admin/stats",
            Get
        );

        // ==========================================
        // SELLERS
        // ==========================================

        ADD_METHOD_TO(
            AdminController::getSellers,
            "/api/admin/sellers",
            Get
        );

        // ==========================================
        // BUYERS
        // ==========================================

        ADD_METHOD_TO(
            AdminController::getBuyers,
            "/api/admin/buyers",
            Get
        );

        // ==========================================
        // PRODUCTS
        // ==========================================

        ADD_METHOD_TO(
            AdminController::getProducts,
            "/api/admin/products",
            Get
        );

        // ==========================================
        // ORDERS
        // ==========================================

        ADD_METHOD_TO(
            AdminController::getOrders,
            "/api/admin/orders",
            Get
        );

    METHOD_LIST_END


    // ==========================================
    // FUNCTION DECLARATIONS
    // ==========================================

    void getStats(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );

    void getSellers(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );

    void getBuyers(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );

    void getProducts(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );

    void getOrders(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );
};