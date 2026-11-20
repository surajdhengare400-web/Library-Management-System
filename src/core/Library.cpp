#include "Library.h"

#include <algorithm>
#include <cstdio>
#include <limits>

#include "Utils.h"

namespace {
const std::size_t kNotFound = std::numeric_limits<std::size_t>::max();
constexpr const char* kBooksFile = "books.txt";
constexpr const char* kMembersFile = "members.txt";
constexpr const char* kIssuesFile = "issue_records.txt";
constexpr const char* kTransactionsFile = "transactions.txt";
constexpr const char* kSettingsFile = "settings.txt";

std::string normalizeId(const std::string& id) { return util::toUpper(util::trim(id)); }
}  // namespace

Library::Library(const FileManager& files)
    : files_(files), fineRate_(5.0), issueDays_(14), nextIssueId_(1), nextTransactionId_(1) {}

// ------------------------------------------------------------------------------------------
// Loading and saving
// ------------------------------------------------------------------------------------------

bool Library::load(std::vector<std::string>& warnings, std::string& fatalError) {
    if (!files_.ensureDataDirectory(fatalError)) return false;
    books_.clear();
    members_.clear();
    issues_.clear();
    transactions_.clear();

    if (!files_.loadRecords<Book>(kBooksFile, books_, warnings, fatalError)) return false;
    if (!files_.loadRecords<Member>(kMembersFile, members_, warnings, fatalError)) return false;
    if (!files_.loadRecords<IssueRecord>(kIssuesFile, issues_, warnings, fatalError)) return false;
    if (!files_.loadRecords<Transaction>(kTransactionsFile, transactions_, warnings, fatalError)) return false;
    loadSettings(warnings);

    auto reportDuplicates = [&](const char* file, std::size_t dropped) {
        if (dropped == 0) return;
        std::string backup;
        const bool saved = files_.backupCorrupted(file, backup);
        warnings.push_back(std::string(file) + ": " + std::to_string(dropped) + " duplicate ID record(s) ignored. " +
                           (saved ? "Original copied to " + backup + "." : "Could not create a backup copy!"));
    };
    reportDuplicates(kBooksFile, util::dropDuplicates(books_, [](const Book& b) { return b.id(); }));
    reportDuplicates(kMembersFile, util::dropDuplicates(members_, [](const Member& m) { return m.id(); }));
    reportDuplicates(kIssuesFile, util::dropDuplicates(issues_, [](const IssueRecord& l) { return l.id(); }));
    reportDuplicates(kTransactionsFile,
                     util::dropDuplicates(transactions_, [](const Transaction& t) { return t.id(); }));

    // The issue list is the source of truth for how many copies are out: repair book counts if they disagree.
    for (Book& book : books_) {
        if (book.reconcile(activeIssueCountForBook(book.id()))) {
            warnings.push_back("Copy counts of book " + book.id() + " did not match the active issues and were corrected.");
        }
    }
    for (const IssueRecord& issue : issues_) {
        if (issue.isActive() && (bookIndex(issue.bookId()) == kNotFound || memberIndex(issue.memberId()) == kNotFound)) {
            warnings.push_back("Active issue " + util::formatId('I', issue.id()) +
                               " refers to a book or member that no longer exists.");
        }
    }

    nextIssueId_ = 1;
    for (const IssueRecord& issue : issues_) nextIssueId_ = std::max(nextIssueId_, issue.id() + 1);
    nextTransactionId_ = 1;
    for (const Transaction& t : transactions_) nextTransactionId_ = std::max(nextTransactionId_, t.id() + 1);
    return true;
}

void Library::loadSettings(std::vector<std::string>& warnings) {
    fineRate_ = 5.0;
    issueDays_ = 14;
    std::vector<std::string> lines;
    std::string error;
    const FileManager::ReadStatus status = files_.readLines(kSettingsFile, lines, error);
    if (status == FileManager::ReadStatus::Missing) return;
    if (status == FileManager::ReadStatus::Failed) {
        warnings.push_back(error + " Default settings are used.");
        return;
    }
    std::vector<std::string> fields;
    double rate = 0.0;
    long long days = 0;
    if (!lines.empty() && FileManager::split(lines[0], fields) && fields.size() == 2 &&
        util::parseDouble(fields[0], rate) && util::parseLong(fields[1], days) && rate >= 0.0 && rate <= 10000.0 &&
        days >= 1 && days <= 90) {
        fineRate_ = rate;
        issueDays_ = static_cast<int>(days);
    } else {
        std::string backup;
        files_.backupCorrupted(kSettingsFile, backup);
        warnings.push_back(std::string(kSettingsFile) + " is malformed; default settings are used.");
    }
}

