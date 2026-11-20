#include "App.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

#include "Console.h"
#include "Utils.h"

namespace {
constexpr int kMaxLoginAttempts = 3;

std::string idFormat(const std::string& value) { return validate::checkId(util::toUpper(value)); }
std::string nameFormat(const std::string& value) { return validate::checkName(value); }
std::string emailFormat(const std::string& value) { return validate::checkEmail(value); }
std::string phoneFormat(const std::string& value) { return validate::checkPhone(value); }
std::string usernameFormat(const std::string& value) { return validate::checkUsername(util::toLower(value)); }

std::string yearFormat(const std::string& value) {
    long long year = 0;
    if (!util::parseLong(value, year)) return "Please enter a valid year (for example 2019).";
    return validate::checkYear(year);
}

int toYear(const std::string& value) {
    long long year = 0;
    util::parseLong(value, year);
    return static_cast<int>(year);
}

ui::Validator textRule(const std::string& label, std::size_t minLen, std::size_t maxLen) {
    return [label, minLen, maxLen](const std::string& value) { return validate::checkText(value, label, minLen, maxLen); };
}

ui::Validator searchTerm() {
    return [](const std::string& value) -> std::string {
        return value.empty() ? "Please enter a search term." : validate::checkText(value, "Search term", 1, 60);
    };
}
}  // namespace

App::App() : files_("Data"), library_(files_), admins_(files_, library_), reports_(library_, admins_) {}

// ------------------------------------------------------------------------------------------
// Start-up, login and main menu
// ------------------------------------------------------------------------------------------

int App::run() {
    ui::init();
    try {
        ui::banner("LIBRARY MANAGEMENT SYSTEM  -  DEMO 3");
        if (!startup()) return 1;

        while (true) {
            if (!admins_.hasAdmins()) {
                const int choice = ui::menu("WELCOME - FIRST-TIME SETUP", {"Create the Super Admin account"}, "Exit");
                if (choice == 0) break;
                guarded([this] { firstTimeSetup(); });
                continue;
            }
            const int choice = ui::menu("ADMINISTRATOR LOGIN", {"Login"}, "Exit");
            if (choice == 0) break;
            if (loginFlow()) mainMenu();
        }
        ui::info("Thank you for using the Library Management System. Goodbye!");
        return 0;
    } catch (const ui::InputClosed&) {
        std::cout << "\n\n  Input closed. Exiting the Library Management System.\n";
        return 0;
    }
}

bool App::startup() {
    std::vector<std::string> warnings;
    std::string fatal;
    if (!library_.load(warnings, fatal) || !admins_.load(warnings, fatal)) {
        ui::error(fatal);
        ui::error("The program stopped to protect your existing data. Fix the problem above and start again.");
        return false;
    }
    std::cout << "\n  Data folder: " << files_.dataDirectory() << "/   Books: " << library_.books().size()
              << "   Members: " << library_.members().size() << "   Issues: " << library_.issues().size() << '\n';
    for (const std::string& w : warnings) ui::warning(w);
    return true;
}

void App::guarded(const std::function<void()>& action) {
    try {
        action();
    } catch (const ui::CancelOperation&) {
        ui::info("Operation cancelled.");
    }
    ui::pause();
}

std::string App::promptNewPassword(const std::string& username) {
    ui::info("Password rules: 8-64 characters with an uppercase letter, a lowercase letter, a digit and a special character; no spaces.");
    while (true) {
        const std::string password = ui::askPassword("New password");
        const std::string problem = validate::checkPassword(password, username);
        if (!problem.empty()) {
            ui::error(problem);
            continue;
        }
        if (ui::askPassword("Confirm new password") != password) {
            ui::error("The passwords do not match. Please try again.");
            continue;
        }
        return password;
    }
}

void App::firstTimeSetup() {
    ui::heading("FIRST-TIME SETUP: CREATE SUPER ADMIN");
    ui::info("No administrator exists yet. This account will have full control. Type /c at any prompt to cancel.");
    const std::string username = util::toLower(ui::ask("Username (3-20 letters, digits, _)", usernameFormat));
    const std::string name = ui::ask("Full name", nameFormat);
    const std::string email = ui::ask("Email", emailFormat);
    const std::string password = promptNewPassword(username);
    ui::showResult(admins_.createFirstSuperAdmin(username, name, email, password));
}

