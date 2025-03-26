//
// Created by Ruurd Adema on 04/10/2023.
// Copyright (c) 2025 IndieKey LTD. All rights reserved.
//
#include "indiekey/Crypto.h"
#include <sodium/core.h>
#include <sodium/crypto_box.h>
#include <sodium/crypto_generichash.h>
#include <stdexcept>

std::vector<uint8_t> indiekey::crypto::genericHash (const std::vector<uint8_t>& data)
{
    unsigned char hash[crypto_generichash_BYTES];

    if (crypto_generichash (hash, sizeof hash, data.data(), data.size(), nullptr, 0) != 0)
        throw std::runtime_error ("Failed to generate hash");

    return { hash, hash + sizeof hash };
}

std::vector<uint8_t> indiekey::crypto::genericHash (const std::string& text)
{
    return genericHash (std::vector<uint8_t> (text.begin(), text.end()));
}

void indiekey::crypto::init()
{
    if (sodium_init() == -1)
        throw std::runtime_error ("Failed to initialise libsodium");
}

std::vector<uint8_t> indiekey::crypto::boxSeal (const std::vector<uint8_t>& data, const std::vector<uint8_t>& key)
{
    return boxSeal (data.data(), data.size(), key);
}

std::vector<uint8_t> indiekey::crypto::boxSeal (const std::string& data, const std::vector<uint8_t>& key)
{
    return boxSeal (reinterpret_cast<const unsigned char*> (data.data()), data.size(), key);
}

std::vector<uint8_t> indiekey::crypto::boxSeal (
    const unsigned char* data,
    size_t dataLength,
    const std::vector<uint8_t>& key)
{
    if (key.size() != crypto_box_PUBLICKEYBYTES)
        throw std::runtime_error ("Invalid key length");

    auto cipherTextLength = dataLength + crypto_box_SEALBYTES;
    std::vector<uint8_t> cipherText (cipherTextLength);

    if (crypto_box_seal (cipherText.data(), data, dataLength, key.data()) != 0)
        throw std::runtime_error ("Failed to encrypt data");

    return cipherText;
}