bool Library::persistBooks(std::string& error) const { return files_.saveRecords(kBooksFile, books_, error); }
bool Library::persistMembers(std::string& error) const { return files_.saveRecords(kMembersFile, members_, error); }
bool Library::persistIssues(std::string& error) const { return files_.saveRecords(kIssuesFile, issues_, error); }
bool Library::persistTransactions(std::string& error) const {
    return files_.saveRecords(kTransactionsFile, transactions_, error);
}
bool Library::persistSettings(std::string& error) const {
    char rate[32];
    std::snprintf(rate, sizeof(rate), "%.2f", fineRate_);
    return files_.writeLinesAtomic(kSettingsFile, {FileManager::join({rate, std::to_string(issueDays_)})}, error);
}

bool Library::logActivity(const std::string& type, const std::string& description) {
    std::string text = description;
    if (!actor_.empty()) text += " [by " + actor_ + "]";
    transactions_.emplace_back(nextTransactionId_, type, text, util::currentTimestamp());
    std::string error;
    if (!persistTransactions(error)) {
        transactions_.pop_back();
        return false;
    }
    ++nextTransactionId_;
    return true;
}

Result Library::finish(Result result, const std::string& type, const std::string& description) {
    if (!logActivity(type, description)) {
        result.message += " (Warning: the transaction history could not be saved.)";
    }
    return result;
}

std::size_t Library::bookIndex(const std::string& id) const {
    const std::string key = normalizeId(id);
    for (std::size_t i = 0; i < books_.size(); ++i) {
        if (books_[i].id() == key) return i;
    }
    return kNotFound;
}

std::size_t Library::memberIndex(const std::string& id) const {
    const std::string key = normalizeId(id);
    for (std::size_t i = 0; i < members_.size(); ++i) {
        if (members_[i].id() == key) return i;
    }
    return kNotFound;
}

std::size_t Library::issueIndex(int id) const {
    for (std::size_t i = 0; i < issues_.size(); ++i) {
        if (issues_[i].id() == id) return i;
    }
    return kNotFound;
}

// ------------------------------------------------------------------------------------------
// Settings
// ------------------------------------------------------------------------------------------

Result Library::setFineRate(double ratePerDay) {
    if (ratePerDay < 0.0 || ratePerDay > 10000.0) return Result::failure("Fine rate must be between 0 and 10000.");
    const double oldRate = fineRate_;
    fineRate_ = util::roundMoney(ratePerDay);
    std::string error;
    if (!persistSettings(error)) {
        fineRate_ = oldRate;
        return Result::failure(error);
    }
    return finish(Result::success("Fine rate set to " + util::formatMoney(fineRate_) + " per overdue day."),
                  Activity::SettingsChanged,
                  "Fine rate changed from " + util::formatMoney(oldRate) + " to " + util::formatMoney(fineRate_) + " per day");
}

Result Library::setIssuePeriod(int days) {
    if (days < 1 || days > 90) return Result::failure("Issue period must be between 1 and 90 days.");
    const int oldDays = issueDays_;
    issueDays_ = days;
    std::string error;
    if (!persistSettings(error)) {
        issueDays_ = oldDays;
        return Result::failure(error);
    }
    return finish(Result::success("Issue period set to " + std::to_string(days) + " days."), Activity::SettingsChanged,
                  "Issue period changed from " + std::to_string(oldDays) + " to " + std::to_string(days) + " days");
}

// ------------------------------------------------------------------------------------------
// Books
// ------------------------------------------------------------------------------------------

