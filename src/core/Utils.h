// Utils.h - string helpers, number formatting and input validators (standard C++ only).
#pragma once
#include <cstddef>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace util {

std::string trim(const std::string& s);
std::string toLower(std::string s);
std::string toUpper(std::string s);
bool containsIgnoreCase(const std::string& haystack, const std::string& needle);
bool hasControlChars(const std::string& s);

bool parseLong(const std::string& text, long long& out);
bool parseDouble(const std::string& text, double& out);
double roundMoney(double value);
std::string formatMoney(double amount);                 // e.g. "₹12.50"
std::string formatId(char prefix, int number);          // e.g. L0007

std::string currentTimestamp();                         // YYYY-MM-DD HH:MM:SS (local time)
int currentYear();

std::size_t displayWidth(const std::string& s);         // counts UTF-8 code points
std::string padRight(const std::string& s, std::size_t width);
std::string padLeft(const std::string& s, std::size_t width);
std::string center(const std::string& s, std::size_t width);
std::string truncate(const std::string& s, std::size_t width);
std::string repeat(const std::string& unit, std::size_t count);

// Removes records whose key was already seen; returns how many were dropped.
template <typename T, typename KeyFn>
std::size_t dropDuplicates(std::vector<T>& items, KeyFn keyOf) {
    using Key = std::decay_t<decltype(keyOf(std::declval<const T&>()))>;
    std::set<Key> seen;
    std::vector<T> unique;
    unique.reserve(items.size());
    std::size_t dropped = 0;
    for (T& item : items) {
        if (seen.insert(keyOf(item)).second) {
            unique.push_back(std::move(item));
        } else {
            ++dropped;
        }
    }
    items = std::move(unique);
    return dropped;
}

}  // namespace util

// Validators return an empty string when the value is valid, otherwise an error message.
namespace validate {

std::string checkId(const std::string& value);                       // 3-12 chars: letters, digits, '-'
std::string checkName(const std::string& value);
std::string checkText(const std::string& value, const std::string& label,
                      std::size_t minLen, std::size_t maxLen);
std::string checkEmail(const std::string& value);
std::string normalizePhone(const std::string& value);
std::string checkPhone(const std::string& value);
std::string checkUsername(const std::string& value);
std::string checkPassword(const std::string& password, const std::string& username);
std::string checkYear(long long year);

}  // namespace validate
