//
// Created by Ruurd Adema on 28/07/2023.
// Copyright (c) 2023 Sound on Digital. All rights reserved.
//

#pragma once

#include <vector>

#include "License.h"
#include <juce_core/juce_core.h>
#include <nlohmann/json.hpp>

namespace indiekey
{

class Activation
{
public:
    using Hash = std::vector<uint8_t>;

    enum class Status
    {
        Undefined,
        NoActivationLoaded,
        InvalidSignature,
        InvalidProductUid,
        InvalidMachineUid,
        LicenseExpired,
        ActivationExpired,
        Valid,
    };

    Activation() = default;
    Activation (
        Hash hash,
        std::string productUid,
        std::vector<uint8_t> machineUid,
        std::optional<juce::Time> expiresAt,
        std::optional<juce::Time> licenseExpiresAt,
        License::Type licenseType,
        std::vector<uint8_t> signature);

    /**
     * Restores this object from given json object.
     * @param json The json object to restore from.
     */
    void fromJson (const nlohmann::json& json);

    /**
     * Saves activation to json.
     * @return A json object representing this activation.
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /**
     * Verifies signature of this activation.
     * @param verifyingKey The verifying key.
     * @return True if signature is valid, or false if not.
     */
    [[nodiscard]] bool verifySignature (const std::vector<uint8_t>& verifyingKey) const;

    /**
     * Validates this activation. Also updates the internal status for later retrieval using getStatus().
     * @param productUid The product uid to verify.
     * @param machineUid The machine uid to verify.
     * @param verifyingKey The verifying key.
     * @return The activation status.
     */
    [[nodiscard]] Status validate (
        const std::string& productUid,
        const std::vector<uint8_t>& machineUid,
        const std::vector<uint8_t>& verifyingKey);

    /**
     * @param status The status to get the string for.
     * @returns A string for given status.
     */
    static const char* statusToString (Status status);

    /**
     * @param True to hide the details of an invalid license.
     * @returns A message which can be shown to the user.
     */
    [[nodiscard]] const char* getStatusUserMessage (bool hideDetails = true) const;

    /**
     * @returns The hash of this activation. The has is a secure hash of a combination of email address and license key.
     */
    [[nodiscard]] const Hash& getHash() const;

    /**
     * @returns The product uid of this activation.
     */
    [[nodiscard]] const std::string& getProductUid() const;

    /**
     * @return The machine uid;
     */
    [[nodiscard]] const std::vector<uint8_t>& getMachineUid() const;

    /**
     * @returns The time at which this activation expires.
     */
    [[nodiscard]] const std::optional<juce::Time>& getExpiresAt() const;

    /**
     * @returns The time at which the license for this activation expires, or nullopt_t if the license doesn't expire.
     */
    [[nodiscard]] const std::optional<juce::Time>& getLicenseExpiresAt() const;

    /**
     * @returns The license type.
     */
    [[nodiscard]] License::Type getLicenseType() const;

    /**
     * @returns The signature of this license.
     */
    [[nodiscard]] const std::vector<uint8_t>& getSignature() const;

    /**
     * @param other The other activation to compare against.
     * @return True if this activation is more valuable than other.
     */
    [[nodiscard]] bool isMoreValuableThan (const Activation& other) const;

    /**
     * @returns True if either the activation or the license expired, or false if both are not expired.
     */
    [[nodiscard]] bool isExpired() const;

    /**
     * @returns The most recent status after validation. If not validated the status will be Status::Undefined.
     */
    [[nodiscard]] Status getStatus() const;

    /**
     * @returns The most recent status as string.
     */
    [[nodiscard]] const char* getStatusAsString() const;

    /**
     * @returns A string with a summary of this activation.
     */
    [[nodiscard]] std::string getSummary() const;

private:
    Hash hash_;
    std::string productUid_;
    std::vector<uint8_t> machineUid_;
    std::optional<juce::Time> expiresAt_;
    std::optional<juce::Time> licenseExpiresAt_;
    License::Type licenseType_ = License::Type::Undefined;
    std::vector<uint8_t> signature_;
    Status status_ = Status::Undefined;

    static std::string expiryDateAsString (std::optional<juce::Time> expiryTime);
};

/**
 * nlohmann::json compatible function to restore an activation from json.
 * @param json The json object to restore from.
 * @param activation The activation object to restore.
 */
void from_json (const nlohmann::json& json, Activation& activation);

/**
 * nlohmann::json compatible function to save an activation to json.
 * @param json The json object to fill in.
 * @param activation The activation to convert to json.
 */
void to_json (nlohmann::json& json, const Activation& activation);

} // namespace indiekey
