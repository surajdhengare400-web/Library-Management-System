#include "Date.h"

#include <cstdio>
#include <ctime>

namespace {

// Days-from-civil algorithm (proleptic Gregorian calendar), epoch 1970-01-01.
long long daysFromCivil(long long y, int m, int d) {
    y -= (m <= 2) ? 1 : 0;
    const long long era = (y >= 0 ? y : y - 399) / 400;
    const long long yoe = y - era * 400;
    const long long doy = (153LL * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

void civilFromDays(long long z, int& y, int& m, int& d) {
    z += 719468;
    const long long era = (z >= 0 ? z : z - 146096) / 146097;
    const long long doe = z - era * 146097;
    const long long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const long long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const long long mp = (5 * doy + 2) / 153;
    const long long dd = doy - (153 * mp + 2) / 5 + 1;
    const long long mm = mp < 10 ? mp + 3 : mp - 9;
    y = static_cast<int>(yoe + era * 400 + (mm <= 2 ? 1 : 0));
    m = static_cast<int>(mm);
    d = static_cast<int>(dd);
}

bool isDigit(char c) { return c >= '0' && c <= '9'; }

}  // namespace

Date::Date() : year_(1970), month_(1), day_(1) {}

Date::Date(int year, int month, int day) : year_(year), month_(month), day_(day) {}

Date Date::today() {
    std::time_t now = std::time(nullptr);
    const std::tm* local = std::localtime(&now);
    if (local == nullptr) return Date();
    return Date(local->tm_year + 1900, local->tm_mon + 1, local->tm_mday);
}

bool Date::isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int Date::daysInMonth(int year, int month) {
    static const int table[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return 0;
    if (month == 2 && isLeapYear(year)) return 29;
    return table[month - 1];
}

bool Date::parse(const std::string& text, Date& out) {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-') return false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (i == 4 || i == 7) continue;
        if (!isDigit(text[i])) return false;
    }
    const int y = (text[0] - '0') * 1000 + (text[1] - '0') * 100 + (text[2] - '0') * 10 + (text[3] - '0');
    const int m = (text[5] - '0') * 10 + (text[6] - '0');
    const int d = (text[8] - '0') * 10 + (text[9] - '0');
    if (y < 1900 || m < 1 || m > 12 || d < 1 || d > daysInMonth(y, m)) return false;
    out = Date(y, m, d);
    return true;
}

bool Date::isValid() const {
    return year_ >= 1900 && year_ <= 9999 && month_ >= 1 && month_ <= 12 && day_ >= 1 &&
           day_ <= daysInMonth(year_, month_);
}

long long Date::toDayNumber() const { return daysFromCivil(year_, month_, day_); }

Date Date::fromDayNumber(long long dayNumber) {
    int y = 1970, m = 1, d = 1;
    civilFromDays(dayNumber, y, m, d);
    return Date(y, m, d);
}

Date Date::addDays(long long days) const { return fromDayNumber(toDayNumber() + days); }

long long Date::daysAfter(const Date& other) const { return toDayNumber() - other.toDayNumber(); }

std::string Date::toString() const {
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", year_, month_, day_);
    return std::string(buffer);
}
