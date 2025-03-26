//
// Created by Ruurd Adema on 06/10/2023.
// Copyright (c) 2025 IndieKey LTD. All rights reserved.
//

#pragma once

namespace indiekey
{

struct OfflineRequest
{
public:
    struct ActivationRequest
    {
        explicit ActivationRequest (
            std::string productUid,
            std::string machineUid,
            std::string emailAddress,
            std::string licenseKey,
            std::optional<std::string> deviceInfo = std::nullopt) :
            productUid_ (std::move (productUid)),
            machineUid_ (std::move (machineUid)),
            emailAddress_ (std::move (emailAddress)),
            licenseKey_ (std::move (licenseKey)),
            deviceInfo_ (std::move (deviceInfo))
        {
        }

        explicit ActivationRequest (const nlohmann::json& json)
        {
            productUid_ = json.at ("product_uid").get<std::string>();
            machineUid_ = json.at ("machine_uid").get<std::string>();
            emailAddress_ = json.at ("email_address").get<std::string>();
            licenseKey_ = json.at ("license_key").get<std::string>();
        }

        [[nodiscard]] nlohmann::json toJson() const
        {
            nlohmann::json json;
            json["product_uid"] = productUid_;
            json["machine_uid"] = machineUid_;
            json["email_address"] = emailAddress_;
            json["license_key"] = licenseKey_;
            if (deviceInfo_)
                json["device_info"] = *deviceInfo_;
            return { { "ActivationRequest", json } };
        }

    private:
        std::string productUid_;
        std::string machineUid_;
        std::string emailAddress_;              // Must be stored encrypted
        std::string licenseKey_;                // Must be stored encrypted
        std::optional<std::string> deviceInfo_; // Must be stored encrypted
    };

    struct TrialRequest
    {
        explicit TrialRequest (std::string productUid, std::string machineUid, std::string emailAddress, std::optional<std::string> deviceInfo = std::nullopt) :
            productUid_ (std::move (productUid)),
            machineUid_ (std::move (machineUid)),
            emailAddress_ (std::move (emailAddress)),
            deviceInfo_ (std::move (deviceInfo))
        {
        }

        explicit TrialRequest (const nlohmann::json& json)
        {
            productUid_ = json.at ("product_uid").get<std::string>();
            machineUid_ = json.at ("machine_uid").get<std::string>();
            emailAddress_ = json.at ("email_address").get<std::string>();
        }

        [[nodiscard]] nlohmann::json toJson() const
        {
            nlohmann::json json;
            json["product_uid"] = productUid_;
            json["machine_uid"] = machineUid_;
            json["email_address"] = emailAddress_;
            if (deviceInfo_)
                json["device_info"] = *deviceInfo_;
            return { { "TrialRequest", json } };
        }

    private:
        std::string productUid_;
        std::string machineUid_;
        std::string emailAddress_; // Must be stored encrypted
        std::optional<std::string> deviceInfo_;
    };

    std::optional<ActivationRequest> activationRequest;
    std::optional<TrialRequest> trialRequest;
};

static void from_json (const nlohmann::json& json, OfflineRequest& request)
{
    if (json.contains ("ActivationRequest"))
    {
        request.activationRequest = OfflineRequest::ActivationRequest (json.at ("ActivationRequest"));
    }
    else if (json.contains ("TrialRequest"))
    {
        request.trialRequest = OfflineRequest::TrialRequest (json.at ("TrialRequest"));
    }
}

static void to_json (nlohmann::json& json, const OfflineRequest& request)
{
    if (request.activationRequest)
        json = request.activationRequest->toJson();
    else if (request.trialRequest)
        json = request.trialRequest->toJson();
}

} // namespace indiekey