bool App::loginFlow() {
    try {
        ui::heading("LOGIN");
        ui::info("Type /c at any prompt to go back.");
        for (int attempt = 1; attempt <= kMaxLoginAttempts; ++attempt) {
            const std::string username = ui::readLine("  Username: ");
            const std::string password = ui::askPassword("Password");
            const Result result = admins_.login(username, password);
            if (result.ok) {
                ui::success(result.message);
                return true;
            }
            ui::error(result.message + " (attempt " + std::to_string(attempt) + " of " +
                      std::to_string(kMaxLoginAttempts) + ")");
        }
        ui::error("Too many failed attempts. Please wait a moment.");
        std::this_thread::sleep_for(std::chrono::seconds(3));
    } catch (const ui::CancelOperation&) {
        ui::info("Login cancelled.");
    }
    return false;
}

void App::mainMenu() {
    while (admins_.isLoggedIn()) {
        const auto admin = admins_.currentAdmin();
        if (admin) {
            std::cout << "\n  Logged in as: " << admin->fullName() << " (" << admin->username() << ", "
                      << roleToString(admin->role()) << ")   Date: " << Date::today().toString() << '\n';
        }
        const int choice = ui::menu("MAIN MENU",
                                    {"Dashboard", "Book Management", "Member Management", "Issue & Return",
                                     "Reports", "Transaction History", "My Account", "Administration (Super Admin)"},
                                    "Logout");
        switch (choice) {
            case 1: reports_.dashboard(Date::today()); ui::pause(); break;
            case 2: bookMenu(); break;
            case 3: memberMenu(); break;
            case 4: issueReturnMenu(); break;
            case 5: reportsMenu(); break;
            case 6: historyMenu(); break;
            case 7: accountMenu(); break;
            case 8: adminMenu(); break;
            case 0:
                if (ui::confirm("Do you really want to log out?")) {
                    admins_.logout();
                    ui::success("You have been logged out.");
                }
                break;
            default: break;
        }
    }
}

ui::Validator App::existingBookId() const {
    return [this](const std::string& value) -> std::string {
        const std::string problem = idFormat(value);
        if (!problem.empty()) return problem;
        return library_.findBook(value) ? std::string() : "No book found with ID '" + util::toUpper(value) + "'.";
    };
}

ui::Validator App::existingMemberId() const {
    return [this](const std::string& value) -> std::string {
        const std::string problem = idFormat(value);
        if (!problem.empty()) return problem;
        return library_.findMember(value) ? std::string() : "No member found with ID '" + util::toUpper(value) + "'.";
    };
}

bool App::adminExists(const std::string& username) const {
    const std::string key = util::toLower(username);
    for (const Admin& a : admins_.admins()) {
        if (a.username() == key) return true;
    }
    return false;
}

// ------------------------------------------------------------------------------------------
// Book management
// ------------------------------------------------------------------------------------------

void App::bookMenu() {
    while (true) {
        const int choice = ui::menu("BOOK MANAGEMENT",
                                    {"Add Book", "Display All Books", "Search by Book ID", "Search by Title",
                                     "Search by Author", "Search by Category", "Update Book", "Delete Book"},
                                    "Back to Main Menu");
        switch (choice) {
            case 1: guarded([this] { addBook(); }); break;
            case 2: guarded([this] {
                ui::heading("ALL BOOKS");
                if (library_.books().empty()) ui::info("The catalogue is empty.");
                else reports_.printBooks(library_.books());
            }); break;
            case 3: guarded([this] { searchBookById(); }); break;
            case 4: guarded([this] { searchBooksBy(BookField::Title, "title"); }); break;
            case 5: guarded([this] { searchBooksBy(BookField::Author, "author"); }); break;
            case 6: guarded([this] { searchBooksBy(BookField::Category, "category"); }); break;
            case 7: guarded([this] { updateBook(); }); break;
            case 8: guarded([this] { deleteBook(); }); break;
            default: return;
        }
    }
}

