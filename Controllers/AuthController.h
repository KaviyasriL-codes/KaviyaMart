#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

class AuthController : public drogon::HttpController<AuthController>
{
public:

    METHOD_LIST_BEGIN

    ADD_METHOD_TO(
        AuthController::registerUser,
        "/api/register",
        Post
    );

    ADD_METHOD_TO(
        AuthController::loginUser,
        "/api/login",
        Post
    );

    METHOD_LIST_END


    void registerUser(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );


    void loginUser(
        const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback
    );
};