// AdminManager.h - authentication, sessions and role-based administration of admin accounts.
// Permission rules are enforced here (not only in the menus), so they cannot be bypassed.
#pragma once
#include <optional>
#include <string>
#include <vector>

#include "Admin.h"
#include "FileManager.h"
#include "Library.h"
#include "Result.h"

class AdminManager {
public:
    AdminManager(const FileManager& files, Library& library);

    bool load(std::vector<std::string>& warnings, std::string& fatalError);

    bool hasAdmins() const { return !admins_.empty(); }
    const std::vector<Admin>& admins() const { return admins_; }
    std::size_t activeAdminCount() const;
    std::size_t activeSuperAdminCount() const;

    // ---- session ------------------------------------------------------------------------
    Result createFirstSuperAdmin(const std::string& username, const std::string& fullName,
                                 const std::string& email, const std::string& password);
    Result login(const std::string& username, const std::string& password);
    void logout();
    // For servers that keep their own session table: makes `username` the current user if the
    // account exists and is active (used once per request by the single-threaded web server).
    bool resumeSession(const std::string& username);
    bool isLoggedIn() const { return !currentUser_.empty(); }
    bool isSuperAdmin() const;
    std::string currentUsername() const { return currentUser_; }
    std::optional<Admin> currentAdmin() const;

    // ---- own account --------------------------------------------------------------------
    Result changeOwnPassword(const std::string& currentPassword, const std::string& newPassword);
    Result updateOwnProfile(const std::string& fullName, const std::string& email);

    // ---- Super Admin only ---------------------------------------------------------------
    Result createAdmin(const std::string& username, const std::string& fullName, const std::string& email,
                       const std::string& password, Role role);
    Result setActive(const std::string& username, bool active);
    Result resetPassword(const std::string& username, const std::string& newPassword);

private:
    std::size_t indexOf(const std::string& username) const;
    bool persist(std::string& error) const;
    Result requireSuperAdmin() const;

    const FileManager& files_;
    Library& library_;
    std::vector<Admin> admins_;
    std::string currentUser_;
};
