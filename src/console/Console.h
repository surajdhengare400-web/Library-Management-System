// Console.h - console user-interface helpers: banners, menus, tables, validated input.
#pragma once
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include "Result.h"

namespace ui {

// Thrown when the input stream ends (Ctrl+Z / Ctrl+C): the program exits cleanly.
class InputClosed : public std::runtime_error {
public:
    InputClosed() : std::runtime_error("Input stream closed") {}
};

// Thrown when the user types /c at a prompt: the current operation is abandoned.
class CancelOperation : public std::runtime_error {
public:
    CancelOperation() : std::runtime_error("Operation cancelled") {}
};

using Validator = std::function<std::string(const std::string&)>;   // "" = valid, else error text

void init();                                      // UTF-8 console output on Windows

void banner(const std::string& text);             // application title box
void heading(const std::string& text);            // section heading box
void success(const std::string& text);
void error(const std::string& text);
void info(const std::string& text);
void warning(const std::string& text);
void showResult(const Result& result);
void line();                                      // horizontal rule

std::string readLine(const std::string& prompt, bool allowCancel = true);
std::string ask(const std::string& label, const Validator& validator);
std::string askKeep(const std::string& label, const std::string& current, const Validator& validator);
int askInt(const std::string& label, int minValue, int maxValue);
int askIntKeep(const std::string& label, int minValue, int maxValue, int current);
double askDouble(const std::string& label, double minValue, double maxValue);
double askDoubleKeep(const std::string& label, double minValue, double maxValue, double current);
std::string askPassword(const std::string& label);
bool confirm(const std::string& question);
void pause();

// Prints a numbered menu (0 = back/exit) and returns the validated choice.
int menu(const std::string& title, const std::vector<std::string>& items, const std::string& zeroLabel);

// Simple ASCII table with automatic column widths.
class Table {
public:
    enum class Align { Left, Right };

    Table& column(const std::string& header, std::size_t maxWidth = 30, Align align = Align::Left);
    void addRow(std::vector<std::string> cells);
    std::size_t rowCount() const { return rows_.size(); }
    void print() const;

private:
    struct Column {
        std::string header;
        std::size_t maxWidth;
        Align align;
    };
    std::vector<Column> columns_;
    std::vector<std::vector<std::string>> rows_;
};

}  // namespace ui
