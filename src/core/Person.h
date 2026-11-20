// Person.h - abstract base class for people stored in the system (Member and Admin).
#pragma once
#include <string>
#include <utility>

class Person {
public:
    Person(std::string fullName, std::string email)
        : fullName_(std::move(fullName)), email_(std::move(email)) {}
    Person(const Person&) = default;
    Person(Person&&) = default;
    Person& operator=(const Person&) = default;
    Person& operator=(Person&&) = default;
    virtual ~Person() = default;

    const std::string& fullName() const { return fullName_; }
    const std::string& email() const { return email_; }
    void setFullName(std::string value) { fullName_ = std::move(value); }
    void setEmail(std::string value) { email_ = std::move(value); }

    // Each kind of person describes itself differently (runtime polymorphism).
    virtual std::string summary() const = 0;

private:
    std::string fullName_;
    std::string email_;
};
