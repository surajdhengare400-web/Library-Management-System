#include "Book.h"

#include <algorithm>

#include "FileManager.h"
#include "Utils.h"

Book::Book(std::string id, std::string title, std::string author, std::string category, int year,
           int totalCopies, int availableCopies)
    : id_(std::move(id)),
      title_(std::move(title)),
      author_(std::move(author)),
      category_(std::move(category)),
      year_(year),
      totalCopies_(std::max(0, totalCopies)),
      availableCopies_(0) {
    availableCopies_ = std::min(std::max(0, availableCopies), totalCopies_);
}

bool Book::checkOut() {
    if (availableCopies_ <= 0) return false;
    --availableCopies_;
    return true;
}

bool Book::checkIn() {
    if (availableCopies_ >= totalCopies_) return false;
    ++availableCopies_;
    return true;
}

Result Book::setTotalCopies(int newTotal) {
    if (newTotal < 1) return Result::failure("Total copies must be at least 1.");
    if (newTotal < issuedCopies()) {
        return Result::failure("Total copies cannot be less than the " + std::to_string(issuedCopies()) +
                               " copies currently issued.");
    }
    const int issued = issuedCopies();
    totalCopies_ = newTotal;
    availableCopies_ = newTotal - issued;
    return Result::success();
}

bool Book::reconcile(int activeIssues) {
    const int total = std::max(totalCopies_, activeIssues);
    const int available = total - activeIssues;
    const bool changed = (total != totalCopies_) || (available != availableCopies_);
    totalCopies_ = total;
    availableCopies_ = available;
    return changed;
}

std::string Book::serialize() const {
    return FileManager::join({id_, title_, author_, category_, std::to_string(year_),
                              std::to_string(totalCopies_), std::to_string(availableCopies_)});
}

std::optional<Book> Book::parse(const std::vector<std::string>& f) {
    if (f.size() != 7) return std::nullopt;
    const std::string id = util::toUpper(util::trim(f[0]));
    if (!validate::checkId(id).empty()) return std::nullopt;
    if (!validate::checkText(f[1], "Title", 1, 120).empty()) return std::nullopt;
    if (!validate::checkText(f[2], "Author", 1, 80).empty()) return std::nullopt;
    if (!validate::checkText(f[3], "Category", 1, 40).empty()) return std::nullopt;
    long long year = 0, total = 0, available = 0;
    if (!util::parseLong(f[4], year) || !util::parseLong(f[5], total) || !util::parseLong(f[6], available)) {
        return std::nullopt;
    }
    if (!validate::checkYear(year).empty()) return std::nullopt;
    if (total < 0 || total > 100000 || available < 0 || available > total) return std::nullopt;
    return Book(id, util::trim(f[1]), util::trim(f[2]), util::trim(f[3]), static_cast<int>(year),
                static_cast<int>(total), static_cast<int>(available));
}
