#include "Admin.h"

#include "FileManager.h"
#include "Security.h"
#include "Utils.h"

std::string roleToString(Role role) { return role == Role::SuperAdmin ? "Super Admin" : "Admin"; }

Admin::Admin(std::string username, std::string fullName, std::string email, Role role, bool active,
             std::string saltHex, std::string hashHex, std::uint32_t iterations, std::string createdAt)
    : Person(std::move(fullName), std::move(email)),
      username_(std::move(username)),
      role_(role),
      active_(active),
      saltHex_(std::move(saltHex)),
      hashHex_(std::move(hashHex)),
      iterations_(iterations),
      createdAt_(std::move(createdAt)) {}

Admin Admin::create(const std::string& username, const std::string& fullName, const std::string& email, Role role,
                    const std::string& plainPassword, const std::string& createdAt) {
    Admin admin(username, fullName, email, role, true, "", "", security::kDefaultIterations, createdAt);
    admin.setPassword(plainPassword);
    return admin;
}

bool Admin::verifyPassword(const std::string& plainPassword) const {
    const std::string candidate = security::derivePasswordHash(plainPassword, saltHex_, iterations_);
    return !candidate.empty() && security::constantTimeEquals(candidate, hashHex_);
}

void Admin::setPassword(const std::string& plainPassword) {
    saltHex_ = security::generateSaltHex();
    iterations_ = security::kDefaultIterations;
    hashHex_ = security::derivePasswordHash(plainPassword, saltHex_, iterations_);
}

std::string Admin::summary() const { return username_ + " (" + roleToString(role_) + ")"; }

std::string Admin::serialize() const {
    return FileManager::join({username_, isSuperAdmin() ? "SUPER_ADMIN" : "ADMIN", active_ ? "1" : "0", fullName(),
                              email(), std::to_string(iterations_), saltHex_, hashHex_, createdAt_});
}

std::optional<Admin> Admin::parse(const std::vector<std::string>& f) {
    if (f.size() != 9) return std::nullopt;
    const std::string username = util::toLower(f[0]);
    if (!validate::checkUsername(username).empty()) return std::nullopt;
    Role role;
    if (f[1] == "SUPER_ADMIN") role = Role::SuperAdmin;
    else if (f[1] == "ADMIN") role = Role::Admin;
    else return std::nullopt;
    if (f[2] != "0" && f[2] != "1") return std::nullopt;
    if (!validate::checkName(f[3]).empty()) return std::nullopt;
    if (!validate::checkEmail(f[4]).empty()) return std::nullopt;
    long long iterations = 0;
    if (!util::parseLong(f[5], iterations) || iterations < security::kMinIterations ||
        iterations > security::kMaxIterations) {
        return std::nullopt;
    }
    if (!security::isHex(f[6], security::kSaltBytes * 2) || !security::isHex(f[7], security::kHashBytes * 2)) {
        return std::nullopt;
    }
    return Admin(username, util::trim(f[3]), f[4], role, f[2] == "1", f[6], f[7],
                 static_cast<std::uint32_t>(iterations), f[8]);
}
