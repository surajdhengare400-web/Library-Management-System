// Result.h - small value type used by every operation that can fail.
#pragma once
#include <string>
#include <utility>

struct Result {
    bool ok;
    std::string message;

    static Result success(std::string text = "") { return Result{true, std::move(text)}; }
    static Result failure(std::string text) { return Result{false, std::move(text)}; }
};
