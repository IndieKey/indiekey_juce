//
// Created by Ruurd Adema on 06/10/2023.
// Copyright (c) 2023 Sound on Digital. All rights reserved.
//

#pragma once

namespace indiekey
{

struct TrialRequest
{
public:
    TrialRequest (
        std::string productUid,
        std::string machineUid,
        std::string emailAddress,
        std::optional<std::string> deviceInfo = std::nullopt) :
        productUid_ (std::move (productUid)),
        machineUid_ (std::move (machineUid)),
        emailAddress_ (std::move (emailAddress)),
        deviceInfo_ (std::move (deviceInfo))
    {
    }

    [[nodiscard]] nlohmann::json toJson() const
    {
        nlohmann::json json;
        json["product_uid"] = productUid_;
        json["machine_uid"] = machineUid_;
        json["email_address"] = emailAddress_;
        if (deviceInfo_)
            json["device_info"] = *deviceInfo_;
        return json;
    }

private:
    std::string productUid_;
    std::string machineUid_;
    std::string emailAddress_;
    std::optional<std::string> deviceInfo_;
};

static void to_json (nlohmann::json& j, const TrialRequest& request)
{
    j = request.toJson();
}

} // namespace indiekey