Result Library::addBook(const std::string& rawId, const std::string& rawTitle, const std::string& rawAuthor,
                        const std::string& rawCategory, int year, int copies) {
    const std::string id = normalizeId(rawId);
    const std::string title = util::trim(rawTitle), author = util::trim(rawAuthor), category = util::trim(rawCategory);
    std::string err = validate::checkId(id);
    if (err.empty()) err = validate::checkText(title, "Title", 1, 120);
    if (err.empty()) err = validate::checkText(author, "Author", 1, 80);
    if (err.empty()) err = validate::checkText(category, "Category", 1, 40);
    if (err.empty()) err = validate::checkYear(year);
    if (!err.empty()) return Result::failure(err);
    if (copies < 1 || copies > kMaxCopiesPerBook) {
        return Result::failure("Number of copies must be between 1 and " + std::to_string(kMaxCopiesPerBook) + ".");
    }
    if (bookIndex(id) != kNotFound) return Result::failure("A book with ID '" + id + "' already exists.");

    std::vector<Book> backup = books_;
    books_.emplace_back(id, title, author, category, year, copies, copies);
    std::string saveError;
    if (!persistBooks(saveError)) {
        books_ = std::move(backup);
        return Result::failure(saveError);
    }
    const std::string copiesText = std::to_string(copies) + (copies == 1 ? " copy" : " copies");
    return finish(Result::success("Book '" + title + "' was added with " + copiesText + "."),
                  Activity::BookAdded, "Added book " + id + " \"" + title + "\" (" + std::to_string(copies) + " copies)");
}

Result Library::updateBook(const std::string& rawId, const std::string& rawTitle, const std::string& rawAuthor,
                           const std::string& rawCategory, int year, int totalCopies) {
    const std::size_t index = bookIndex(rawId);
    if (index == kNotFound) return Result::failure("No book found with ID '" + normalizeId(rawId) + "'.");
    const std::string title = util::trim(rawTitle), author = util::trim(rawAuthor), category = util::trim(rawCategory);
    std::string err = validate::checkText(title, "Title", 1, 120);
    if (err.empty()) err = validate::checkText(author, "Author", 1, 80);
    if (err.empty()) err = validate::checkText(category, "Category", 1, 40);
    if (err.empty()) err = validate::checkYear(year);
    if (!err.empty()) return Result::failure(err);
    if (totalCopies > kMaxCopiesPerBook) {
        return Result::failure("Number of copies must be between 1 and " + std::to_string(kMaxCopiesPerBook) + ".");
    }

    std::vector<Book> backup = books_;
    Book& book = books_[index];
    const Result copiesResult = book.setTotalCopies(totalCopies);
    if (!copiesResult.ok) {
        books_ = std::move(backup);
        return copiesResult;
    }
    book.setTitle(title);
    book.setAuthor(author);
    book.setCategory(category);
    book.setYear(year);
    std::string saveError;
    if (!persistBooks(saveError)) {
        books_ = std::move(backup);
        return Result::failure(saveError);
    }
    return finish(Result::success("Book " + book.id() + " was updated."), Activity::BookUpdated,
                  "Updated book " + book.id() + " \"" + title + "\" (" + std::to_string(book.totalCopies()) + " copies)");
}

Result Library::deleteBook(const std::string& rawId) {
    const std::size_t index = bookIndex(rawId);
    if (index == kNotFound) return Result::failure("No book found with ID '" + normalizeId(rawId) + "'.");
    const int issued = activeIssueCountForBook(books_[index].id());
    if (issued > 0 || books_[index].issuedCopies() > 0) {
        return Result::failure("This book cannot be deleted: " + std::to_string(std::max(issued, books_[index].issuedCopies())) +
                               " copy(ies) are currently issued.");
    }
    std::vector<Book> backup = books_;
    const std::string id = books_[index].id();
    const std::string title = books_[index].title();
    books_.erase(books_.begin() + static_cast<std::ptrdiff_t>(index));
    std::string saveError;
    if (!persistBooks(saveError)) {
        books_ = std::move(backup);
        return Result::failure(saveError);
    }
    return finish(Result::success("Book " + id + " was deleted."), Activity::BookDeleted,
                  "Deleted book " + id + " \"" + title + "\"");
}

std::optional<Book> Library::findBook(const std::string& id) const {
    const std::size_t index = bookIndex(id);
    if (index == kNotFound) return std::nullopt;
    return books_[index];
}

