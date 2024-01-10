//
// Created by Ruurd Adema on 28/07/2023.
// Copyright (c) 2023 Sound on Digital. All rights reserved.
//

#include "indiekey/RestClient.h"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <utility>

indiekey::RestClient::Exception::Exception (int statusCode, const char* message) :
    mMessage (fmt::format ("{} {}", statusCode, message))
{
}

indiekey::RestClient::Exception::Exception (const indiekey::RestClient::Response& response) :
    mMessage (response.toString())
{
}

indiekey::RestClient::RestClient (juce::URL address) : mAddress (std::move (address)) {}

indiekey::RestClient::Response indiekey::RestClient::get (juce::StringRef path)
{
    auto requestUrl = mAddress.getChildURL (path);

    Response response;

    auto inputStream = requestUrl.createInputStream (
        getDefaultInputStreamOptions().withStatusCode (&response.statusCode));

    if (inputStream == nullptr)
        throw std::runtime_error ("Failed to reach activation server");

    response.body = inputStream->readEntireStreamAsString();

    return response;
}

indiekey::RestClient::Response indiekey::RestClient::post (juce::StringRef path, const nlohmann::json& postData)
{
    auto requestUrl = mAddress.getChildURL (path);
    requestUrl = requestUrl.withPOSTData (postData.dump());

    Response response;

    auto inputStream = requestUrl.createInputStream (getDefaultInputStreamOptions()
                                                         .withStatusCode (&response.statusCode)
                                                         .withExtraHeaders ("Content-Type: application/json")
                                                         .withConnectionTimeoutMs (3000));

    if (inputStream == nullptr)
        throw std::runtime_error ("Failed to reach activation server");

    response.body = inputStream->readEntireStreamAsString();

    return response;
}

const juce::URL::InputStreamOptions& indiekey::RestClient::getDefaultInputStreamOptions()
{
    static const auto inputStreamOptions = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                                               .withConnectionTimeoutMs (1000)
                                               .withNumRedirectsToFollow (0);
    return inputStreamOptions;
}

bool indiekey::RestClient::Response::isInformational() const
{
    return statusCode >= 100 && statusCode <= 199;
}

bool indiekey::RestClient::Response::isSuccessful() const
{
    return statusCode >= 200 && statusCode <= 299;
}

bool indiekey::RestClient::Response::isRedirection() const
{
    return statusCode >= 300 && statusCode <= 399;
}

bool indiekey::RestClient::Response::isClientError() const
{
    return statusCode >= 400 && statusCode <= 499;
}

bool indiekey::RestClient::Response::isServerError() const
{
    return statusCode >= 500 && statusCode <= 599;
}

void indiekey::RestClient::Response::throwIfNotSuccessful()
{
    if (!isSuccessful())
        throw Exception (*this);
}

std::string indiekey::RestClient::Response::toString() const
{
    return fmt::format ("{} ({})", body.toRawUTF8(), statusCode);
}

const char* indiekey::RestClient::Exception::what() const noexcept
{
    return mMessage.c_str();
}