void App::addBook() {
    ui::heading("ADD NEW BOOK");
    ui::info("Type /c at any prompt to cancel.");
    const std::string id = util::toUpper(ui::ask("Book ID (3-12 letters, digits, -)", [this](const std::string& v) -> std::string {
        const std::string problem = idFormat(v);
        if (!problem.empty()) return problem;
        return library_.findBook(v) ? "A book with this ID already exists." : std::string();
    }));
    const std::string title = ui::ask("Title", textRule("Title", 1, 120));
    const std::string author = ui::ask("Author", textRule("Author", 1, 80));
    const std::string category = ui::ask("Category", textRule("Category", 1, 40));
    const std::string year = ui::ask("Publication year", yearFormat);
    const int copies = ui::askInt("Number of copies (1-" + std::to_string(Library::kMaxCopiesPerBook) + ")", 1,
                                  Library::kMaxCopiesPerBook);
    ui::showResult(library_.addBook(id, title, author, category, toYear(year), copies));
}

void App::searchBookById() {
    ui::heading("SEARCH BOOK BY ID");
    const std::string id = ui::ask("Book ID", idFormat);
    const auto book = library_.findBook(id);
    if (!book) {
        ui::error("No book found with ID '" + util::toUpper(id) + "'.");
        return;
    }
    reports_.printBooks({*book});
}

void App::searchBooksBy(BookField field, const std::string& label) {
    ui::heading("SEARCH BOOKS BY " + util::toUpper(label));
    const std::string query = ui::ask("Enter part of the " + label, searchTerm());
    const std::vector<Book> found = library_.searchBooks(field, query);
    if (found.empty()) {
        ui::info("No books matched '" + query + "'.");
        return;
    }
    reports_.printBooks(found);
}

void App::updateBook() {
    ui::heading("UPDATE BOOK");
    ui::info("Type /c at any prompt to cancel. Press Enter to keep the current value.");
    const std::string id = ui::ask("Book ID to update", existingBookId());
    const Book book = *library_.findBook(id);
    reports_.printBooks({book});
    const std::string title = ui::askKeep("Title", book.title(), textRule("Title", 1, 120));
    const std::string author = ui::askKeep("Author", book.author(), textRule("Author", 1, 80));
    const std::string category = ui::askKeep("Category", book.category(), textRule("Category", 1, 40));
    const std::string year = ui::askKeep("Publication year", std::to_string(book.year()), yearFormat);
    if (book.issuedCopies() > 0) {
        ui::info(std::to_string(book.issuedCopies()) + " copy(ies) are currently issued, so total copies cannot go below that.");
    }
    const int copies = ui::askIntKeep("Total copies", std::max(1, book.issuedCopies()), Library::kMaxCopiesPerBook,
                                      book.totalCopies());
    if (!ui::confirm("Save these changes?")) {
        ui::info("No changes were saved.");
        return;
    }
    ui::showResult(library_.updateBook(book.id(), title, author, category, toYear(year), copies));
}

void App::deleteBook() {
    ui::heading("DELETE BOOK");
    const std::string id = ui::ask("Book ID to delete", existingBookId());
    const Book book = *library_.findBook(id);
    reports_.printBooks({book});
    if (book.issuedCopies() > 0) {
        ui::error("This book cannot be deleted while " + std::to_string(book.issuedCopies()) + " copy(ies) are issued.");
        return;
    }
    ui::warning("Deleting a book is permanent.");
    if (!ui::confirm("Delete '" + book.title() + "'?")) {
        ui::info("Deletion cancelled.");
        return;
    }
    ui::showResult(library_.deleteBook(book.id()));
}

// ------------------------------------------------------------------------------------------
// Member management
// ------------------------------------------------------------------------------------------

void App::memberMenu() {
    while (true) {
        const int choice = ui::menu("MEMBER MANAGEMENT",
                                    {"Register Member", "Display All Members", "Search Member",
                                     "Update Member Details", "Member Issue History"},
                                    "Back to Main Menu");
        switch (choice) {
            case 1: guarded([this] { registerMember(); }); break;
            case 2: guarded([this] {
                ui::heading("ALL MEMBERS");
                if (library_.members().empty()) ui::info("No members are registered.");
                else reports_.printMembers(library_.members());
            }); break;
            case 3: guarded([this] { searchMembers(); }); break;
            case 4: guarded([this] { updateMember(); }); break;
            case 5: guarded([this] { memberIssueHistory(); }); break;
            default: return;
        }
    }
}