std::vector<Book> Library::searchBooks(BookField field, const std::string& query) const {
    std::vector<Book> found;
    const std::string q = util::trim(query);
    if (q.empty()) return found;
    for (const Book& book : books_) {
        const std::string& text = field == BookField::Title ? book.title()
                                  : field == BookField::Author ? book.author() : book.category();
        if (util::containsIgnoreCase(text, q)) found.push_back(book);
    }
    return found;
}

// ------------------------------------------------------------------------------------------
// Members
// ------------------------------------------------------------------------------------------

Result Library::registerMember(const std::string& rawId, const std::string& rawName, const std::string& rawEmail,
                               const std::string& rawPhone, const Date& registeredOn) {
    const std::string id = normalizeId(rawId);
    const std::string name = util::trim(rawName), email = util::trim(rawEmail);
    const std::string phone = validate::normalizePhone(rawPhone);
    std::string err = validate::checkId(id);
    if (err.empty()) err = validate::checkName(name);
    if (err.empty()) err = validate::checkEmail(email);
    if (err.empty()) err = validate::checkPhone(phone);
    if (!err.empty()) return Result::failure(err);
    if (memberIndex(id) != kNotFound) return Result::failure("A member with ID '" + id + "' already exists.");
    for (const Member& m : members_) {
        if (util::toLower(m.email()) == util::toLower(email)) {
            return Result::failure("The email '" + email + "' is already registered to member " + m.id() + ".");
        }
    }

    std::vector<Member> backup = members_;
    members_.emplace_back(id, name, email, phone, registeredOn);
    std::string saveError;
    if (!persistMembers(saveError)) {
        members_ = std::move(backup);
        return Result::failure(saveError);
    }
    return finish(Result::success("Member '" + name + "' registered with ID " + id + "."), Activity::MemberRegistered,
                  "Registered member " + id + " \"" + name + "\"");
}

Result Library::updateMember(const std::string& rawId, const std::string& rawName, const std::string& rawEmail,
                             const std::string& rawPhone) {
    const std::size_t index = memberIndex(rawId);
    if (index == kNotFound) return Result::failure("No member found with ID '" + normalizeId(rawId) + "'.");
    const std::string name = util::trim(rawName), email = util::trim(rawEmail);
    const std::string phone = validate::normalizePhone(rawPhone);
    std::string err = validate::checkName(name);
    if (err.empty()) err = validate::checkEmail(email);
    if (err.empty()) err = validate::checkPhone(phone);
    if (!err.empty()) return Result::failure(err);
    for (std::size_t i = 0; i < members_.size(); ++i) {
        if (i != index && util::toLower(members_[i].email()) == util::toLower(email)) {
            return Result::failure("The email '" + email + "' is already registered to member " + members_[i].id() + ".");
        }
    }

    std::vector<Member> backup = members_;
    members_[index].setFullName(name);
    members_[index].setEmail(email);
    members_[index].setPhone(phone);
    std::string saveError;
    if (!persistMembers(saveError)) {
        members_ = std::move(backup);
        return Result::failure(saveError);
    }
    return finish(Result::success("Member " + members_[index].id() + " was updated."), Activity::MemberUpdated,
                  "Updated member " + members_[index].id() + " \"" + name + "\"");
}

std::optional<Member> Library::findMember(const std::string& id) const {
    const std::size_t index = memberIndex(id);
    if (index == kNotFound) return std::nullopt;
    return members_[index];
}

std::vector<Member> Library::searchMembers(const std::string& query) const {
    std::vector<Member> found;
    const std::string q = util::trim(query);
    if (q.empty()) return found;
    for (const Member& m : members_) {
        if (util::containsIgnoreCase(m.id(), q) || util::containsIgnoreCase(m.fullName(), q) ||
            util::containsIgnoreCase(m.email(), q) || util::containsIgnoreCase(m.phone(), q)) {
            found.push_back(m);
        }
    }
    return found;
}

// ------------------------------------------------------------------------------------------
// Issues
// ------------------------------------------------------------------------------------------

