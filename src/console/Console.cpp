#include "Console.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "Utils.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <conio.h>
#include <windows.h>
#endif

namespace ui {

namespace {
const std::size_t kWidth = 70;

std::string rule(char fill = '-') { return std::string(kWidth, fill); }

void box(const std::string& text, char fill) {
    std::cout << '\n' << '+' << std::string(kWidth - 2, fill) << "+\n";
    std::cout << '|' << util::center(util::truncate(text, kWidth - 4), kWidth - 2) << "|\n";
    std::cout << '+' << std::string(kWidth - 2, fill) << "+\n";
}

bool isCancel(const std::string& text) { return util::toLower(util::trim(text)) == "/c"; }

}  // namespace

void init() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);     // so that the rupee sign is displayed correctly
#endif
    std::cout << std::unitbuf;
}

void banner(const std::string& text) { box(text, '='); }
void heading(const std::string& text) { box(text, '-'); }
void success(const std::string& text) { std::cout << "\n  [SUCCESS] " << text << '\n'; }
void error(const std::string& text) { std::cout << "\n  [ERROR]   " << text << '\n'; }
void info(const std::string& text) { std::cout << "\n  [INFO]    " << text << '\n'; }
void warning(const std::string& text) { std::cout << "\n  [WARNING] " << text << '\n'; }
void line() { std::cout << rule() << '\n'; }

void showResult(const Result& result) {
    if (result.ok) success(result.message);
    else error(result.message);
}

std::string readLine(const std::string& prompt, bool allowCancel) {
    std::cout << prompt << std::flush;
    std::string text;
    if (!std::getline(std::cin, text)) throw InputClosed();
    if (!text.empty() && text.back() == '\r') text.pop_back();
    if (allowCancel && isCancel(text)) throw CancelOperation();
    return text;
}

std::string ask(const std::string& label, const Validator& validator) {
    while (true) {
        const std::string value = util::trim(readLine("  " + label + ": "));
        const std::string problem = validator ? validator(value) : std::string();
        if (problem.empty()) return value;
        error(problem);
    }
}

std::string askKeep(const std::string& label, const std::string& current, const Validator& validator) {
    while (true) {
        const std::string value = util::trim(readLine("  " + label + " [" + current + "] (Enter = keep): "));
        if (value.empty()) return current;
        const std::string problem = validator ? validator(value) : std::string();
        if (problem.empty()) return value;
        error(problem);
    }
}

namespace {
bool parseIntInRange(const std::string& text, int minValue, int maxValue, int& out) {
    long long value = 0;
    if (!util::parseLong(text, value)) return false;
    if (value < minValue || value > maxValue) return false;
    out = static_cast<int>(value);
    return true;
}
}  // namespace

int askInt(const std::string& label, int minValue, int maxValue) {
    while (true) {
        const std::string text = util::trim(readLine("  " + label + ": "));
        int value = 0;
        if (parseIntInRange(text, minValue, maxValue, value)) return value;
        error("Please enter a whole number between " + std::to_string(minValue) + " and " + std::to_string(maxValue) + ".");
    }
}

int askIntKeep(const std::string& label, int minValue, int maxValue, int current) {
    while (true) {
        const std::string text = util::trim(readLine("  " + label + " [" + std::to_string(current) + "] (Enter = keep): "));
        if (text.empty()) return current;
        int value = 0;
        if (parseIntInRange(text, minValue, maxValue, value)) return value;
        error("Please enter a whole number between " + std::to_string(minValue) + " and " + std::to_string(maxValue) + ".");
    }
}

double askDouble(const std::string& label, double minValue, double maxValue) {
    while (true) {
        const std::string text = util::trim(readLine("  " + label + ": "));
        double value = 0.0;
        if (util::parseDouble(text, value) && value >= minValue && value <= maxValue) return value;
        error("Please enter a number between " + std::to_string(static_cast<long long>(minValue)) + " and " +
              std::to_string(static_cast<long long>(maxValue)) + ".");
    }
}

