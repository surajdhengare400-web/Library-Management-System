// Security.h - SHA-256, HMAC-SHA256 and PBKDF2-HMAC-SHA256 implemented in standard C++.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace security {

using Bytes = std::vector<std::uint8_t>;

constexpr std::uint32_t kDefaultIterations = 200000;
constexpr std::uint32_t kMinIterations = 10000;
constexpr std::uint32_t kMaxIterations = 5000000;
constexpr std::size_t kSaltBytes = 16;
constexpr std::size_t kHashBytes = 32;

Bytes sha256(const Bytes& data);
Bytes pbkdf2HmacSha256(const std::string& password, const Bytes& salt, std::uint32_t iterations,
                       std::size_t derivedKeyLength = kHashBytes);

std::string toHex(const Bytes& bytes);
bool fromHex(const std::string& text, Bytes& out);
bool isHex(const std::string& text, std::size_t expectedLength);

std::string randomHex(std::size_t byteCount);                          // random token, hex encoded
std::string generateSaltHex();                                       // random salt, hex encoded
std::string derivePasswordHash(const std::string& password, const std::string& saltHex,
                               std::uint32_t iterations);            // hex, "" if salt is invalid
bool constantTimeEquals(const std::string& a, const std::string& b);

}  // namespace security
