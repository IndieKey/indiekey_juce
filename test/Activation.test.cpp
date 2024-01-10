//
// Created by Ruurd Adema on 13/10/2023.
// Copyright (c) 2023 Sound on Digital. All rights reserved.
//

#include <gtest/gtest.h>

#include "indiekey/Activation.h"

TEST (Activation, Activation_TestDefaultState)
{
    indiekey::Activation activation;
    ASSERT_FALSE (activation.verifySignature ({}));
    ASSERT_EQ (activation.getStatus(), indiekey::Activation::Status::Undefined);
    ASSERT_EQ (activation.validate ({}, {}, {}), indiekey::Activation::Status::InvalidSignature);
    ASSERT_EQ (activation.getStatus(), indiekey::Activation::Status::InvalidSignature);
    ASSERT_TRUE (activation.getHash().empty());
    ASSERT_TRUE (activation.getProductUid().empty());
    ASSERT_TRUE (activation.getMachineUid().empty());
    ASSERT_FALSE (activation.getExpiresAt().has_value());
    ASSERT_FALSE (activation.getLicenseExpiresAt().has_value());
    ASSERT_EQ (activation.getLicenseType(), indiekey::License::Type::Undefined);
    ASSERT_EQ (activation.getSignature(), std::vector<uint8_t> {});
    ASSERT_FALSE (activation.isExpired());
}