void App::registerMember() {
    ui::heading("REGISTER MEMBER");
    ui::info("Type /c at any prompt to cancel.");
    const std::string id = util::toUpper(ui::ask("Member ID (3-12 letters, digits, -)", [this](const std::string& v) -> std::string {
        const std::string problem = idFormat(v);
        if (!problem.empty()) return problem;
        return library_.findMember(v) ? "A member with this ID already exists." : std::string();
    }));
    const std::string name = ui::ask("Full name", nameFormat);
    const std::string email = ui::ask("Email", emailFormat);
    const std::string phone = ui::ask("Phone number", phoneFormat);
    ui::showResult(library_.registerMember(id, name, email, phone, Date::today()));
}

void App::searchMembers() {
    ui::heading("SEARCH MEMBER");
    const std::string query = ui::ask("Enter ID, name, email or phone (or part of it)", searchTerm());
    const std::vector<Member> found = library_.searchMembers(query);
    if (found.empty()) {
        ui::info("No members matched '" + query + "'.");
        return;
    }
    reports_.printMembers(found);
}

void App::updateMember() {
    ui::heading("UPDATE MEMBER");
    ui::info("Type /c at any prompt to cancel. Press Enter to keep the current value.");
    const std::string id = ui::ask("Member ID to update", existingMemberId());
    const Member member = *library_.findMember(id);
    reports_.printMembers({member});
    const std::string name = ui::askKeep("Full name", member.fullName(), nameFormat);
    const std::string email = ui::askKeep("Email", member.email(), emailFormat);
    const std::string phone = ui::askKeep("Phone number", member.phone(), phoneFormat);
    if (!ui::confirm("Save these changes?")) {
        ui::info("No changes were saved.");
        return;
    }
    ui::showResult(library_.updateMember(member.id(), name, email, phone));
}

void App::memberIssueHistory() {
    ui::heading("MEMBER ISSUE HISTORY");
    const std::string id = ui::ask("Member ID", existingMemberId());
    const std::vector<IssueRecord> issues = library_.issuesForMember(id);
    std::cout << "\n  Member: " << library_.findMember(id)->summary() << '\n';
    if (issues.empty()) {
        ui::info("This member has no issue records.");
        return;
    }
    reports_.printIssues(issues, Date::today(), true);
}

// ------------------------------------------------------------------------------------------
// Issue and return
// ------------------------------------------------------------------------------------------

void App::issueReturnMenu() {
    while (true) {
        const int choice = ui::menu("ISSUE & RETURN",
                                    {"Issue Book", "Return Book", "Pay Fine", "Active Issues", "Completed Issues",
                                     "Overdue Issues"},
                                    "Back to Main Menu");
        switch (choice) {
            case 1: guarded([this] { issueBook(); }); break;
            case 2: guarded([this] { returnBook(); }); break;
            case 3: guarded([this] { payFine(); }); break;
            case 4: guarded([this] { reports_.issuedBooksReport(Date::today()); }); break;
            case 5: guarded([this] { reports_.returnedBooksReport(Date::today()); }); break;
            case 6: guarded([this] { reports_.overdueReport(Date::today()); }); break;
            default: return;
        }
    }
}

void App::issueBook() {
    ui::heading("ISSUE BOOK");
    ui::info("Type /c at any prompt to cancel. Issue period: " + std::to_string(library_.issuePeriodDays()) + " days.");
    const std::string memberId = ui::ask("Member ID", existingMemberId());
    std::cout << "  Member: " << library_.findMember(memberId)->summary() << '\n';
    const std::string bookId = ui::ask("Book ID", existingBookId());
    const Book book = *library_.findBook(bookId);
    std::cout << "  Book:   " << book.title() << " (" << book.availableCopies() << " of " << book.totalCopies()
              << " copies available)\n";
    ui::showResult(library_.issueBook(bookId, memberId, Date::today()));
}

void App::returnBook() {
    ui::heading("RETURN BOOK");
    const std::vector<IssueRecord> active = library_.activeIssues();
    if (active.empty()) {
        ui::info("There are no active issues.");
        return;
    }
    reports_.printIssues(active, Date::today(), false);
    ui::info("Type /c at any prompt to cancel. Enter the issue number, for example 1 for I0001.");
    const int issueId = std::stoi(ui::ask("Issue number to return", [this](const std::string& v) -> std::string {
        long long id = 0;
        if (!util::parseLong(v, id) || id < 1 || id > 99999999) return "Please enter a valid issue number.";
        const auto issue = library_.findIssue(static_cast<int>(id));
        if (!issue) return "No issue found with that number.";
        if (issue->isReturned()) return "That issue was already returned.";
        return std::string();
    }));
    const Result result = library_.returnBook(issueId, Date::today());
    ui::showResult(result);
    if (!result.ok) return;
    const auto issue = library_.findIssue(issueId);
    if (issue && issue->hasUnpaidFine() && ui::confirm("Collect the fine of " + util::formatMoney(issue->fineAmount()) + " now?")) {
        ui::showResult(library_.payFine(issueId));
    } else if (issue && issue->hasUnpaidFine()) {
        ui::info("The fine stays outstanding. Use 'Pay Fine' later to record the payment.");
    }
}

