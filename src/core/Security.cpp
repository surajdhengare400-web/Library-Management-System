#include "Security.h"

#include <algorithm>
#include <chrono>
#include <random>

namespace security {

namespace {

const std::uint32_t kRoundConstants[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

inline std::uint32_t rotr(std::uint32_t x, unsigned n) { return (x >> n) | (x << (32U - n)); }

// Streaming SHA-256. Copyable, so HMAC can reuse the state after the key block.
class Sha256 {
public:
    Sha256() : bufferLength_(0), totalLength_(0) {
        state_[0] = 0x6a09e667; state_[1] = 0xbb67ae85; state_[2] = 0x3c6ef372; state_[3] = 0xa54ff53a;
        state_[4] = 0x510e527f; state_[5] = 0x9b05688c; state_[6] = 0x1f83d9ab; state_[7] = 0x5be0cd19;
        std::fill(buffer_, buffer_ + 64, static_cast<std::uint8_t>(0));
    }

    void update(const std::uint8_t* data, std::size_t length) {
        totalLength_ += length;
        for (std::size_t i = 0; i < length; ++i) {
            buffer_[bufferLength_++] = data[i];
            if (bufferLength_ == 64) {
                compress(buffer_);
                bufferLength_ = 0;
            }
        }
    }

    void finish(std::uint8_t* out) {
        const std::uint64_t bits = totalLength_ * 8ULL;
        std::uint8_t pad[72] = {0};
        pad[0] = 0x80;
        const std::size_t padLength = (bufferLength_ < 56) ? (56 - bufferLength_) : (120 - bufferLength_);
        update(pad, padLength);
        std::uint8_t lengthBytes[8];
        for (int i = 0; i < 8; ++i) lengthBytes[i] = static_cast<std::uint8_t>((bits >> (56 - 8 * i)) & 0xFFU);
        update(lengthBytes, 8);
        for (int i = 0; i < 8; ++i) {
            out[i * 4] = static_cast<std::uint8_t>(state_[i] >> 24);
            out[i * 4 + 1] = static_cast<std::uint8_t>(state_[i] >> 16);
            out[i * 4 + 2] = static_cast<std::uint8_t>(state_[i] >> 8);
            out[i * 4 + 3] = static_cast<std::uint8_t>(state_[i]);
        }
    }

private:
    void compress(const std::uint8_t* block) {
        std::uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<std::uint32_t>(block[i * 4]) << 24) | (static_cast<std::uint32_t>(block[i * 4 + 1]) << 16) |
                   (static_cast<std::uint32_t>(block[i * 4 + 2]) << 8) | static_cast<std::uint32_t>(block[i * 4 + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            const std::uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        std::uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
        std::uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];
        for (int i = 0; i < 64; ++i) {
            const std::uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            const std::uint32_t ch = (e & f) ^ (~e & g);
            const std::uint32_t t1 = h + s1 + ch + kRoundConstants[i] + w[i];
            const std::uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t t2 = s0 + maj;
            h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
        }
        state_[0] += a; state_[1] += b; state_[2] += c; state_[3] += d;
        state_[4] += e; state_[5] += f; state_[6] += g; state_[7] += h;
    }

    std::uint32_t state_[8];
    std::uint8_t buffer_[64];
    std::size_t bufferLength_;
    std::uint64_t totalLength_;
};

// HMAC-SHA256 with the padded key blocks pre-absorbed (fast for PBKDF2's many iterations).
class HmacSha256 {
public:
    explicit HmacSha256(const Bytes& key) {
        std::uint8_t block[64] = {0};
        if (key.size() > 64) {
            Sha256 h;
            h.update(key.data(), key.size());
            h.finish(block);                // 32-byte digest, rest stays zero
        } else {
            std::copy(key.begin(), key.end(), block);
        }
        std::uint8_t ipad[64];
        std::uint8_t opad[64];
        for (int i = 0; i < 64; ++i) {
            ipad[i] = static_cast<std::uint8_t>(block[i] ^ 0x36);
            opad[i] = static_cast<std::uint8_t>(block[i] ^ 0x5c);
        }
        inner_.update(ipad, 64);
        outer_.update(opad, 64);
    }

    void compute(const std::uint8_t* message, std::size_t length, std::uint8_t* out) const {
        Sha256 inner = inner_;
        inner.update(message, length);
        std::uint8_t innerDigest[32];
        inner.finish(innerDigest);
        Sha256 outer = outer_;
        outer.update(innerDigest, 32);
        outer.finish(out);
    }

private:
    Sha256 inner_;
    Sha256 outer_;
};

}  // namespace

Bytes sha256(const Bytes& data) {
    Sha256 h;
    h.update(data.data(), data.size());
    Bytes out(32);
    h.finish(out.data());
    return out;
}

Bytes pbkdf2HmacSha256(const std::string& password, const Bytes& salt, std::uint32_t iterations,
                       std::size_t derivedKeyLength) {
    const Bytes key(password.begin(), password.end());
    const HmacSha256 hmac(key);
    Bytes result;
    result.reserve(derivedKeyLength);
    const std::size_t blocks = (derivedKeyLength + 31) / 32;
    for (std::size_t blockIndex = 1; blockIndex <= blocks; ++blockIndex) {
        Bytes message = salt;
        message.push_back(static_cast<std::uint8_t>((blockIndex >> 24) & 0xFF));
        message.push_back(static_cast<std::uint8_t>((blockIndex >> 16) & 0xFF));
        message.push_back(static_cast<std::uint8_t>((blockIndex >> 8) & 0xFF));
        message.push_back(static_cast<std::uint8_t>(blockIndex & 0xFF));
        std::uint8_t u[32];
        std::uint8_t next[32];
        std::uint8_t t[32];
        hmac.compute(message.data(), message.size(), u);
        std::copy(u, u + 32, t);
        for (std::uint32_t i = 1; i < iterations; ++i) {
            hmac.compute(u, 32, next);
            std::copy(next, next + 32, u);
            for (int j = 0; j < 32; ++j) t[j] = static_cast<std::uint8_t>(t[j] ^ u[j]);
        }
        const std::size_t take = std::min<std::size_t>(32, derivedKeyLength - result.size());
        result.insert(result.end(), t, t + take);
    }
    return result;
}

std::string toHex(const Bytes& bytes) {
    static const char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (std::uint8_t b : bytes) {
        out += digits[b >> 4];
        out += digits[b & 0x0F];
    }
    return out;
}

bool fromHex(const std::string& text, Bytes& out) {
    if (text.size() % 2 != 0) return false;
    auto nibble = [](char c, int& value) {
        if (c >= '0' && c <= '9') value = c - '0';
        else if (c >= 'a' && c <= 'f') value = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') value = c - 'A' + 10;
        else return false;
        return true;
    };
    Bytes result;
    result.reserve(text.size() / 2);
    for (std::size_t i = 0; i < text.size(); i += 2) {
        int hi = 0, lo = 0;
        if (!nibble(text[i], hi) || !nibble(text[i + 1], lo)) return false;
        result.push_back(static_cast<std::uint8_t>(hi * 16 + lo));
    }
    out = std::move(result);
    return true;
}

bool isHex(const std::string& text, std::size_t expectedLength) {
    Bytes ignored;
    return text.size() == expectedLength && fromHex(text, ignored);
}

std::string generateSaltHex() { return randomHex(kSaltBytes); }

std::string randomHex(std::size_t byteCount) {
    // Salts only need to be unique, not secret. We mix std::random_device with the clock and
    // a stack address so that the result is unpredictable even on MinGW versions whose
    // random_device is deterministic.
    std::random_device device;
    const auto ticks = static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    int marker = 0;
    std::seed_seq seed{device(), device(), device(), device(), ticks,
                       static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(&marker))};
    std::mt19937_64 engine(seed);
    Bytes salt(byteCount);
    for (std::uint8_t& b : salt) {
        b = static_cast<std::uint8_t>((engine() ^ device()) >> 8);
    }
    return toHex(salt);
}

std::string derivePasswordHash(const std::string& password, const std::string& saltHex, std::uint32_t iterations) {
    Bytes salt;
    if (!fromHex(saltHex, salt) || salt.empty()) return "";
    return toHex(pbkdf2HmacSha256(password, salt, iterations, kHashBytes));
}

bool constantTimeEquals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    unsigned char diff = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        diff = static_cast<unsigned char>(diff | (static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i])));
    }
    return diff == 0;
}

}  // namespace security