Result Library::issueBook(const std::string& rawBookId, const std::string& rawMemberId, const Date& issueDate) {
    const std::size_t memberPos = memberIndex(rawMemberId);
    if (memberPos == kNotFound) return Result::failure("No member found with ID '" + normalizeId(rawMemberId) + "'.");
    const std::size_t bookPos = bookIndex(rawBookId);
    if (bookPos == kNotFound) return Result::failure("No book found with ID '" + normalizeId(rawBookId) + "'.");
    if (!issueDate.isValid()) return Result::failure("The issue date is invalid.");

    const Member& member = members_[memberPos];
    const Book& book = books_[bookPos];
    if (!book.hasAvailableCopy()) return Result::failure("No copies of '" + book.title() + "' are available right now.");
    for (const IssueRecord& issue : issues_) {
        if (issue.isActive() && issue.memberId() == member.id() && issue.bookId() == book.id()) {
            return Result::failure(member.fullName() + " already has '" + book.title() + "' on issue (" +
                                   util::formatId('I', issue.id()) + ").");
        }
    }
    if (activeIssueCountForMember(member.id()) >= kMaxIssuesPerMember) {
        return Result::failure(member.fullName() + " already has the maximum of " + std::to_string(kMaxIssuesPerMember) +
                               " active issues.");
    }

    std::vector<Book> backupBooks = books_;
    std::vector<IssueRecord> backupIssues = issues_;
    const Date due = issueDate.addDays(issueDays_);
    const int issueId = nextIssueId_;
    issues_.emplace_back(issueId, book.id(), member.id(), issueDate, due);
    books_[bookPos].checkOut();

    std::string error;
    if (!persistIssues(error) || !persistBooks(error)) {
        books_ = std::move(backupBooks);
        issues_ = std::move(backupIssues);
        return Result::failure(error);
    }
    ++nextIssueId_;
    const std::string issueLabel = util::formatId('I', issueId);
    return finish(Result::success("Book issued. Issue " + issueLabel + " - due on " + due.toString() + "."),
                  Activity::BookIssued,
                  "Issued " + books_[bookPos].id() + " \"" + books_[bookPos].title() + "\" to " + member.id() + " \"" +
                      member.fullName() + "\" (" + issueLabel + ", due " + due.toString() + ")");
}

Result Library::returnBook(int issueId, const Date& returnDate) {
    const std::size_t issuePos = issueIndex(issueId);
    if (issuePos == kNotFound) return Result::failure("No issue found with ID " + util::formatId('I', issueId) + ".");
    if (issues_[issuePos].isReturned()) return Result::failure("This issue was already returned.");
    if (!returnDate.isValid() || returnDate < issues_[issuePos].issueDate()) {
        return Result::failure("The return date cannot be before the issue date.");
    }

    std::vector<Book> backupBooks = books_;
    std::vector<IssueRecord> backupIssues = issues_;
    IssueRecord& issue = issues_[issuePos];
    const long long lateDays = std::max(0LL, returnDate.daysAfter(issue.dueDate()));
    const double fine = calculateFine(lateDays);
    issue.markReturned(returnDate, fine);
    const std::size_t bookPos = bookIndex(issue.bookId());
    if (bookPos != kNotFound) books_[bookPos].checkIn();

    std::string error;
    if (!persistIssues(error) || !persistBooks(error)) {
        books_ = std::move(backupBooks);
        issues_ = std::move(backupIssues);
        return Result::failure(error);
    }
    std::string message = "Book returned on " + returnDate.toString() + ".";
    if (lateDays > 0) {
        message += " It was " + std::to_string(lateDays) + " day(s) overdue - fine " + util::formatMoney(fine) + ".";
    } else {
        message += " Returned on time - no fine.";
    }
    return finish(Result::success(message), Activity::BookReturned,
                  "Returned " + issue.bookId() + " \"" + bookTitle(issue.bookId()) + "\" from " + issue.memberId() + " (" +
                      util::formatId('I', issue.id()) + ", " + std::to_string(lateDays) + " day(s) late, fine " +
                      util::formatMoney(fine) + ")");
}

Result Library::payFine(int issueId) {
    const std::size_t issuePos = issueIndex(issueId);
    if (issuePos == kNotFound) return Result::failure("No issue found with ID " + util::formatId('I', issueId) + ".");
    IssueRecord& issue = issues_[issuePos];
    if (!issue.isReturned()) return Result::failure("Fines can be paid only after the book has been returned.");
    if (issue.fineAmount() <= 0.0) return Result::failure("This issue has no fine.");
    if (issue.isFinePaid()) return Result::failure("The fine for this issue was already paid.");

    std::vector<IssueRecord> backup = issues_;
    issue.markFinePaid();
    std::string error;
    if (!persistIssues(error)) {
        issues_ = std::move(backup);
        return Result::failure(error);
    }
    return finish(Result::success("Fine of " + util::formatMoney(issue.fineAmount()) + " marked as paid."),
                  Activity::FinePaid,
                  "Fine " + util::formatMoney(issue.fineAmount()) + " paid for " + util::formatId('I', issue.id()) +
                      " (member " + issue.memberId() + ")");
}

