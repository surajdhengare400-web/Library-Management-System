// Serializable.h - interface implemented by every class that is stored in a data file.
// FileManager only depends on this interface (polymorphism): it does not need to know
// whether it is saving a Book, a Member, a IssueRecord, a Transaction or an Admin.
#pragma once
#include <string>

class Serializable {
public:
    virtual ~Serializable() = default;
    virtual std::string serialize() const = 0;   // one line of text, fields separated by '|'
};
