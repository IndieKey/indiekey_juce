//
// Created by Ruurd Adema on 04/10/2023.
// Copyright (c) 2023 Sound on Digital. All rights reserved.
//

#include "indiekey/Activation.h"
#include "indiekey/Encoding.h"

#include <sodium/core.h>
#include <sodium/crypto_sign.h>

#include <utility>

void indiekey::Activation::fromJson (const nlohmann::json& json)
{
    hash_ = decodeFromBase64 (json.at ("activation_hash").get<std::string>());
    productUid_ = json.at ("product_uid").get<std::string>();
    machineUid_ = decodeFromBase64 (json.at ("machine_uid").get<std::string>());

    if (const auto& expiresAt = json.at ("expires_at"); !expiresAt.is_null())
        expiresAt_ = juce::Time (expiresAt.get<int64_t>());

    if (const auto& licenseExpiresAt = json.at ("license_expires_at"); !licenseExpiresAt.is_null())
        licenseExpiresAt_ = juce::Time (licenseExpiresAt.get<int64_t>());

    licenseType_ = License::typeFromString (json.at ("license_type").get<std::string>());
    signature_ = decodeFromBase64 (json.at ("signature").get<std::string>());
}

nlohmann::json indiekey::Activation::toJson() const
{
    nlohmann::json json;

    json["activation_hash"] = encodeToBase64 (hash_);
    json["product_uid"] = productUid_;
    json["machine_uid"] = encodeToBase64 (machineUid_);
    expiresAt_.has_value() ? json["expires_at"] = expiresAt_->toMilliseconds() : json["expires_at"] = nullptr;
    licenseExpiresAt_.has_value() ? json["license_expires_at"] = licenseExpiresAt_->toMilliseconds()
                                  : json["license_expires_at"] = nullptr;
    json["license_type"] = License::typeToString (licenseType_);
    json["signature"] = encodeToBase64 (signature_);

    return json;
}

indiekey::Activation::Status indiekey::Activation::validate (
    const std::string& productUid,
    const std::vector<uint8_t>& machineUid,
    const std::vector<uint8_t>& verifyingKey)
{
    auto now = juce::Time::getCurrentTime();

    if (productUid != productUid_)
        status_ = indiekey::Activation::Status::InvalidProductUid;
    else if (machineUid != machineUid_)
        status_ = indiekey::Activation::Status::InvalidMachineUid;
    else if (licenseExpiresAt_.has_value() && now > licenseExpiresAt_.value())
        status_ = indiekey::Activation::Status::LicenseExpired;
    else if (expiresAt_.has_value() && now > expiresAt_)
        status_ = indiekey::Activation::Status::ActivationExpired;
    else if (!verifySignature (verifyingKey))
        status_ = indiekey::Activation::Status::InvalidSignature;
    else
        status_ = indiekey::Activation::Status::Valid;

    return status_;
}

bool indiekey::Activation::verifySignature (const std::vector<uint8_t>& verifyingKey) const
{
    if (sodium_init() == -1)
        throw std::runtime_error ("Initialisation failure");

    if (verifyingKey.size() != crypto_sign_PUBLICKEYBYTES)
        return false;

    crypto_sign_state state {};

    if (crypto_sign_init (&state) != 0)
        throw std::runtime_error ("Failed to initialize crypto_sign_state");

    if (crypto_sign_update (&state, hash_.data(), hash_.size()) != 0)
        throw std::runtime_error ("Failed to update crypto_sign_state");

    auto productUidData = reinterpret_cast<const unsigned char*> (productUid_.data());
    if (crypto_sign_update (&state, productUidData, productUid_.size()) != 0)
        throw std::runtime_error ("Failed to update crypto_sign_state");

    if (crypto_sign_update (&state, machineUid_.data(), machineUid_.size()) != 0)
        throw std::runtime_error ("Failed to update crypto_sign_state");

    if (expiresAt_.has_value())
    {
        int64_t bigEndianBytes = juce::ByteOrder::swapIfLittleEndian (expiresAt_->toMilliseconds());

        if (crypto_sign_update (
                &state,
                reinterpret_cast<const unsigned char*> (&bigEndianBytes),
                sizeof (bigEndianBytes)) != 0)
            throw std::runtime_error ("Failed to update crypto_sign_state");
    }

    if (licenseExpiresAt_.has_value())
    {
        int64_t bigEndianBytes = juce::ByteOrder::swapIfLittleEndian (licenseExpiresAt_->toMilliseconds());

        if (crypto_sign_update (
                &state,
                reinterpret_cast<const unsigned char*> (&bigEndianBytes),
                sizeof (bigEndianBytes)) != 0)
            throw std::runtime_error ("Failed to update crypto_sign_state");
    }

    std::string typeString = License::typeToString (licenseType_);
    if (crypto_sign_update (&state, reinterpret_cast<const unsigned char*> (typeString.data()), typeString.size()) != 0)
        throw std::runtime_error ("Failed to update crypto_sign_state");

    return crypto_sign_final_verify (&state, signature_.data(), verifyingKey.data()) == 0;
}

