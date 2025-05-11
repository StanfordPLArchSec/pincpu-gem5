#pragma once

#include <openssl/sha.h>
#include <cstdint>
#include <array>

using Sha256Hash = std::array<uint8_t, SHA256_DIGEST_LENGTH>;

template <typename T>
Sha256Hash
sha256(const T *data, std::size_t len)
{
    Sha256Hash hash;
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data, len);
    SHA256_Final(hash.data(), &sha256);
    return hash;
}

template <typename T>
Sha256Hash
sha256(const std::vector<T> &data)
{
    return sha256(data.data(), data.size());
}
                 
