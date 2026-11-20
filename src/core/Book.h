// Book.h - a catalogue entry with copy tracking. Copy counts can only change through
// methods that keep 0 <= available <= total (encapsulation).
#pragma once
#include <optional>
#include <string>
#include <vector>

#include "Result.h"
#include "Serializable.h"

class Book : public Serializable {
public:
    Book(std::string id, std::string title, std::string author, std::string category, int year,
         int totalCopies, int availableCopies);

    const std::string& id() const { return id_; }
    const std::string& title() const { return title_; }
    const std::string& author() const { return author_; }
    const std::string& category() const { return category_; }
    int year() const { return year_; }
    int totalCopies() const { return totalCopies_; }
    int availableCopies() const { return availableCopies_; }
    int issuedCopies() const { return totalCopies_ - availableCopies_; }
    bool hasAvailableCopy() const { return availableCopies_ > 0; }

    void setTitle(std::string value) { title_ = std::move(value); }
    void setAuthor(std::string value) { author_ = std::move(value); }
    void setCategory(std::string value) { category_ = std::move(value); }
    void setYear(int value) { year_ = value; }

    bool checkOut();                         // one copy leaves the shelf
    bool checkIn();                          // one copy comes back
    Result setTotalCopies(int newTotal);     // refuses to go below the copies currently issued
    bool reconcile(int activeIssues);         // repairs counts after loading; true if anything changed

    std::string serialize() const override;
    static std::optional<Book> parse(const std::vector<std::string>& fields);

private:
    std::string id_;
    std::string title_;
    std::string author_;
    std::string category_;
    int year_;
    int totalCopies_;
    int availableCopies_;
};
