#include "AdminManager.h"

#include <limits>

#include "Security.h"
#include "Utils.h"

namespace {
const std::size_t kNotFound = std::numeric_limits<std::size_t>::max();
constexpr const char* kAdminsFile = "admins.txt";
// Used to spend the same time on unknown usernames as on real ones (no user enumeration by timing).
constexpr const char* kDummySalt = "00112233445566778899aabbccddeeff";
}  // namespace

AdminManager::AdminManager(const FileManager& files, Library& library) : files_(files), library_(library) {}

bool AdminManager::load(std::vector<std::string>& warnings, std::string& fatalError) {
    admins_.clear();
    currentUser_.clear();
    if (!files_.loadRecords<Admin>(kAdminsFile, admins_, warnings, fatalError)) return false;
    const std::size_t dropped = util::dropDuplicates(admins_, [](const Admin& a) { return a.username(); });
    if (dropped > 0) {
        std::string backup;
        files_.backupCorrupted(kAdminsFile, backup);
        warnings.push_back(std::string(kAdminsFile) + ": " + std::to_string(dropped) +
                           " duplicate username record(s) ignored. Original copied to " + backup + ".");
    }
    if (!admins_.empty() && activeSuperAdminCount() == 0) {
        warnings.push_back("There is no active Super Admin account. Administrator management is unavailable.");
    }
    return true;
}

std::size_t AdminManager::activeAdminCount() const {
    std::size_t n = 0;
    for (const Admin& a : admins_) if (a.isActive()) ++n;
    return n;
}

std::size_t AdminManager::activeSuperAdminCount() const {
    std::size_t n = 0;
    for (const Admin& a : admins_) if (a.isActive() && a.isSuperAdmin()) ++n;
    return n;
}

std::size_t AdminManager::indexOf(const std::string& username) const {
    const std::string key = util::toLower(util::trim(username));
    for (std::size_t i = 0; i < admins_.size(); ++i) {
        if (admins_[i].username() == key) return i;
    }
    return kNotFound;
}

bool AdminManager::persist(std::string& error) const { return files_.saveRecords(kAdminsFile, admins_, error); }

bool AdminManager::isSuperAdmin() const {
    const std::size_t index = indexOf(currentUser_);
    return index != kNotFound && admins_[index].isSuperAdmin() && admins_[index].isActive();
}

std::optional<Admin> AdminManager::currentAdmin() const {
    const std::size_t index = indexOf(currentUser_);
    if (currentUser_.empty() || index == kNotFound) return std::nullopt;
    return admins_[index];
}

Result AdminManager::requireSuperAdmin() const {
    if (!isLoggedIn()) return Result::failure("You must be logged in.");
    if (!isSuperAdmin()) return Result::failure("Access denied: only a Super Admin can do this.");
    return Result::success();
}

Result AdminManager::createFirstSuperAdmin(const std::string& rawUsername, const std::string& fullName,
                                           const std::string& email, const std::string& password) {
    if (!admins_.empty()) return Result::failure("An administrator account already exists.");
    const std::string username = util::toLower(util::trim(rawUsername));
    std::string err = validate::checkUsername(username);
    if (err.empty()) err = validate::checkName(fullName);
    if (err.empty()) err = validate::checkEmail(util::trim(email));
    if (err.empty()) err = validate::checkPassword(password, username);
    if (!err.empty()) return Result::failure(err);

    admins_.push_back(Admin::create(username, util::trim(fullName), util::trim(email), Role::SuperAdmin, password,
                                    util::currentTimestamp()));
    std::string saveError;
    if (!persist(saveError)) {
        admins_.clear();
        return Result::failure(saveError);
    }
    library_.setActor("");
    library_.logActivity(Activity::AdminCreated, "Super Admin '" + username + "' created during first-time setup");
    return Result::success("Super Admin account '" + username + "' created.");
}

Result AdminManager::login(const std::string& rawUsername, const std::string& password) {
    const std::size_t index = indexOf(rawUsername);
    if (index == kNotFound) {
        security::derivePasswordHash(password, kDummySalt, security::kDefaultIterations);
        return Result::failure("Invalid username or password.");
    }
    if (!admins_[index].verifyPassword(password)) return Result::failure("Invalid username or password.");
    if (!admins_[index].isActive()) return Result::failure("This account is deactivated. Please contact a Super Admin.");
    currentUser_ = admins_[index].username();
    library_.setActor(currentUser_);
    return Result::success("Welcome, " + admins_[index].fullName() + "!");
}

bool AdminManager::resumeSession(const std::string& username) {
    const std::size_t index = indexOf(username);
    if (index == kNotFound || !admins_[index].isActive()) {
        logout();
        return false;
    }
    currentUser_ = admins_[index].username();
    library_.setActor(currentUser_);
    return true;
}

void AdminManager::logout() {
    currentUser_.clear();
    library_.setActor("");
}

