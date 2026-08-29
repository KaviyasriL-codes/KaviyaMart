#include "AuthController.h"
#include "../Services/Database.h"

#include <drogon/drogon.h>
#include <openssl/sha.h>

#include <iomanip>
#include <sstream>
#include <string>

using namespace drogon;

namespace
{
    std::string hashPassword(const std::string& password)
    {
        unsigned char hash[SHA256_DIGEST_LENGTH];

        SHA256(
            reinterpret_cast<const unsigned char*>(password.c_str()),
            password.size(),
            hash
        );

        std::ostringstream result;

        for (unsigned char c : hash)
        {
            result << std::hex
                   << std::setw(2)
                   << std::setfill('0')
                   << static_cast<int>(c);
        }

        return result.str();
    }

    HttpResponsePtr jsonResponse(
        const Json::Value& data,
        int statusCode = 200)
    {
        auto response = HttpResponse::newHttpJsonResponse(data);
        response->setStatusCode(
            static_cast<HttpStatusCode>(statusCode)
        );
        return response;
    }
}

void AuthController::registerUser(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    try
    {
        auto json = req->getJsonObject();

        if (!json)
        {
            Json::Value response;
            response["success"] = false;
            response["message"] = "Invalid JSON request.";

            callback(jsonResponse(response, 400));
            return;
        }

        const std::string name =
            (*json)["name"].asString();

        const std::string email =
            (*json)["email"].asString();

        const std::string password =
            (*json)["password"].asString();

        const std::string role =
            (*json)["role"].asString();

        const std::string mobile =
            (*json)["mobile"].asString();

        if (name.empty() ||
            email.empty() ||
            password.empty() ||
            role.empty())
        {
            Json::Value response;
            response["success"] = false;
            response["message"] =
                "Name, email, password and role are required.";

            callback(jsonResponse(response, 400));
            return;
        }

        if (role != "buyer" && role != "seller")
        {
            Json::Value response;
            response["success"] = false;
            response["message"] =
                "Role must be buyer or seller.";

            callback(jsonResponse(response, 400));
            return;
        }

        auto db = Database::connect();

        pqxx::work transaction(*db);

        const std::string passwordHash =
            hashPassword(password);

        pqxx::result result = transaction.exec_params(
            "INSERT INTO users "
            "(name, email, password_hash, role, mobile) "
            "VALUES ($1, $2, $3, $4, $5) "
            "RETURNING user_id, name, email, role",
            name,
            email,
            passwordHash,
            role,
            mobile
        );

        transaction.commit();

        Json::Value response;
        response["success"] = true;
        response["message"] =
            "Registration successful.";

        response["user_id"] =
            result[0]["user_id"].as<int>();

        response["name"] =
            result[0]["name"].c_str();

        response["email"] =
            result[0]["email"].c_str();

        response["role"] =
            result[0]["role"].c_str();

        callback(jsonResponse(response, 201));
    }
    catch (const pqxx::unique_violation&)
    {
        Json::Value response;
        response["success"] = false;
        response["message"] =
            "Email already registered.";

        callback(jsonResponse(response, 409));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Registration error: "
                  << e.what();

        Json::Value response;
        response["success"] = false;
        response["message"] =
            "Registration failed.";

        callback(jsonResponse(response, 500));
    }
}

void AuthController::loginUser(
    const HttpRequestPtr& req,
    std::function<void(const HttpResponsePtr&)>&& callback)
{
    try
    {
        auto json = req->getJsonObject();

        if (!json)
        {
            Json::Value response;
            response["success"] = false;
            response["message"] = "Invalid JSON request.";

            callback(jsonResponse(response, 400));
            return;
        }

        const std::string email =
            (*json)["email"].asString();

        const std::string password =
            (*json)["password"].asString();

        if (email.empty() || password.empty())
        {
            Json::Value response;
            response["success"] = false;
            response["message"] =
                "Email and password are required.";

            callback(jsonResponse(response, 400));
            return;
        }

        auto db = Database::connect();

        pqxx::work transaction(*db);

        pqxx::result result = transaction.exec_params(
            "SELECT user_id, name, email, password_hash, role "
            "FROM users "
            "WHERE email = $1",
            email
        );

        transaction.commit();

        if (result.empty())
        {
            Json::Value response;
            response["success"] = false;
            response["message"] =
                "Invalid email or password.";

            callback(jsonResponse(response, 401));
            return;
        }

        const std::string storedHash =
            result[0]["password_hash"].c_str();

        const std::string enteredHash =
            hashPassword(password);

        if (storedHash != enteredHash)
        {
            Json::Value response;
            response["success"] = false;
            response["message"] =
                "Invalid email or password.";

            callback(jsonResponse(response, 401));
            return;
        }

        Json::Value response;

        response["success"] = true;
        response["message"] =
            "Login successful.";

        response["user_id"] =
            result[0]["user_id"].as<int>();

        response["name"] =
            result[0]["name"].c_str();

        response["email"] =
            result[0]["email"].c_str();

        response["role"] =
            result[0]["role"].c_str();

        callback(jsonResponse(response, 200));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << "Login error: "
                  << e.what();

        Json::Value response;
        response["success"] = false;
        response["message"] =
            "Login failed.";

        callback(jsonResponse(response, 500));
    }
}