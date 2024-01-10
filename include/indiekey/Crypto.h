//
// Created by Ruurd Adema on 04/10/2023.
// Copyright (c) 2023 Sound on Digital. All rights reserved.
//

#pragma once

#include <string>
#include <vector>

namespace indiekey::crypto
{

/**
 * Initialises sodium and throws an exception if the initialisation fails.
 */
void init();

/**
 * @param data The data to hash.
 * @returns A secure hash of given data.
 */
[[maybe_unused]] std::vector<uint8_t> genericHash (const std::vector<uint8_t>& data);

/**
 *
 * @param text The text to hash.
 * @returns A secure hash of given string.
 */
[[maybe_unused]] std::vector<uint8_t> genericHash (const std::string& text);

/**
 * Encrypts given data with given key.
 * @param data The data to encrypt.
 * @param key The key to use for encryption.
 * @return The encrypted data.
 */
[[maybe_unused]] std::vector<uint8_t> boxSeal (const std::vector<uint8_t>& data, const std::vector<uint8_t>& key);

/**
 * Encrypts given data with given key.
 * @param data The data to encrypt.
 * @param key The key to use for encryption.
 * @return The encrypted data.
 */
[[maybe_unused]] std::vector<uint8_t> boxSeal (const std::string& data, const std::vector<uint8_t>& key);

/**
 * Encrypts given data with given key.
 * @param data The data to encrypt.
 * @param dataLength The length of the data to encrypt.
 * @param key The key to use for encryption.
 * @return The encrypted data.
 */
[[maybe_unused]] std::vector<uint8_t> boxSeal (const unsigned char* data, size_t dataLength, const std::vector<uint8_t>& key);

} // namespace indiekey::crypto