const char* indiekey::Activation::statusToString (Status status)
{
    switch (status)
    {
    case Status::Undefined:
        return "Undefined";
    case Status::InvalidSignature:
        return "InvalidSignature";
    case Status::InvalidProductUid:
        return "InvalidProductUid";
    case Status::InvalidMachineUid:
        return "InvalidMachineUid";
    case Status::LicenseExpired:
        return "LicenseExpired";
    case Status::ActivationExpired:
        return "ActivationExpired";
    case Status::Valid:
        return "Valid";
    case Status::NoActivationLoaded:
        return "NoActivationLoaded";
    }
    return "";
}

const indiekey::Activation::Hash& indiekey::Activation::getHash() const
{
    return hash_;
}

const std::string& indiekey::Activation::getProductUid() const
{
    return productUid_;
}

const std::vector<uint8_t>& indiekey::Activation::getMachineUid() const
{
    return machineUid_;
}

const std::optional<juce::Time>& indiekey::Activation::getExpiresAt() const
{
    return expiresAt_;
}

const std::optional<juce::Time>& indiekey::Activation::getLicenseExpiresAt() const
{
    return licenseExpiresAt_;
}

indiekey::License::Type indiekey::Activation::getLicenseType() const
{
    return licenseType_;
}

const std::vector<uint8_t>& indiekey::Activation::getSignature() const
{
    return signature_;
}

indiekey::Activation::Activation (
    Hash hash,
    std::string productUid,
    std::vector<uint8_t> machineUid,
    std::optional<juce::Time> expiresAt,
    std::optional<juce::Time> licenseExpiresAt,
    License::Type licenseType,
    std::vector<uint8_t> signature) :
    hash_ (std::move (hash)),
    productUid_ (std::move (productUid)),
    machineUid_ (std::move (machineUid)),
    expiresAt_ (expiresAt),
    licenseExpiresAt_ (licenseExpiresAt),
    licenseType_ (licenseType),
    signature_ (std::move (signature))
{
}

bool indiekey::Activation::isMoreValuableThan (const indiekey::Activation& other) const
{
    if (isExpired() && !other.isExpired())
        return false;

    if (other.isExpired())
        return true;

    if (licenseExpiresAt_ != other.licenseExpiresAt_)
    {
        if (licenseExpiresAt_.has_value())
        {
            if (other.licenseExpiresAt_.has_value())
            {
                if (licenseExpiresAt_.value() > other.licenseExpiresAt_.value())
                    return true;
            }
            else
            {
                return false; // Other is more valuable because it doesn't expire whereas this does.
            }
        }
        else
        {
            return true; // Only this or both don't expire in which case we consider this to be more valuable
        }
    }

    if (expiresAt_ > other.expiresAt_)
        return true;

    if (expiresAt_ < other.expiresAt_)
        return false;

    return License::compareTypeValue (licenseType_, other.licenseType_) > 0;
}

bool indiekey::Activation::isExpired() const
{
    auto now = juce::Time::getCurrentTime();
    if (expiresAt_.has_value() && now > expiresAt_)
        return true;
    if (licenseExpiresAt_.has_value() && now > licenseExpiresAt_.value())
        return true;
    return false;
}

indiekey::Activation::Status indiekey::Activation::getStatus() const
{
    return status_;
}

const char* indiekey::Activation::getStatusAsString() const
{
    return statusToString (status_);
}

const char* indiekey::Activation::getStatusUserMessage (bool hideDetails) const
{
    switch (status_)
    {
    case Status::Undefined:
        return hideDetails ? "Invalid activation (1)" : "Undefined";
    case Status::NoActivationLoaded:
        return hideDetails ? "Invalid activation (2)" : "No activation loaded";
    case Status::InvalidSignature:
        return hideDetails ? "Invalid activation (3)" : "Invalid signature";
    case Status::InvalidProductUid:
        return hideDetails ? "Invalid activation (4)" : "Invalid product uid";
    case Status::InvalidMachineUid:
        return hideDetails ? "Invalid activation (5)" : "Invalid machine uid";
    case Status::LicenseExpired:
        return licenseType_ == License::Type::Trial ? "Your trial license expired" : "Your license expired";
    case Status::ActivationExpired:
        return licenseType_ == License::Type::Trial ? "Your trial activation expired" : "Your activation expired";
    case Status::Valid:
        return "License valid";
    default:
        return "";
    }
}

std::string indiekey::Activation::getSummary() const
{
    auto text = std::string (License::typeToString (licenseType_)) + " license is " + getStatusAsString();
    text += std::string (", activation expires on ") + expiryDateAsString (expiresAt_);
    text += std::string (" and the license itself expires on ") + expiryDateAsString (licenseExpiresAt_);
    return text;
}

std::string indiekey::Activation::expiryDateAsString (std::optional<juce::Time> expiryTime)
{
    auto now = juce::Time::getCurrentTime();
    if (expiryTime.has_value())
    {
        return expiryTime->toString (true, true, false).toStdString() + " (which is " +
               (*expiryTime - now).getDescription().toStdString() + " from now)";
    }
    return "never";
}

void indiekey::from_json (const nlohmann::json& json, indiekey::Activation& activation)
{
    activation.fromJson (json);
}

void indiekey::to_json (nlohmann::json& json, const indiekey::Activation& activation)
{
    json = activation.toJson();
}
