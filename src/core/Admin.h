// Admin.h - an administrator account. Passwords are never stored: only a random salt and a
// PBKDF2-HMAC-SHA256 hash (with its iteration count) are kept.
#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "Person.h"
#include "Serializable.h"

enum class Role { SuperAdmin, Admin };

std::string roleToString(Role role);          // "Super Admin" / "Admin"

class Admin : public Person, public Serializable {
public:
    // Creates a brand-new account: generates a salt and hashes the password.
    static Admin create(const std::string& username, const std::string& fullName, const std::string& email,
                        Role role, const std::string& plainPassword, const std::string& createdAt);

    // Restores an account from stored values (used when loading).
    Admin(std::string username, std::string fullName, std::string email, Role role, bool active,
          std::string saltHex, std::string hashHex, std::uint32_t iterations, std::string createdAt);

    const std::string& username() const { return username_; }
    Role role() const { return role_; }
    bool isSuperAdmin() const { return role_ == Role::SuperAdmin; }
    bool isActive() const { return active_; }
    const std::string& createdAt() const { return createdAt_; }
    void setActive(bool value) { active_ = value; }

    bool verifyPassword(const std::string& plainPassword) const;
    void setPassword(const std::string& plainPassword);       // new salt + new hash

    std::string summary() const override;
    std::string serialize() const override;
    static std::optional<Admin> parse(const std::vector<std::string>& fields);

private:
    std::string username_;
    Role role_;
    bool active_;
    std::string saltHex_;
    std::string hashHex_;
    std::uint32_t iterations_;
    std::string createdAt_;
};
