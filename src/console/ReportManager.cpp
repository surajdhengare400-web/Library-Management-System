#include "ReportManager.h"

#include <algorithm>
#include <iostream>
#include <map>

#include "Console.h"
#include "Utils.h"

using ui::Table;

namespace {
std::string percent(int part, int whole) {
    if (whole <= 0) return "0.0%";
    const int tenths = static_cast<int>((static_cast<long long>(part) * 1000 + whole / 2) / whole);
    return std::to_string(tenths / 10) + "." + std::to_string(tenths % 10) + "%";
}

void printMetrics(const std::vector<std::pair<std::string, std::string>>& metrics) {
    Table table;
    table.column("Metric", 32).column("Value", 24, Table::Align::Right);
    for (const auto& m : metrics) table.addRow({m.first, m.second});
    table.print();
}
}  // namespace

ReportManager::ReportManager(const Library& library, const AdminManager& admins)
    : library_(library), admins_(admins) {}

void ReportManager::printBooks(const std::vector<Book>& books) const {
    Table table;
    table.column("Book ID", 12).column("Title", 28).column("Author", 20).column("Category", 16)
        .column("Year", 4, Table::Align::Right).column("Total", 5, Table::Align::Right)
        .column("Avail", 5, Table::Align::Right);
    for (const Book& b : books) {
        table.addRow({b.id(), b.title(), b.author(), b.category(), std::to_string(b.year()),
                      std::to_string(b.totalCopies()), std::to_string(b.availableCopies())});
    }
    table.print();
    std::cout << "  " << books.size() << " record(s).\n";
}

void ReportManager::printMembers(const std::vector<Member>& members) const {
    Table table;
    table.column("Member ID", 12).column("Full Name", 24).column("Email", 28).column("Phone", 16).column("Registered", 10);
    for (const Member& m : members) {
        table.addRow({m.id(), m.fullName(), m.email(), m.phone(), m.registrationDate().toString()});
    }
    table.print();
    std::cout << "  " << members.size() << " record(s).\n";
}

void ReportManager::printIssues(const std::vector<IssueRecord>& issues, const Date& asOf, bool showReturnDate) const {
    Table table;
    table.column("Issue", 5).column("Book", 22).column("Member", 18).column("Issued", 10).column("Due", 10);
    if (showReturnDate) table.column("Returned", 10);
    table.column("Status", 16).column("Fine", 10, Table::Align::Right);
    for (const IssueRecord& l : issues) {
        std::string status;
        std::string fine = "-";
        if (l.isReturned()) {
            const long long late = l.overdueDays(asOf);
            status = late > 0 ? "Returned late" : "Returned";
            if (l.fineAmount() > 0.0) fine = util::formatMoney(l.fineAmount()) + (l.isFinePaid() ? " paid" : " due");
        } else if (l.isOverdue(asOf)) {
            status = "Overdue " + std::to_string(l.overdueDays(asOf)) + "d";
            fine = util::formatMoney(library_.fineFor(l, asOf)) + " acc.";
        } else {
            const long long left = l.dueDate().daysAfter(asOf);
            status = left == 0 ? "Due today" : "Due in " + std::to_string(left) + "d";
        }
        std::vector<std::string> row = {util::formatId('I', l.id()),
                                        l.bookId() + " " + library_.bookTitle(l.bookId()),
                                        l.memberId() + " " + library_.memberName(l.memberId()),
                                        l.issueDate().toString(), l.dueDate().toString()};
        if (showReturnDate) row.push_back(l.isReturned() ? l.returnDate().toString() : "-");
        row.push_back(status);
        row.push_back(fine);
        table.addRow(std::move(row));
    }
    table.print();
    std::cout << "  " << issues.size() << " record(s).\n";
}

void ReportManager::printAdmins() const {
    Table table;
    table.column("Username", 20).column("Full Name", 22).column("Email", 26).column("Role", 11)
        .column("Status", 8).column("Created", 10);
    for (const Admin& a : admins_.admins()) {
        table.addRow({a.username(), a.fullName(), a.email(), roleToString(a.role()),
                      a.isActive() ? "Active" : "Inactive", a.createdAt().substr(0, 10)});
    }
    table.print();
}

void ReportManager::dashboard(const Date& asOf) const {
    ui::heading("DASHBOARD  -  " + asOf.toString());
    printMetrics({
        {"Total book titles", std::to_string(library_.books().size())},
        {"Total book copies", std::to_string(library_.totalCopies())},
        {"Available copies", std::to_string(library_.availableCopies())},
        {"Issued copies", std::to_string(library_.issuedCopies())},
        {"Registered members", std::to_string(library_.members().size())},
        {"Active issues", std::to_string(library_.activeIssues().size())},
        {"Overdue issues", std::to_string(library_.overdueIssues(asOf).size())},
        {"Total administrators", std::to_string(admins_.admins().size())},
        {"Outstanding fines", util::formatMoney(library_.outstandingFines(asOf))},
    });
}

void ReportManager::inventoryReport() const {
    ui::heading("REPORT: COMPLETE BOOK INVENTORY");
    if (library_.books().empty()) { ui::info("The catalogue is empty."); return; }
    printBooks(library_.books());
    std::cout << "  Total copies: " << library_.totalCopies() << "   Available: " << library_.availableCopies()
              << "   Issued: " << library_.issuedCopies() << '\n';
}

void ReportManager::memberReport() const {
    ui::heading("REPORT: MEMBER RECORDS");
    if (library_.members().empty()) { ui::info("No members are registered."); return; }
    printMembers(library_.members());
}

