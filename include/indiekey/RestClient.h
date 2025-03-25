//
// Created by Ruurd Adema on 28/07/2023.
// Copyright (c) 2025 IndieKey LTD. All rights reserved.
//

#pragma once

#include <juce_core/juce_core.h>
#include <nlohmann/json.hpp>

namespace indiekey
{

class RestClient
{
public:
    struct Response
    {
        int statusCode { 0 };
        juce::String body;

        [[nodiscard]] bool isInformational() const;
        [[nodiscard]] bool isSuccessful() const;
        [[nodiscard]] bool isRedirection() const;
        [[nodiscard]] bool isClientError() const;
        [[nodiscard]] bool isServerError() const;

        void throwIfNotSuccessful();

        [[nodiscard]] std::string toString() const;
    };

    class Exception : public std::exception
    {
    public:
        Exception (int statusCode, const char* message);
        explicit Exception (const Response& response);

        [[nodiscard]] const char* what() const noexcept override;

    private:
        std::string mMessage;
    };

    explicit RestClient (juce::URL address);

    Response get (juce::StringRef path);
    Response post (juce::StringRef path, const nlohmann::json& postData);

private:
    juce::URL mAddress;

    static const juce::URL::InputStreamOptions& getDefaultInputStreamOptions();
};

} // namespace indiekey