void App::payFine() {
    ui::heading("PAY FINE");
    const std::vector<IssueRecord> unpaid = library_.unpaidFineIssues();
    if (unpaid.empty()) {
        ui::info("There are no unpaid fines on returned books.");
        return;
    }
    reports_.printIssues(unpaid, Date::today(), true);
    const int issueId = std::stoi(ui::ask("Issue number whose fine was paid", [this](const std::string& v) -> std::string {
        long long id = 0;
        if (!util::parseLong(v, id) || id < 1 || id > 99999999) return "Please enter a valid issue number.";
        const auto issue = library_.findIssue(static_cast<int>(id));
        if (!issue || !issue->hasUnpaidFine()) return "That issue has no unpaid fine.";
        return std::string();
    }));
    const auto issue = *library_.findIssue(issueId);
    if (!ui::confirm("Record payment of " + util::formatMoney(issue.fineAmount()) + "?")) {
        ui::info("No payment was recorded.");
        return;
    }
    ui::showResult(library_.payFine(issueId));
}

// ------------------------------------------------------------------------------------------
// Reports and history
// ------------------------------------------------------------------------------------------

void App::reportsMenu() {
    while (true) {
        const int choice = ui::menu("REPORTS",
                                    {"Complete Book Inventory", "Member Records", "Currently Issued Books",
                                     "Returned Books", "Overdue Books", "Fine Records", "Transaction History",
                                     "Overall Library Statistics"},
                                    "Back to Main Menu");
        const Date today = Date::today();
        switch (choice) {
            case 1: reports_.inventoryReport(); break;
            case 2: reports_.memberReport(); break;
            case 3: reports_.issuedBooksReport(today); break;
            case 4: reports_.returnedBooksReport(today); break;
            case 5: reports_.overdueReport(today); break;
            case 6: reports_.fineReport(today); break;
            case 7: reports_.transactionReport(0); break;
            case 8: reports_.statisticsReport(today); break;
            default: return;
        }
        ui::pause();
    }
}

void App::historyMenu() {
    while (true) {
        const int choice = ui::menu("TRANSACTION HISTORY", {"Show last 20 transactions", "Show all transactions"},
                                    "Back to Main Menu");
        if (choice == 0) return;
        reports_.transactionReport(choice == 1 ? 20 : 0);
        ui::pause();
    }
}

// ------------------------------------------------------------------------------------------
// Own account
// ------------------------------------------------------------------------------------------

void App::accountMenu() {
    while (true) {
        const int choice = ui::menu("MY ACCOUNT", {"View Profile", "Change Password", "Update Profile"},
                                    "Back to Main Menu");
        switch (choice) {
            case 1: guarded([this] { showProfile(); }); break;
            case 2: guarded([this] { changePassword(); }); break;
            case 3: guarded([this] { updateProfile(); }); break;
            default: return;
        }
    }
}

void App::showProfile() {
    ui::heading("MY PROFILE");
    const auto admin = admins_.currentAdmin();
    if (!admin) {
        ui::error("You are not logged in.");
        return;
    }
    ui::Table table;
    table.column("Field", 12).column("Value", 40);
    table.addRow({"Username", admin->username()});
    table.addRow({"Full name", admin->fullName()});
    table.addRow({"Email", admin->email()});
    table.addRow({"Role", roleToString(admin->role())});
    table.addRow({"Status", admin->isActive() ? "Active" : "Inactive"});
    table.addRow({"Created", admin->createdAt()});
    table.print();
}

void App::changePassword() {
    ui::heading("CHANGE PASSWORD");
    ui::info("Type /c at any prompt to cancel.");
    const std::string current = ui::askPassword("Current password");
    const std::string next = promptNewPassword(admins_.currentUsername());
    ui::showResult(admins_.changeOwnPassword(current, next));
}