void ReportManager::issuedBooksReport(const Date& asOf) const {
    ui::heading("REPORT: CURRENTLY ISSUED BOOKS");
    const std::vector<IssueRecord> issues = library_.activeIssues();
    if (issues.empty()) { ui::info("No books are currently issued."); return; }
    printIssues(issues, asOf, false);
}

void ReportManager::returnedBooksReport(const Date& asOf) const {
    ui::heading("REPORT: RETURNED BOOKS");
    const std::vector<IssueRecord> issues = library_.completedIssues();
    if (issues.empty()) { ui::info("No books have been returned yet."); return; }
    printIssues(issues, asOf, true);
}

void ReportManager::overdueReport(const Date& asOf) const {
    ui::heading("REPORT: OVERDUE BOOKS  -  as of " + asOf.toString());
    const std::vector<IssueRecord> issues = library_.overdueIssues(asOf);
    if (issues.empty()) { ui::info("There are no overdue books."); return; }
    printIssues(issues, asOf, false);
    std::cout << "  Fine rate: " << util::formatMoney(library_.fineRatePerDay()) << " per overdue day.   Accruing: "
              << util::formatMoney(library_.accruingFines(asOf)) << '\n';
}

void ReportManager::fineReport(const Date& asOf) const {
    ui::heading("REPORT: FINE RECORDS  -  as of " + asOf.toString());
    Table table;
    table.column("Issue", 5).column("Member", 24).column("Book", 22).column("Days late", 9, Table::Align::Right)
        .column("Fine", 10, Table::Align::Right).column("Status", 10);
    std::size_t rows = 0;
    for (const IssueRecord& l : library_.issues()) {
        const long long late = l.overdueDays(asOf);
        const double fine = library_.fineFor(l, asOf);
        if (fine <= 0.0) continue;
        std::string status = l.isActive() ? "Accruing" : (l.isFinePaid() ? "Paid" : "Unpaid");
        table.addRow({util::formatId('I', l.id()), l.memberId() + " " + library_.memberName(l.memberId()),
                      l.bookId() + " " + library_.bookTitle(l.bookId()), std::to_string(late),
                      util::formatMoney(fine), status});
        ++rows;
    }
    if (rows == 0) { ui::info("No fines have been charged."); return; }
    table.print();
    std::cout << "  Collected: " << util::formatMoney(library_.collectedFines())
              << "   Unpaid: " << util::formatMoney(library_.unpaidRecordedFines())
              << "   Accruing: " << util::formatMoney(library_.accruingFines(asOf))
              << "   Outstanding: " << util::formatMoney(library_.outstandingFines(asOf)) << '\n';
}

void ReportManager::transactionReport(std::size_t lastCount) const {
    ui::heading(lastCount == 0 ? "REPORT: TRANSACTION HISTORY (ALL)"
                               : "REPORT: TRANSACTION HISTORY (LAST " + std::to_string(lastCount) + ")");
    const std::vector<Transaction>& all = library_.transactions();
    if (all.empty()) { ui::info("No activity has been recorded yet."); return; }
    Table table;
    table.column("ID", 6).column("Date & Time", 19).column("Activity", 18).column("Description", 62);
    const std::size_t start = (lastCount > 0 && all.size() > lastCount) ? all.size() - lastCount : 0;
    for (std::size_t i = start; i < all.size(); ++i) {
        table.addRow({util::formatId('T', all[i].id()), all[i].timestamp(), all[i].type(), all[i].description()});
    }
    table.print();
    std::cout << "  Showing " << (all.size() - start) << " of " << all.size() << " transaction(s).\n";
}

void ReportManager::statisticsReport(const Date& asOf) const {
    ui::heading("REPORT: OVERALL LIBRARY STATISTICS  -  " + asOf.toString());
    std::size_t superAdmins = 0;
    for (const Admin& a : admins_.admins()) if (a.isSuperAdmin()) ++superAdmins;
    printMetrics({
        {"Book titles", std::to_string(library_.books().size())},
        {"Total copies", std::to_string(library_.totalCopies())},
        {"Available copies", std::to_string(library_.availableCopies())},
        {"Issued copies", std::to_string(library_.issuedCopies())},
        {"Collection utilisation", percent(library_.issuedCopies(), library_.totalCopies())},
        {"Registered members", std::to_string(library_.members().size())},
        {"Active issues", std::to_string(library_.activeIssues().size())},
        {"Completed issues", std::to_string(library_.completedIssues().size())},
        {"Overdue issues", std::to_string(library_.overdueIssues(asOf).size())},
        {"Administrators (total / Super)", std::to_string(admins_.admins().size()) + " / " + std::to_string(superAdmins)},
        {"Fine rate per day", util::formatMoney(library_.fineRatePerDay())},
        {"Issue period", std::to_string(library_.issuePeriodDays()) + " days"},
        {"Fines collected", util::formatMoney(library_.collectedFines())},
        {"Fines unpaid (returned books)", util::formatMoney(library_.unpaidRecordedFines())},
        {"Fines accruing (overdue books)", util::formatMoney(library_.accruingFines(asOf))},
        {"Transactions recorded", std::to_string(library_.transactions().size())},
    });

    std::map<std::string, std::pair<int, int>> byCategory;    // category -> (titles, copies)
    for (const Book& b : library_.books()) {
        auto& entry = byCategory[b.category()];
        entry.first += 1;
        entry.second += b.totalCopies();
    }
    if (!byCategory.empty()) {
        Table table;
        table.column("Category", 28).column("Titles", 8, Table::Align::Right).column("Copies", 8, Table::Align::Right);
        for (const auto& entry : byCategory) {
            table.addRow({entry.first, std::to_string(entry.second.first), std::to_string(entry.second.second)});
        }
        std::cout << "\n  Books by category:";
        table.print();
    }
}
