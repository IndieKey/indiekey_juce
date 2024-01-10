//
// Created by Ruurd Adema on 04/10/2023.
// Copyright (c) 2023 Sound on Digital. All rights reserved.
//

#pragma once

#include "Encoding.h"
#include <nlohmann/json.hpp>

namespace indiekey
{

struct ProductData
{
    std::string organisationName;
    std::string productName;
    std::string productUid;
    std::vector<uint8_t> verifyingKey;
    std::vector<uint8_t> cryptoPublicKey;
    std::string primaryPublicServerAddress;
    std::string secondaryPublicServerAddress;

    [[nodiscard]] std::string toString() const
    {
        return std::string ("ProductData { ") + "organisationName='" + organisationName + "'" + ", productName='" +
               productName + "'" + ", productUid='" + productUid + "'" + ", primaryPublicServerAddress='" +
               primaryPublicServerAddress + "'" + ", secondaryPublicServerAddress='" + secondaryPublicServerAddress +
               "'" + " }";
    }
};

[[maybe_unused]] static void from_json (const nlohmann::json& json, ProductData& productData)
{
    productData.organisationName = json["organisation_name"].get<std::string>();
    productData.productName = json["product_name"].get<std::string>();
    productData.productUid = json["product_uid"].get<std::string>();
    productData.verifyingKey = decodeFromBase64 (json["verifying_key"].get<std::string>());
    productData.cryptoPublicKey = decodeFromBase64 (json["crypto_public_key"].get<std::string>());
    productData.primaryPublicServerAddress = json["primary_public_server_address"].get<std::string>();
    productData.secondaryPublicServerAddress = json["secondary_public_server_address"].get<std::string>();
}

} // namespace indiekey