Result AdminManager::changeOwnPassword(const std::string& currentPassword, const std::string& newPassword) {
    const std::size_t index = indexOf(currentUser_);
    if (!isLoggedIn() || index == kNotFound) return Result::failure("You must be logged in.");
    if (!admins_[index].verifyPassword(currentPassword)) return Result::failure("The current password is incorrect.");
    const std::string err = validate::checkPassword(newPassword, admins_[index].username());
    if (!err.empty()) return Result::failure(err);
    if (newPassword == currentPassword) return Result::failure("The new password must be different from the current one.");

    std::vector<Admin> backup = admins_;
    admins_[index].setPassword(newPassword);
    std::string saveError;
    if (!persist(saveError)) {
        admins_ = std::move(backup);
        return Result::failure(saveError);
    }
    Result result = Result::success("Password changed successfully.");
    if (!library_.logActivity(Activity::PasswordChanged, "Password changed for '" + currentUser_ + "'")) {
        result.message += " (Warning: the transaction history could not be saved.)";
    }
    return result;
}

Result AdminManager::updateOwnProfile(const std::string& fullName, const std::string& email) {
    const std::size_t index = indexOf(currentUser_);
    if (!isLoggedIn() || index == kNotFound) return Result::failure("You must be logged in.");
    std::string err = validate::checkName(fullName);
    if (err.empty()) err = validate::checkEmail(util::trim(email));
    if (!err.empty()) return Result::failure(err);

    std::vector<Admin> backup = admins_;
    admins_[index].setFullName(util::trim(fullName));
    admins_[index].setEmail(util::trim(email));
    std::string saveError;
    if (!persist(saveError)) {
        admins_ = std::move(backup);
        return Result::failure(saveError);
    }
    Result result = Result::success("Profile updated.");
    if (!library_.logActivity(Activity::ProfileUpdated, "Profile updated for '" + currentUser_ + "'")) {
        result.message += " (Warning: the transaction history could not be saved.)";
    }
    return result;
}

Result AdminManager::createAdmin(const std::string& rawUsername, const std::string& fullName, const std::string& email,
                                 const std::string& password, Role role) {
    const Result permission = requireSuperAdmin();
    if (!permission.ok) return permission;
    const std::string username = util::toLower(util::trim(rawUsername));
    std::string err = validate::checkUsername(username);
    if (err.empty()) err = validate::checkName(fullName);
    if (err.empty()) err = validate::checkEmail(util::trim(email));
    if (err.empty()) err = validate::checkPassword(password, username);
    if (!err.empty()) return Result::failure(err);
    if (indexOf(username) != kNotFound) return Result::failure("The username '" + username + "' is already taken.");

    std::vector<Admin> backup = admins_;
    admins_.push_back(Admin::create(username, util::trim(fullName), util::trim(email), role, password,
                                    util::currentTimestamp()));
    std::string saveError;
    if (!persist(saveError)) {
        admins_ = std::move(backup);
        return Result::failure(saveError);
    }
    Result result = Result::success("Account '" + username + "' created as " + roleToString(role) + ".");
    if (!library_.logActivity(Activity::AdminCreated, "Created " + roleToString(role) + " account '" + username + "'")) {
        result.message += " (Warning: the transaction history could not be saved.)";
    }
    return result;
}

Result AdminManager::setActive(const std::string& rawUsername, bool active) {
    const Result permission = requireSuperAdmin();
    if (!permission.ok) return permission;
    const std::size_t index = indexOf(rawUsername);
    if (index == kNotFound) return Result::failure("No administrator found with that username.");
    const Admin& target = admins_[index];
    if (target.isActive() == active) {
        return Result::failure(std::string("The account is already ") + (active ? "active." : "deactivated."));
    }
    if (!active) {
        if (target.username() == currentUser_) return Result::failure("You cannot deactivate your own account.");
        if (target.isSuperAdmin() && activeSuperAdminCount() <= 1) {
            return Result::failure("The last active Super Admin cannot be deactivated.");
        }
    }

    std::vector<Admin> backup = admins_;
    admins_[index].setActive(active);
    std::string saveError;
    if (!persist(saveError)) {
        admins_ = std::move(backup);
        return Result::failure(saveError);
    }
    const std::string name = admins_[index].username();
    Result result = Result::success("Account '" + name + "' was " + (active ? "activated." : "deactivated."));
    if (!library_.logActivity(active ? Activity::AdminActivated : Activity::AdminDeactivated,
                              std::string(active ? "Activated" : "Deactivated") + " account '" + name + "'")) {
        result.message += " (Warning: the transaction history could not be saved.)";
    }
    return result;
}

Result AdminManager::resetPassword(const std::string& rawUsername, const std::string& newPassword) {
    const Result permission = requireSuperAdmin();
    if (!permission.ok) return permission;
    const std::size_t index = indexOf(rawUsername);
    if (index == kNotFound) return Result::failure("No administrator found with that username.");
    const std::string err = validate::checkPassword(newPassword, admins_[index].username());
    if (!err.empty()) return Result::failure(err);

    std::vector<Admin> backup = admins_;
    admins_[index].setPassword(newPassword);
    std::string saveError;
    if (!persist(saveError)) {
        admins_ = std::move(backup);
        return Result::failure(saveError);
    }
    Result result = Result::success("Password reset for '" + admins_[index].username() + "'.");
    if (!library_.logActivity(Activity::PasswordChanged, "Password reset for '" + admins_[index].username() + "'")) {
        result.message += " (Warning: the transaction history could not be saved.)";
    }
    return result;
}
