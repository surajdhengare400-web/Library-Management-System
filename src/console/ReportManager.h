// ReportManager.h - builds and prints every table, report and the dashboard.
#pragma once
#include <vector>

#include "AdminManager.h"
#include "Date.h"
#include "Library.h"

class ReportManager {
public:
    ReportManager(const Library& library, const AdminManager& admins);

    // Generic table printers (also used by the search screens).
    void printBooks(const std::vector<Book>& books) const;
    void printMembers(const std::vector<Member>& members) const;
    void printIssues(const std::vector<IssueRecord>& issues, const Date& asOf, bool showReturnDate) const;
    void printAdmins() const;

    void dashboard(const Date& asOf) const;
    void inventoryReport() const;
    void memberReport() const;
    void issuedBooksReport(const Date& asOf) const;
    void returnedBooksReport(const Date& asOf) const;
    void overdueReport(const Date& asOf) const;
    void fineReport(const Date& asOf) const;
    void transactionReport(std::size_t lastCount = 0) const;       // 0 = all
    void statisticsReport(const Date& asOf) const;

private:
    const Library& library_;
    const AdminManager& admins_;
};
