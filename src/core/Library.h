// Library.h - the core domain model. Owns books, members, issues and the transaction log
// (composition) and saves every change to disk through FileManager.
#pragma once
#include <optional>
#include <string>
#include <vector>

#include "Book.h"
#include "Date.h"
#include "FileManager.h"
#include "IssueRecord.h"
#include "Member.h"
#include "Result.h"
#include "Transaction.h"

enum class BookField { Title, Author, Category };

class Library {
public:
    static constexpr int kMaxIssuesPerMember = 5;
    static constexpr int kMaxCopiesPerBook = 1000;

    explicit Library(const FileManager& files);

    // Loads all data files. Malformed records are skipped (with a warning and a backup copy).
    // Returns false only when a file exists but cannot be read (nothing is overwritten then).
    bool load(std::vector<std::string>& warnings, std::string& fatalError);

    // ---- activity log -------------------------------------------------------------------
    void setActor(const std::string& username) { actor_ = username; }
    bool logActivity(const std::string& type, const std::string& description);
    const std::vector<Transaction>& transactions() const { return transactions_; }

    // ---- settings -----------------------------------------------------------------------
    double fineRatePerDay() const { return fineRate_; }
    int issuePeriodDays() const { return issueDays_; }
    Result setFineRate(double ratePerDay);
    Result setIssuePeriod(int days);

    // ---- books --------------------------------------------------------------------------
    Result addBook(const std::string& id, const std::string& title, const std::string& author,
                   const std::string& category, int year, int copies);
    Result updateBook(const std::string& id, const std::string& title, const std::string& author,
                      const std::string& category, int year, int totalCopies);
    Result deleteBook(const std::string& id);
    std::optional<Book> findBook(const std::string& id) const;
    std::vector<Book> searchBooks(BookField field, const std::string& query) const;
    const std::vector<Book>& books() const { return books_; }

    // ---- members ------------------------------------------------------------------------
    Result registerMember(const std::string& id, const std::string& name, const std::string& email,
                          const std::string& phone, const Date& registeredOn);
    Result updateMember(const std::string& id, const std::string& name, const std::string& email,
                        const std::string& phone);
    std::optional<Member> findMember(const std::string& id) const;
    std::vector<Member> searchMembers(const std::string& query) const;
    const std::vector<Member>& members() const { return members_; }

    // ---- issues --------------------------------------------------------------------------
    Result issueBook(const std::string& bookId, const std::string& memberId, const Date& issueDate);
    Result returnBook(int issueId, const Date& returnDate);
    Result payFine(int issueId);
    std::optional<IssueRecord> findIssue(int issueId) const;
    const std::vector<IssueRecord>& issues() const { return issues_; }
    std::vector<IssueRecord> activeIssues() const;
    std::vector<IssueRecord> completedIssues() const;
    std::vector<IssueRecord> overdueIssues(const Date& asOf) const;
    std::vector<IssueRecord> unpaidFineIssues() const;
    std::vector<IssueRecord> issuesForMember(const std::string& memberId) const;
    int activeIssueCountForBook(const std::string& bookId) const;
    int activeIssueCountForMember(const std::string& memberId) const;

    // ---- fines and statistics -------------------------------------------------------------
    double calculateFine(long long overdueDays) const;
    double fineFor(const IssueRecord& issue, const Date& asOf) const;     // stored fine, or accrued fine if active
    double collectedFines() const;
    double unpaidRecordedFines() const;
    double accruingFines(const Date& asOf) const;
    double outstandingFines(const Date& asOf) const;               // unpaid + accruing
    int totalCopies() const;
    int availableCopies() const;
    int issuedCopies() const;

    std::string bookTitle(const std::string& bookId) const;        // "(deleted book)" if unknown
    std::string memberName(const std::string& memberId) const;

private:
    std::size_t bookIndex(const std::string& id) const;
    std::size_t memberIndex(const std::string& id) const;
    std::size_t issueIndex(int id) const;
    bool persistBooks(std::string& error) const;
    bool persistMembers(std::string& error) const;
    bool persistIssues(std::string& error) const;
    bool persistTransactions(std::string& error) const;
    bool persistSettings(std::string& error) const;
    Result finish(Result result, const std::string& type, const std::string& description);
    void loadSettings(std::vector<std::string>& warnings);

    const FileManager& files_;
    std::vector<Book> books_;
    std::vector<Member> members_;
    std::vector<IssueRecord> issues_;
    std::vector<Transaction> transactions_;
    double fineRate_;
    int issueDays_;
    int nextIssueId_;
    int nextTransactionId_;
    std::string actor_;
};
