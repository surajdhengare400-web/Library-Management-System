#include "Member.h"

#include "FileManager.h"
#include "Utils.h"

Member::Member(std::string id, std::string fullName, std::string email, std::string phone, Date registrationDate)
    : Person(std::move(fullName), std::move(email)),
      id_(std::move(id)),
      phone_(std::move(phone)),
      registrationDate_(registrationDate) {}

std::string Member::summary() const { return id_ + " - " + fullName(); }

std::string Member::serialize() const {
    return FileManager::join({id_, fullName(), email(), phone_, registrationDate_.toString()});
}

std::optional<Member> Member::parse(const std::vector<std::string>& f) {
    if (f.size() != 5) return std::nullopt;
    const std::string id = util::toUpper(util::trim(f[0]));
    if (!validate::checkId(id).empty()) return std::nullopt;
    if (!validate::checkName(f[1]).empty()) return std::nullopt;
    if (!validate::checkEmail(util::trim(f[2])).empty()) return std::nullopt;
    if (!validate::checkPhone(f[3]).empty()) return std::nullopt;
    Date registered;
    if (!Date::parse(f[4], registered)) return std::nullopt;
    return Member(id, util::trim(f[1]), util::trim(f[2]), validate::normalizePhone(f[3]), registered);
}
