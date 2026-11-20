#include "Utils.h"

#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace util {

namespace {
std::size_t utf8Length(unsigned char lead) {
    if (lead < 0x80) return 1;
    if ((lead & 0xE0) == 0xC0) return 2;
    if ((lead & 0xF0) == 0xE0) return 3;
    if ((lead & 0xF8) == 0xF0) return 4;
    return 1;
}
}  // namespace

std::string trim(const std::string& s) {
    std::size_t begin = 0;
    std::size_t end = s.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(s[begin]))) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(begin, end - begin);
}

std::string toLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string toUpper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

bool containsIgnoreCase(const std::string& haystack, const std::string& needle) {
    return toLower(haystack).find(toLower(needle)) != std::string::npos;
}

bool hasControlChars(const std::string& s) {
    for (char c : s) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (u < 0x20 || u == 0x7F) return true;
    }
    return false;
}

bool parseLong(const std::string& text, long long& out) {
    if (text.empty() || std::isspace(static_cast<unsigned char>(text[0]))) return false;
    errno = 0;
    char* end = nullptr;
    const long long value = std::strtoll(text.c_str(), &end, 10);
    if (errno == ERANGE || end == text.c_str() || *end != '\0') return false;
    out = value;
    return true;
}

bool parseDouble(const std::string& text, double& out) {
    if (text.empty()) return false;
    for (char c : text) {
        if (!(std::isdigit(static_cast<unsigned char>(c)) || c == '.' || c == '-' || c == '+')) return false;
    }
    errno = 0;
    char* end = nullptr;
    const double value = std::strtod(text.c_str(), &end);
    if (errno == ERANGE || end == text.c_str() || *end != '\0' || !std::isfinite(value)) return false;
    out = value;
    return true;
}

double roundMoney(double value) { return std::round(value * 100.0) / 100.0; }

std::string formatMoney(double amount) {
    std::ostringstream out;
    out << "\xE2\x82\xB9" << std::fixed << std::setprecision(2) << amount;   // ₹ (UTF-8)
    return out.str();
}

std::string formatId(char prefix, int number) {
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "%c%04d", prefix, number);
    return std::string(buffer);
}

std::string currentTimestamp() {
    std::time_t now = std::time(nullptr);
    const std::tm* local = std::localtime(&now);
    char buffer[32];
    if (local == nullptr || std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local) == 0) {
        return "1970-01-01 00:00:00";
    }
    return std::string(buffer);
}

int currentYear() {
    std::time_t now = std::time(nullptr);
    const std::tm* local = std::localtime(&now);
    return local == nullptr ? 2025 : local->tm_year + 1900;
}

std::size_t displayWidth(const std::string& s) {
    std::size_t width = 0;
    for (char c : s) {
        if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) ++width;
    }
    return width;
}

std::string padRight(const std::string& s, std::size_t width) {
    const std::size_t w = displayWidth(s);
    return w >= width ? s : s + std::string(width - w, ' ');
}

std::string padLeft(const std::string& s, std::size_t width) {
    const std::size_t w = displayWidth(s);
    return w >= width ? s : std::string(width - w, ' ') + s;
}

std::string center(const std::string& s, std::size_t width) {
    const std::size_t w = displayWidth(s);
    if (w >= width) return s;
    const std::size_t left = (width - w) / 2;
    return std::string(left, ' ') + s + std::string(width - w - left, ' ');
}

std::string truncate(const std::string& s, std::size_t width) {
    if (displayWidth(s) <= width) return s;
    const std::size_t keep = width > 3 ? width - 3 : width;
    std::string out;
    std::size_t count = 0;
    for (std::size_t i = 0; i < s.size() && count < keep;) {
        const std::size_t len = utf8Length(static_cast<unsigned char>(s[i]));
        out.append(s, i, len);
        i += len;
        ++count;
    }
    return width > 3 ? out + "..." : out;
}

std::string repeat(const std::string& unit, std::size_t count) {
    std::string out;
    out.reserve(unit.size() * count);
    for (std::size_t i = 0; i < count; ++i) out += unit;
    return out;
}

}  // namespace util

