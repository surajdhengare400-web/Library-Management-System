// Member.h - a registered library member (is-a Person, can be saved to a file).
#pragma once
#include <optional>
#include <string>
#include <vector>

#include "Date.h"
#include "Person.h"
#include "Serializable.h"

class Member : public Person, public Serializable {
public:
    Member(std::string id, std::string fullName, std::string email, std::string phone, Date registrationDate);

    const std::string& id() const { return id_; }
    const std::string& phone() const { return phone_; }
    const Date& registrationDate() const { return registrationDate_; }
    void setPhone(std::string value) { phone_ = std::move(value); }

    std::string summary() const override;
    std::string serialize() const override;
    static std::optional<Member> parse(const std::vector<std::string>& fields);

private:
    std::string id_;
    std::string phone_;
    Date registrationDate_;
};
