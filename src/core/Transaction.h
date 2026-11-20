// Transaction.h - one entry of the audit trail (activity history).
#pragma once
#include <optional>
#include <string>
#include <vector>

#include "Serializable.h"

namespace Activity {
inline constexpr const char* BookAdded = "BOOK_ADDED";
inline constexpr const char* BookUpdated = "BOOK_UPDATED";
inline constexpr const char* BookDeleted = "BOOK_DELETED";
inline constexpr const char* MemberRegistered = "MEMBER_REGISTERED";
inline constexpr const char* MemberUpdated = "MEMBER_UPDATED";
inline constexpr const char* BookIssued = "BOOK_ISSUED";
inline constexpr const char* BookReturned = "BOOK_RETURNED";
inline constexpr const char* FinePaid = "FINE_PAID";
inline constexpr const char* AdminCreated = "ADMIN_CREATED";
inline constexpr const char* AdminActivated = "ADMIN_ACTIVATED";
inline constexpr const char* AdminDeactivated = "ADMIN_DEACTIVATED";
inline constexpr const char* PasswordChanged = "PASSWORD_CHANGED";
inline constexpr const char* ProfileUpdated = "PROFILE_UPDATED";
inline constexpr const char* SettingsChanged = "SETTINGS_CHANGED";
}  // namespace Activity

class Transaction : public Serializable {
public:
    Transaction(int id, std::string type, std::string description, std::string timestamp);

    int id() const { return id_; }
    const std::string& type() const { return type_; }
    const std::string& description() const { return description_; }
    const std::string& timestamp() const { return timestamp_; }

    std::string serialize() const override;
    static std::optional<Transaction> parse(const std::vector<std::string>& fields);

private:
    int id_;
    std::string type_;
    std::string description_;
    std::string timestamp_;
};