namespace validate {

std::string checkId(const std::string& value) {
    if (value.size() < 3 || value.size() > 12) return "ID must be 3 to 12 characters long.";
    for (char c : value) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (!(std::isalnum(u) || c == '-')) return "ID may contain only letters, digits and '-'.";
    }
    return "";
}

std::string checkText(const std::string& value, const std::string& label, std::size_t minLen,
                      std::size_t maxLen) {
    const std::string v = util::trim(value);
    if (v.empty()) return label + " cannot be empty.";
    if (util::hasControlChars(v)) return label + " contains invalid characters.";
    const std::size_t len = util::displayWidth(v);
    if (len < minLen) return label + " must be at least " + std::to_string(minLen) + " characters.";
    if (len > maxLen) return label + " must be at most " + std::to_string(maxLen) + " characters.";
    return "";
}

std::string checkName(const std::string& value) {
    std::string err = checkText(value, "Name", 2, 60);
    if (!err.empty()) return err;
    for (char c : value) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (std::isalpha(u) || u >= 0x80) return "";
    }
    return "Name must contain at least one letter.";
}

std::string checkEmail(const std::string& value) {
    if (value.empty()) return "Email cannot be empty.";
    if (value.size() > 100) return "Email is too long (maximum 100 characters).";
    for (char c : value) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (u <= 0x20 || u >= 0x7F) return "Email contains invalid characters or spaces.";
    }
    const std::size_t at = value.find('@');
    if (at == std::string::npos || at != value.rfind('@')) return "Email must contain exactly one '@'.";
    const std::string local = value.substr(0, at);
    const std::string domain = value.substr(at + 1);
    if (local.empty() || local.size() > 64) return "Email has an invalid name before '@'.";
    const std::size_t dot = domain.rfind('.');
    if (dot == std::string::npos || dot == 0 || dot + 3 > domain.size() || domain.front() == '.' ||
        domain.find("..") != std::string::npos) {
        return "Email has an invalid domain (example: name@example.com).";
    }
    return "";
}

std::string normalizePhone(const std::string& value) {
    std::string out;
    for (char c : util::trim(value)) {
        if (c != ' ' && c != '-') out += c;
    }
    return out;
}

std::string checkPhone(const std::string& value) {
    const std::string p = normalizePhone(value);
    if (p.empty()) return "Phone number cannot be empty.";
    std::size_t digits = 0;
    for (std::size_t i = 0; i < p.size(); ++i) {
        if (i == 0 && p[i] == '+') continue;
        if (!std::isdigit(static_cast<unsigned char>(p[i]))) {
            return "Phone number may contain only digits, spaces, '-' and a leading '+'.";
        }
        ++digits;
    }
    if (digits < 7 || digits > 15) return "Phone number must have 7 to 15 digits.";
    return "";
}

std::string checkUsername(const std::string& value) {
    if (value.size() < 3 || value.size() > 20) return "Username must be 3 to 20 characters long.";
    for (char c : value) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (!(std::isalnum(u) || c == '_')) return "Username may contain only letters, digits and '_'.";
    }
    return "";
}

std::string checkPassword(const std::string& password, const std::string& username) {
    if (password.size() < 8) return "Password must be at least 8 characters long.";
    if (password.size() > 64) return "Password must be at most 64 characters long.";
    bool upper = false, lower = false, digit = false, special = false;
    for (char c : password) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (std::isspace(u)) return "Password must not contain spaces.";
        if (std::isupper(u)) upper = true;
        else if (std::islower(u)) lower = true;
        else if (std::isdigit(u)) digit = true;
        else special = true;
    }
    if (!(upper && lower && digit && special)) {
        return "Password must contain an uppercase letter, a lowercase letter, a digit and a special character.";
    }
    if (username.size() >= 3 && util::containsIgnoreCase(password, username)) {
        return "Password must not contain the username.";
    }
    const std::string lowered = util::toLower(password);
    if (lowered.find("password") != std::string::npos || lowered.find("qwerty") != std::string::npos ||
        lowered.find("12345678") != std::string::npos) {
        return "Password is too common. Choose something less predictable.";
    }
    return "";
}

std::string checkYear(long long year) {
    const int current = util::currentYear();
    if (year < 1000 || year > current) {
        return "Publication year must be between 1000 and " + std::to_string(current) + ".";
    }
    return "";
}

}  // namespace validate