void App::updateProfile() {
    ui::heading("UPDATE PROFILE");
    const auto admin = admins_.currentAdmin();
    if (!admin) {
        ui::error("You are not logged in.");
        return;
    }
    ui::info("Type /c at any prompt to cancel. Press Enter to keep the current value.");
    const std::string name = ui::askKeep("Full name", admin->fullName(), nameFormat);
    const std::string email = ui::askKeep("Email", admin->email(), emailFormat);
    ui::showResult(admins_.updateOwnProfile(name, email));
}

// ------------------------------------------------------------------------------------------
// Administration (Super Admin only)
// ------------------------------------------------------------------------------------------

void App::adminMenu() {
    if (!admins_.isSuperAdmin()) {
        ui::error("Access denied: only a Super Admin can manage administrators and library settings.");
        ui::pause();
        return;
    }
    while (true) {
        const int choice = ui::menu("ADMINISTRATION",
                                    {"List Administrators", "Create Administrator", "Activate / Deactivate Administrator",
                                     "Reset Administrator Password", "Library Settings (fine rate, issue period)"},
                                    "Back to Main Menu");
        switch (choice) {
            case 1: guarded([this] { ui::heading("ADMINISTRATORS"); reports_.printAdmins(); }); break;
            case 2: guarded([this] { createAdmin(); }); break;
            case 3: guarded([this] { toggleAdmin(); }); break;
            case 4: guarded([this] { resetAdminPassword(); }); break;
            case 5: guarded([this] { librarySettings(); }); break;
            default: return;
        }
    }
}

void App::createAdmin() {
    ui::heading("CREATE ADMINISTRATOR");
    ui::info("Type /c at any prompt to cancel.");
    const std::string username = util::toLower(ui::ask("Username (3-20 letters, digits, _)", [this](const std::string& v) -> std::string {
        const std::string problem = usernameFormat(v);
        if (!problem.empty()) return problem;
        return adminExists(v) ? "That username is already taken." : std::string();
    }));
    const std::string name = ui::ask("Full name", nameFormat);
    const std::string email = ui::ask("Email", emailFormat);
    const int role = ui::askInt("Role (1 = Admin, 2 = Super Admin)", 1, 2);
    const std::string password = promptNewPassword(username);
    ui::showResult(admins_.createAdmin(username, name, email, password, role == 2 ? Role::SuperAdmin : Role::Admin));
}

void App::toggleAdmin() {
    ui::heading("ACTIVATE / DEACTIVATE ADMINISTRATOR");
    reports_.printAdmins();
    ui::info("Type /c at any prompt to cancel.");
    const std::string username = util::toLower(ui::ask("Username", [this](const std::string& v) -> std::string {
        return adminExists(v) ? std::string() : "No administrator found with that username.";
    }));
    bool active = true;
    for (const Admin& a : admins_.admins()) {
        if (a.username() == username) active = a.isActive();
    }
    const bool activate = !active;
    if (!ui::confirm(std::string(activate ? "Activate" : "Deactivate") + " the account '" + username + "'?")) {
        ui::info("No changes were made.");
        return;
    }
    ui::showResult(admins_.setActive(username, activate));
}

void App::resetAdminPassword() {
    ui::heading("RESET ADMINISTRATOR PASSWORD");
    ui::info("Type /c at any prompt to cancel.");
    const std::string username = util::toLower(ui::ask("Username", [this](const std::string& v) -> std::string {
        return adminExists(v) ? std::string() : "No administrator found with that username.";
    }));
    const std::string password = promptNewPassword(username);
    ui::showResult(admins_.resetPassword(username, password));
}

void App::librarySettings() {
    ui::heading("LIBRARY SETTINGS");
    ui::info("Type /c at any prompt to cancel. Press Enter to keep the current value. Changing the fine rate does not alter fines already recorded.");
    const double rate = ui::askDoubleKeep("Fine per overdue day (" "\xE2\x82\xB9" ")", 0.0, 10000.0, library_.fineRatePerDay());
    const int days = ui::askIntKeep("Issue period in days", 1, 90, library_.issuePeriodDays());
    if (!ui::confirm("Apply these settings?")) {
        ui::info("No changes were made.");
        return;
    }
    if (util::roundMoney(rate) != library_.fineRatePerDay()) ui::showResult(library_.setFineRate(rate));
    if (days != library_.issuePeriodDays()) ui::showResult(library_.setIssuePeriod(days));
}
