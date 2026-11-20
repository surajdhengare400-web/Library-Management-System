#include "Transaction.h"

#include "Date.h"
#include "FileManager.h"
#include "Utils.h"

Transaction::Transaction(int id, std::string type, std::string description, std::string timestamp)
    : id_(id), type_(std::move(type)), description_(std::move(description)), timestamp_(std::move(timestamp)) {}

std::string Transaction::serialize() const {
    return FileManager::join({std::to_string(id_), type_, timestamp_, description_});
}

std::optional<Transaction> Transaction::parse(const std::vector<std::string>& f) {
    if (f.size() != 4) return std::nullopt;
    long long id = 0;
    if (!util::parseLong(f[0], id) || id < 1 || id > 99999999) return std::nullopt;
    if (f[1].empty() || f[1].size() > 30 || util::hasControlChars(f[1])) return std::nullopt;
    Date day;
    if (f[2].size() != 19 || !Date::parse(f[2].substr(0, 10), day)) return std::nullopt;
    if (f[3].empty()) return std::nullopt;
    return Transaction(static_cast<int>(id), f[1], f[3], f[2]);
}