double askDoubleKeep(const std::string& label, double minValue, double maxValue, double current) {
    std::ostringstream shown;
    shown << std::fixed << std::setprecision(2) << current;
    while (true) {
        const std::string text = util::trim(readLine("  " + label + " [" + shown.str() + "] (Enter = keep): "));
        if (text.empty()) return current;
        double value = 0.0;
        if (util::parseDouble(text, value) && value >= minValue && value <= maxValue) return value;
        error("Please enter a number between " + std::to_string(static_cast<long long>(minValue)) + " and " +
              std::to_string(static_cast<long long>(maxValue)) + ".");
    }
}

std::string askPassword(const std::string& label) {
#ifdef _WIN32
    std::cout << "  " << label << ": " << std::flush;
    std::string password;
    while (true) {
        const int ch = _getch();
        if (ch == '\r' || ch == '\n') break;
        if (ch == 3) throw InputClosed();                       // Ctrl+C
        if (ch == 8) {                                          // Backspace
            if (!password.empty()) {
                password.pop_back();
                std::cout << "\b \b" << std::flush;
            }
        } else if (ch == 0 || ch == 224) {
            _getch();                                           // swallow arrow / function keys
        } else if (ch >= 32 && ch < 127) {
            password.push_back(static_cast<char>(ch));
            std::cout << '*' << std::flush;
        }
    }
    std::cout << '\n';
    if (isCancel(password)) throw CancelOperation();
    return password;
#else
    return readLine("  " + label + ": ");                       // no portable way to hide input
#endif
}

bool confirm(const std::string& question) {
    while (true) {
        const std::string answer = util::toLower(util::trim(readLine("  " + question + " (y/n): ")));
        if (answer == "y" || answer == "yes") return true;
        if (answer == "n" || answer == "no") return false;
        error("Please answer with 'y' or 'n'.");
    }
}

void pause() { readLine("\n  Press Enter to continue...", false); }

int menu(const std::string& title, const std::vector<std::string>& items, const std::string& zeroLabel) {
    heading(title);
    std::cout << '\n';
    for (std::size_t i = 0; i < items.size(); ++i) {
        std::cout << "   [" << (i + 1) << "] " << items[i] << '\n';
    }
    std::cout << "   [0] " << zeroLabel << "\n\n";
    while (true) {
        const std::string text = util::trim(readLine("  Enter your choice: ", false));
        int value = 0;
        if (parseIntInRange(text, 0, static_cast<int>(items.size()), value)) return value;
        error("Invalid selection. Please enter a number from 0 to " + std::to_string(items.size()) + ".");
    }
}

Table& Table::column(const std::string& header, std::size_t maxWidth, Align align) {
    columns_.push_back(Column{header, maxWidth, align});
    return *this;
}

void Table::addRow(std::vector<std::string> cells) {
    cells.resize(columns_.size());
    rows_.push_back(std::move(cells));
}

void Table::print() const {
    std::vector<std::size_t> widths(columns_.size(), 0);
    for (std::size_t c = 0; c < columns_.size(); ++c) {
        widths[c] = util::displayWidth(columns_[c].header);
        for (const auto& row : rows_) {
            widths[c] = std::max(widths[c], util::displayWidth(row[c]));
        }
        widths[c] = std::min(widths[c], std::max(columns_[c].maxWidth, util::displayWidth(columns_[c].header)));
    }
    std::string separator = "+";
    for (std::size_t w : widths) separator += std::string(w + 2, '-') + "+";

    auto printRow = [&](const std::vector<std::string>& cells, bool header) {
        std::cout << '|';
        for (std::size_t c = 0; c < columns_.size(); ++c) {
            const std::string text = util::truncate(cells[c], widths[c]);
            const bool right = !header && columns_[c].align == Align::Right;
            std::cout << ' ' << (right ? util::padLeft(text, widths[c]) : util::padRight(text, widths[c])) << " |";
        }
        std::cout << '\n';
    };

    std::vector<std::string> headers;
    for (const Column& col : columns_) headers.push_back(col.header);
    std::cout << '\n' << separator << '\n';
    printRow(headers, true);
    std::cout << separator << '\n';
    for (const auto& row : rows_) printRow(row, false);
    std::cout << separator << '\n';
}

}  // namespace ui
