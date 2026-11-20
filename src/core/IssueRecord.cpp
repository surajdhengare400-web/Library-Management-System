#include "IssueRecord.h"

#include <cstdio>

#include "FileManager.h"
#include "Utils.h"

IssueRecord::IssueRecord(int id, std::string bookId, std::string memberId, Date issueDate, Date dueDate)
    : id_(id),
      bookId_(std::move(bookId)),
      memberId_(std::move(memberId)),
      issueDate_(issueDate),
      dueDate_(dueDate),
      returned_(false),
      returnDate_(),
      fine_(0.0),
      finePaid_(false) {}

IssueRecord::IssueRecord(int id, std::string bookId, std::string memberId, Date issueDate, Date dueDate, bool returned,
           Date returnDate, double fine, bool finePaid)
    : id_(id),
      bookId_(std::move(bookId)),
      memberId_(std::move(memberId)),
      issueDate_(issueDate),
      dueDate_(dueDate),
      returned_(returned),
      returnDate_(returnDate),
      fine_(fine),
      finePaid_(finePaid) {}

long long IssueRecord::overdueDays(const Date& asOf) const {
    const Date& reference = returned_ ? returnDate_ : asOf;
    const long long late = reference.daysAfter(dueDate_);
    return late > 0 ? late : 0;
}

void IssueRecord::markReturned(const Date& returnDate, double fine) {
    returned_ = true;
    returnDate_ = returnDate;
    fine_ = fine;
    finePaid_ = (fine <= 0.0);
}

std::string IssueRecord::serialize() const {
    char fineText[32];
    std::snprintf(fineText, sizeof(fineText), "%.2f", fine_);
    return FileManager::join({std::to_string(id_), bookId_, memberId_, issueDate_.toString(), dueDate_.toString(),
                              returned_ ? returnDate_.toString() : "-", fineText, finePaid_ ? "1" : "0"});
}

std::optional<IssueRecord> IssueRecord::parse(const std::vector<std::string>& f) {
    if (f.size() != 8) return std::nullopt;
    long long id = 0;
    if (!util::parseLong(f[0], id) || id < 1 || id > 99999999) return std::nullopt;
    const std::string bookId = util::toUpper(util::trim(f[1]));
    const std::string memberId = util::toUpper(util::trim(f[2]));
    if (!validate::checkId(bookId).empty() || !validate::checkId(memberId).empty()) return std::nullopt;
    Date issued, due, returnDate;
    if (!Date::parse(f[3], issued) || !Date::parse(f[4], due) || due < issued) return std::nullopt;
    bool returned = false;
    if (f[5] != "-") {
        if (!Date::parse(f[5], returnDate) || returnDate < issued) return std::nullopt;
        returned = true;
    }
    double fine = 0.0;
    if (!util::parseDouble(f[6], fine) || fine < 0.0 || fine > 10000000.0) return std::nullopt;
    if (f[7] != "0" && f[7] != "1") return std::nullopt;
    const bool paid = (f[7] == "1");
    if (!returned && (fine != 0.0 || paid)) return std::nullopt;
    return IssueRecord(static_cast<int>(id), bookId, memberId, issued, due, returned, returnDate, fine, paid);
}
