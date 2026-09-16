#pragma once

#include <drogon/HttpController.h>

class ProductController
    : public drogon::HttpController<ProductController>
{
public:

    METHOD_LIST_BEGIN

    // =====================================================
    // Add Product
    // POST /api/products
    // =====================================================
    ADD_METHOD_TO(
        ProductController::addProduct,
        "/api/products",
        drogon::Post
    );


    // =====================================================
    // Get All Products
    // GET /api/products
    // =====================================================
    ADD_METHOD_TO(
        ProductController::getProducts,
        "/api/products",
        drogon::Get
    );


    // =====================================================
    // Get Seller Products
    // GET /api/products/seller/{sellerId}
    // =====================================================
    ADD_METHOD_TO(
        ProductController::getSellerProducts,
        "/api/products/seller/{1}",
        drogon::Get
    );


    // =====================================================
    // Get Single Product
    // GET /api/products/{productId}
    // =====================================================
    ADD_METHOD_TO(
        ProductController::getProduct,
        "/api/products/{1}",
        drogon::Get
    );


    // =====================================================
    // Update Product
    // PUT /api/products/{productId}
    // =====================================================
    ADD_METHOD_TO(
        ProductController::updateProduct,
        "/api/products/{1}",
        drogon::Put
    );

    METHOD_LIST_END


    // =====================================================
    // Add Product
    // =====================================================
    void addProduct(
        const drogon::HttpRequestPtr& req,
        std::function<void(
            const drogon::HttpResponsePtr&
        )>&& callback
    );


    // =====================================================
    // Get All Products
    // =====================================================
    void getProducts(
        const drogon::HttpRequestPtr& req,
        std::function<void(
            const drogon::HttpResponsePtr&
        )>&& callback
    );


    // =====================================================
    // Get Seller Products
    // =====================================================
    void getSellerProducts(
        const drogon::HttpRequestPtr& req,
        std::function<void(
            const drogon::HttpResponsePtr&
        )>&& callback,
        int sellerId
    );


    // =====================================================
    // Get Single Product
    // =====================================================
    void getProduct(
        const drogon::HttpRequestPtr& req,
        std::function<void(
            const drogon::HttpResponsePtr&
        )>&& callback,
        int productId
    );


    // =====================================================
    // Update Product
    // =====================================================
    void updateProduct(
        const drogon::HttpRequestPtr& req,
        std::function<void(
            const drogon::HttpResponsePtr&
        )>&& callback,
        int productId
    );
};