std::optional<IssueRecord> Library::findIssue(int issueId) const {
    const std::size_t index = issueIndex(issueId);
    if (index == kNotFound) return std::nullopt;
    return issues_[index];
}

std::vector<IssueRecord> Library::activeIssues() const {
    std::vector<IssueRecord> out;
    for (const IssueRecord& l : issues_) if (l.isActive()) out.push_back(l);
    return out;
}

std::vector<IssueRecord> Library::completedIssues() const {
    std::vector<IssueRecord> out;
    for (const IssueRecord& l : issues_) if (l.isReturned()) out.push_back(l);
    return out;
}

std::vector<IssueRecord> Library::overdueIssues(const Date& asOf) const {
    std::vector<IssueRecord> out;
    for (const IssueRecord& l : issues_) if (l.isOverdue(asOf)) out.push_back(l);
    return out;
}

std::vector<IssueRecord> Library::unpaidFineIssues() const {
    std::vector<IssueRecord> out;
    for (const IssueRecord& l : issues_) if (l.hasUnpaidFine()) out.push_back(l);
    return out;
}

std::vector<IssueRecord> Library::issuesForMember(const std::string& memberId) const {
    std::vector<IssueRecord> out;
    const std::string key = normalizeId(memberId);
    for (const IssueRecord& l : issues_) if (l.memberId() == key) out.push_back(l);
    return out;
}

int Library::activeIssueCountForBook(const std::string& bookId) const {
    int count = 0;
    for (const IssueRecord& l : issues_) if (l.isActive() && l.bookId() == bookId) ++count;
    return count;
}

int Library::activeIssueCountForMember(const std::string& memberId) const {
    int count = 0;
    for (const IssueRecord& l : issues_) if (l.isActive() && l.memberId() == memberId) ++count;
    return count;
}

// ------------------------------------------------------------------------------------------
// Fines and statistics
// ------------------------------------------------------------------------------------------

double Library::calculateFine(long long overdueDays) const {
    return overdueDays <= 0 ? 0.0 : util::roundMoney(static_cast<double>(overdueDays) * fineRate_);
}

double Library::fineFor(const IssueRecord& issue, const Date& asOf) const {
    return issue.isReturned() ? issue.fineAmount() : calculateFine(issue.overdueDays(asOf));
}

double Library::collectedFines() const {
    double total = 0.0;
    for (const IssueRecord& l : issues_) if (l.isReturned() && l.isFinePaid()) total += l.fineAmount();
    return util::roundMoney(total);
}

double Library::unpaidRecordedFines() const {
    double total = 0.0;
    for (const IssueRecord& l : issues_) if (l.hasUnpaidFine()) total += l.fineAmount();
    return util::roundMoney(total);
}

double Library::accruingFines(const Date& asOf) const {
    double total = 0.0;
    for (const IssueRecord& l : issues_) if (l.isOverdue(asOf)) total += calculateFine(l.overdueDays(asOf));
    return util::roundMoney(total);
}

double Library::outstandingFines(const Date& asOf) const {
    return util::roundMoney(unpaidRecordedFines() + accruingFines(asOf));
}

int Library::totalCopies() const {
    int total = 0;
    for (const Book& b : books_) total += b.totalCopies();
    return total;
}

int Library::availableCopies() const {
    int total = 0;
    for (const Book& b : books_) total += b.availableCopies();
    return total;
}

int Library::issuedCopies() const { return totalCopies() - availableCopies(); }

std::string Library::bookTitle(const std::string& bookId) const {
    const std::size_t index = bookIndex(bookId);
    return index == kNotFound ? "(deleted book)" : books_[index].title();
}

std::string Library::memberName(const std::string& memberId) const {
    const std::size_t index = memberIndex(memberId);
    return index == kNotFound ? "(unknown member)" : members_[index].fullName();
}
