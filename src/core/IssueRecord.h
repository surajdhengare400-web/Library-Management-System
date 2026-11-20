// IssueRecord.h - one issue/return record linking a book and a member (composition by ID).
#pragma once
#include <optional>
#include <string>
#include <vector>

#include "Date.h"
#include "Serializable.h"

class IssueRecord : public Serializable {
public:
    // New issue (not yet returned).
    IssueRecord(int id, std::string bookId, std::string memberId, Date issueDate, Date dueDate);
    // Full constructor used when loading from a file.
    IssueRecord(int id, std::string bookId, std::string memberId, Date issueDate, Date dueDate, bool returned,
         Date returnDate, double fine, bool finePaid);

    int id() const { return id_; }
    const std::string& bookId() const { return bookId_; }
    const std::string& memberId() const { return memberId_; }
    const Date& issueDate() const { return issueDate_; }
    const Date& dueDate() const { return dueDate_; }
    bool isReturned() const { return returned_; }
    bool isActive() const { return !returned_; }
    const Date& returnDate() const { return returnDate_; }
    double fineAmount() const { return fine_; }
    bool isFinePaid() const { return finePaid_; }
    bool hasUnpaidFine() const { return returned_ && fine_ > 0.0 && !finePaid_; }

    // Days late: measured to the return date for returned issues, otherwise to `asOf`.
    long long overdueDays(const Date& asOf) const;
    bool isOverdue(const Date& asOf) const { return isActive() && overdueDays(asOf) > 0; }

    void markReturned(const Date& returnDate, double fine);
    void markFinePaid() { finePaid_ = true; }

    std::string serialize() const override;
    static std::optional<IssueRecord> parse(const std::vector<std::string>& fields);

private:
    int id_;
    std::string bookId_;
    std::string memberId_;
    Date issueDate_;
    Date dueDate_;
    bool returned_;
    Date returnDate_;
    double fine_;
    bool finePaid_;
};
