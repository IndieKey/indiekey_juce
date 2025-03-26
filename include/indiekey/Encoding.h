//
// Created by Ruurd Adema on 04/10/2023.
// Copyright (c) 2025 IndieKey LTD. All rights reserved.
//

#pragma once

#include <string>
#include <vector>

#include <sodium/utils.h>

namespace indiekey
{

static std::string encodeToBase64 (const uint8_t* data, size_t dataLength)
{
    auto maxLength = sodium_base64_encoded_len (dataLength, sodium_base64_VARIANT_ORIGINAL);
    std::string output (maxLength, 0);
    if (sodium_bin2base64(output.data(), output.size(), data, dataLength, sodium_base64_VARIANT_ORIGINAL) == nullptr)
        throw std::runtime_error ("Base64 encoding failed");

    // Remove trailing null character
    jassert(output.back() == 0);
    output.pop_back();
    jassert(output.back() != 0);

    return output;
}

[[maybe_unused]] static std::string encodeToBase64 (const std::vector<uint8_t>& data)
{
    return encodeToBase64 (data.data(), data.size());
}

static std::vector<uint8_t> decodeFromBase64 (const char* base64EncodedString)
{
    jassert (base64EncodedString);

    if (base64EncodedString == nullptr)
        return {};

    auto length = std::strlen (base64EncodedString);

    if (length == 0)
        return {};

    std::vector<uint8_t> binaryData (length / 4 * 3);

    size_t outputLength = 0;
    if (sodium_base642bin (
            binaryData.data(),
            binaryData.size(),
            base64EncodedString,
            length,
            nullptr,
            &outputLength,
            nullptr,
            sodium_base64_VARIANT_ORIGINAL) != 0)
    {
        throw std::runtime_error ("Base64 decoding failed");
    }

    binaryData.resize (outputLength);

    return binaryData;
}

static std::vector<uint8_t> decodeFromBase64 (const std::string& base64EncodedString)
{
    return decodeFromBase64 (base64EncodedString.c_str());
}

} // namespace indiekey
