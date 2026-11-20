// Date.h - calendar date with safe arithmetic (no string comparisons for date maths).
#pragma once
#include <string>

class Date {
public:
    Date();                          // 1970-01-01
    Date(int year, int month, int day);

    static Date today();
    static bool parse(const std::string& text, Date& out);   // strict YYYY-MM-DD
    static bool isLeapYear(int year);
    static int daysInMonth(int year, int month);
    static Date fromDayNumber(long long dayNumber);

    bool isValid() const;
    int year() const { return year_; }
    int month() const { return month_; }
    int day() const { return day_; }

    long long toDayNumber() const;          // days since 1970-01-01
    Date addDays(long long days) const;
    long long daysAfter(const Date& other) const;   // (*this - other) in days
    std::string toString() const;           // YYYY-MM-DD

    bool operator==(const Date& o) const { return toDayNumber() == o.toDayNumber(); }
    bool operator!=(const Date& o) const { return !(*this == o); }
    bool operator<(const Date& o) const { return toDayNumber() < o.toDayNumber(); }
    bool operator<=(const Date& o) const { return toDayNumber() <= o.toDayNumber(); }
    bool operator>(const Date& o) const { return o < *this; }
    bool operator>=(const Date& o) const { return o <= *this; }

private:
    int year_;
    int month_;
    int day_;
};
