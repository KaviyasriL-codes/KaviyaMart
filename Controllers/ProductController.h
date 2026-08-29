#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class ProductController
    : public drogon::HttpController<ProductController>
{
public:

    METHOD_LIST_BEGIN

    ADD_METHOD_TO(
        ProductController::addProduct,
        "/api/products",
        Post
    );

    ADD_METHOD_TO(
        ProductController::getProducts,
        "/api/products",
        Get
    );

    ADD_METHOD_TO(
        ProductController::getSellerProducts,
        "/api/products/seller/{1}",
        Get
    );

    METHOD_LIST_END


    void addProduct(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );

    void getProducts(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );

    void getSellerProducts(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback,
        int sellerId
    );
};