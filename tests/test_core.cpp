// tests/test_main.cpp - automated checks for the core classes (no console interaction).
// Build with build_tests.bat (or the g++ command in README.md) and run LibraryTests.exe.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "AdminManager.h"
#include "Date.h"
#include "FileManager.h"
#include "Library.h"
#include "ReportManager.h"
#include "Security.h"
#include "Utils.h"

namespace fs = std::filesystem;

static int g_passed = 0;
static int g_failed = 0;

#define CHECK(cond)                                                                      \
    do {                                                                                 \
        if (cond) { ++g_passed; }                                                        \
        else { ++g_failed; std::cout << "  FAIL (line " << __LINE__ << "): " #cond "\n"; } \
    } while (0)

static void section(const char* name) { std::cout << "[" << name << "]\n"; }

static std::string slurp(const std::string& path) {
    std::ifstream in(path);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static void spit(const std::string& path, const std::string& text) {
    std::ofstream out(path, std::ios::trunc);
    out << text;
}

static const char* kDir = "TestData_tmp";
static const std::string kPass = "Str0ng!Pass";

static void testDate() {
    section("Date");
    Date d;
    CHECK(Date::parse("2024-02-29", d) && d.toString() == "2024-02-29");
    CHECK(!Date::parse("2023-02-29", d));
    CHECK(!Date::parse("2024-13-01", d));
    CHECK(!Date::parse("2024-1-01", d));
    CHECK(!Date::parse("abcd-ef-gh", d));
    CHECK(Date(2024, 1, 31).addDays(1).toString() == "2024-02-01");
    CHECK(Date(2024, 2, 28).addDays(1).toString() == "2024-02-29");
    CHECK(Date(2023, 12, 31).addDays(1).toString() == "2024-01-01");
    CHECK(Date(2024, 3, 1).daysAfter(Date(2024, 2, 1)) == 29);
    CHECK(Date(2000, 1, 1).daysAfter(Date(1999, 12, 31)) == 1);
    CHECK(Date(2026, 1, 15) < Date(2026, 1, 25));
    CHECK(Date(1970, 1, 1).toDayNumber() == 0);
    CHECK(Date::fromDayNumber(Date(2031, 7, 9).toDayNumber()).toString() == "2031-07-09");
}

static void testSecurity() {
    section("Security (hashing)");
    const std::string abc = "abc";
    CHECK(security::toHex(security::sha256(security::Bytes(abc.begin(), abc.end()))) ==
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    const std::string salt = "salt";
    const security::Bytes saltBytes(salt.begin(), salt.end());
    CHECK(security::toHex(security::pbkdf2HmacSha256("password", saltBytes, 1)) ==
          "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b");
    CHECK(security::toHex(security::pbkdf2HmacSha256("password", saltBytes, 2)) ==
          "ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43");
    CHECK(security::toHex(security::pbkdf2HmacSha256("password", saltBytes, 4096)) ==
          "c5e478d59288c841aa530db6845c4c8d962893a001ce4e11a4963873aa98134a");
    const std::string s1 = security::generateSaltHex(), s2 = security::generateSaltHex();
    CHECK(s1.size() == 32 && s1 != s2);
    CHECK(security::constantTimeEquals("abc", "abc") && !security::constantTimeEquals("abc", "abd"));
}

static void testValidators() {
    section("Validators");
    CHECK(validate::checkPassword("Str0ng!Pass", "admin").empty());
    CHECK(!validate::checkPassword("short1!A", "admin").empty() == false);   // 8 chars is allowed
    CHECK(!validate::checkPassword("alllowercase1!", "admin").empty());
    CHECK(!validate::checkPassword("NoDigits!!", "admin").empty());
    CHECK(!validate::checkPassword("NoSpecial123", "admin").empty());
    CHECK(!validate::checkPassword("Sh0rt!", "admin").empty());
    CHECK(!validate::checkPassword("Has Space1!", "admin").empty());
    CHECK(!validate::checkPassword("Admin123!x", "admin").empty());           // contains username
    CHECK(!validate::checkPassword("Password1!", "bob").empty());             // common
    CHECK(validate::checkEmail("a.b@example.com").empty());
    CHECK(!validate::checkEmail("no-at.example.com").empty());
    CHECK(!validate::checkEmail("a@b").empty());
    CHECK(!validate::checkEmail("a b@example.com").empty());
    CHECK(validate::checkPhone("+91 98765-43210").empty());
    CHECK(!validate::checkPhone("12ab567").empty());
    CHECK(!validate::checkPhone("123").empty());
    CHECK(validate::checkId("B-001").empty() && !validate::checkId("B 1").empty() && !validate::checkId("ab").empty());
    CHECK(validate::checkName("Asha Verma").empty() && !validate::checkName("1234").empty());
}

static void testFileManager() {
    section("FileManager");
    fs::remove_all(kDir);
    FileManager fm(kDir);
    std::string err;
    CHECK(fm.ensureDataDirectory(err) && fs::is_directory(kDir));
    std::vector<std::string> fields = {"plain", "with|pipe", "back\\slash", "multi\nline", ""};
    std::vector<std::string> back;
    CHECK(FileManager::split(FileManager::join(fields), back) && back == fields);
    CHECK(!FileManager::split("bad\\x", back));
    CHECK(fm.writeLinesAtomic("t.txt", {"one", "two"}, err));
    CHECK(fm.writeLinesAtomic("t.txt", {"three"}, err));          // replaces an existing file
    std::vector<std::string> lines;
    CHECK(fm.readLines("t.txt", lines, err) == FileManager::ReadStatus::Ok && lines.size() == 1 && lines[0] == "three");
    CHECK(!fs::exists(fs::path(kDir) / "t.txt.tmp"));
    CHECK(fm.readLines("missing.txt", lines, err) == FileManager::ReadStatus::Missing);
    fs::remove_all(kDir);
}

static void testBooksMembersIssues() {
    section("Books, members, issues and fines");
    fs::remove_all(kDir);
    FileManager fm(kDir);
    Library lib(fm);
    std::vector<std::string> warnings;
    std::string fatal;
    CHECK(lib.load(warnings, fatal) && warnings.empty());      // missing files are fine

    CHECK(lib.addBook("b001", "Clean Code", "Robert Martin", "Programming", 2008, 2).ok);
    CHECK(!lib.addBook("B001", "Dup", "X", "Y", 2000, 1).ok);                // duplicate (case-insensitive)
    CHECK(!lib.addBook("B002", "Neg", "X", "Y", 2000, -3).ok);               // negative copies
    CHECK(!lib.addBook("B002", "Zero", "X", "Y", 2000, 0).ok);
    CHECK(!lib.addBook("B002", "", "X", "Y", 2000, 1).ok);                   // empty title
    CHECK(!lib.addBook("B002", "Future", "X", "Y", Date::today().year() + 5, 1).ok);
    CHECK(!lib.addBook("B 2", "Bad id", "X", "Y", 2000, 1).ok);
    CHECK(lib.addBook("B002", "Algorithms", "Sedgewick", "Computer Science", 2011, 1).ok);
    CHECK(lib.books().size() == 2 && lib.totalCopies() == 3 && lib.availableCopies() == 3);
    CHECK(lib.searchBooks(BookField::Title, "clean").size() == 1);
    CHECK(lib.searchBooks(BookField::Author, "SEDGE").size() == 1);
    CHECK(lib.searchBooks(BookField::Category, "program").size() == 1);
    CHECK(lib.findBook("b002").has_value() && !lib.findBook("ZZZ").has_value());

    CHECK(lib.registerMember("M001", "Asha Verma", "asha@example.com", "9876543210", Date(2026, 1, 1)).ok);
    CHECK(!lib.registerMember("m001", "Other", "o@example.com", "9876543211", Date(2026, 1, 1)).ok);   // dup id
    CHECK(!lib.registerMember("M002", "Ravi", "ASHA@example.com", "9876543212", Date(2026, 1, 1)).ok); // dup email
    CHECK(!lib.registerMember("M002", "Ravi", "bad-email", "9876543212", Date(2026, 1, 1)).ok);
    CHECK(!lib.registerMember("M002", "Ravi", "r@example.com", "12", Date(2026, 1, 1)).ok);
    CHECK(lib.registerMember("M002", "Ravi Kumar", "ravi@example.com", "98765 43212", Date(2026, 1, 1)).ok);
    CHECK(lib.updateMember("M002", "Ravi K", "ravi@example.com", "9999999999").ok);
    CHECK(lib.findMember("M002")->fullName() == "Ravi K");

    const Date day(2026, 1, 1);
    CHECK(!lib.issueBook("B001", "NOPE", day).ok);                           // invalid member
    CHECK(!lib.issueBook("NOPE", "M001", day).ok);                           // invalid book
    CHECK(lib.issueBook("B001", "M001", day).ok);                            // I0001, due Jan 15
    CHECK(!lib.issueBook("B001", "M001", day).ok);                           // duplicate active issue
    CHECK(lib.issueBook("B001", "M002", day).ok);                            // second copy
    CHECK(!lib.issueBook("B001", "M002", day).ok);                           // none left
    CHECK(lib.findBook("B001")->availableCopies() == 0 && lib.issuedCopies() == 2);
    CHECK(!lib.deleteBook("B001").ok);                                       // copies issued
    CHECK(!lib.updateBook("B001", "Clean Code", "Robert Martin", "Programming", 2008, 1).ok);   // below issued
    CHECK(lib.updateBook("B001", "Clean Code 2nd", "Robert Martin", "Programming", 2008, 3).ok);
    CHECK(lib.findBook("B001")->availableCopies() == 1);
    CHECK(lib.findIssue(1)->dueDate().toString() == "2026-01-15");

    CHECK(lib.overdueIssues(Date(2026, 1, 15)).empty());                      // due today is not overdue
    CHECK(lib.overdueIssues(Date(2026, 1, 20)).size() == 2);
    CHECK(lib.accruingFines(Date(2026, 1, 20)) == 50.0);                     // 5 days x 5 x 2 issues

    CHECK(!lib.returnBook(1, Date(2025, 12, 1)).ok);                         // before issue date
    CHECK(lib.returnBook(1, Date(2026, 1, 25)).ok);                          // 10 days late -> 50.00
    CHECK(lib.findIssue(1)->fineAmount() == 50.0 && lib.findIssue(1)->hasUnpaidFine());
    CHECK(!lib.returnBook(1, Date(2026, 1, 25)).ok);                         // already returned
    CHECK(lib.returnBook(2, Date(2026, 1, 15)).ok);                          // on time
    CHECK(lib.findIssue(2)->fineAmount() == 0.0 && !lib.findIssue(2)->hasUnpaidFine());
    CHECK(lib.availableCopies() == lib.totalCopies());
    CHECK(lib.outstandingFines(Date(2026, 2, 1)) == 50.0);
    CHECK(!lib.payFine(2).ok);                                               // no fine
    CHECK(lib.payFine(1).ok && !lib.payFine(1).ok);
    CHECK(lib.outstandingFines(Date(2026, 2, 1)) == 0.0 && lib.collectedFines() == 50.0);

    CHECK(lib.setFineRate(7.5).ok && lib.fineRatePerDay() == 7.5);
    CHECK(!lib.setFineRate(-1).ok && !lib.setIssuePeriod(0).ok && lib.setIssuePeriod(7).ok);
    CHECK(lib.issueBook("B002", "M001", Date(2026, 3, 1)).ok);               // due Mar 8 with 7-day period
    CHECK(lib.returnBook(3, Date(2026, 3, 12)).ok && lib.findIssue(3)->fineAmount() == 30.0);   // 4 x 7.5

    for (int i = 0; i < 5; ++i) {                                            // member issue limit
        const std::string id = "X00" + std::to_string(i);
        CHECK(lib.addBook(id, "Filler " + id, "A", "C", 2000, 1).ok);
        CHECK(lib.issueBook(id, "M002", Date(2026, 4, 1)).ok == true);
    }
    CHECK(lib.addBook("X999", "One more", "A", "C", 2000, 1).ok && !lib.issueBook("X999", "M002", Date(2026, 4, 1)).ok);
    CHECK(lib.deleteBook("X999").ok);

    // Persistence: a fresh Library object must see exactly the same data.
    Library again(fm);
    warnings.clear();
    CHECK(again.load(warnings, fatal) && warnings.empty());
    CHECK(again.books().size() == lib.books().size() && again.members().size() == 2);
    CHECK(again.issues().size() == lib.issues().size() && again.transactions().size() == lib.transactions().size());
    CHECK(again.fineRatePerDay() == 7.5 && again.issuePeriodDays() == 7);
    CHECK(again.findIssue(3)->fineAmount() == 30.0 && again.findIssue(1)->isFinePaid());
    CHECK(again.findBook("X000")->availableCopies() == 0);

    // Reports run without crashing and mention the right data.
    AdminManager admins(fm, again);
    ReportManager reports(again, admins);
    std::ostringstream capture;
    std::streambuf* old = std::cout.rdbuf(capture.rdbuf());
    reports.dashboard(Date(2026, 4, 2));
    reports.inventoryReport();
    reports.memberReport();
    reports.issuedBooksReport(Date(2026, 6, 1));
    reports.returnedBooksReport(Date(2026, 6, 1));
    reports.overdueReport(Date(2026, 6, 1));
    reports.fineReport(Date(2026, 6, 1));
    reports.transactionReport(5);
    reports.statisticsReport(Date(2026, 6, 1));
    std::cout.rdbuf(old);
    CHECK(capture.str().find("Outstanding fines") != std::string::npos);
    CHECK(capture.str().find("Fine rate per day") != std::string::npos);
    fs::remove_all(kDir);
}

static void testMalformedFiles() {
    section("Malformed and missing data files");
    fs::remove_all(kDir);
    fs::create_directories(kDir);
    spit(std::string(kDir) + "/books.txt",
         "B001|Good Book|Auth|Cat|2001|2|2\n"
         "this line is garbage\n"
         "B002|Bad Counts|Auth|Cat|2001|1|5\n"          // available > total
         "B003|Bad Year|Auth|Cat|abcd|1|1\n"
         "B001|Duplicate Id|Auth|Cat|2001|1|1\n"
         "B004|Escape\\xBad|Auth|Cat|2001|1|1\n"        // invalid escape
         "B005|Fine Book|Auth|Cat|1999|3|3\n");
    spit(std::string(kDir) + "/issue_records.txt", "1|B005|M001|2026-01-01|2026-01-15|-|0.00|0\n1|B005|M001|2026-01-01|2026-01-15|-|0.00|0\nxx\n");
    spit(std::string(kDir) + "/settings.txt", "not-a-number|x\n");
    FileManager fm(kDir);
    Library lib(fm);
    std::vector<std::string> warnings;
    std::string fatal;
    CHECK(lib.load(warnings, fatal));
    CHECK(lib.books().size() == 2);                                    // B001 and B005 survive
    CHECK(lib.findBook("B001").has_value() && lib.findBook("B005").has_value());
    CHECK(lib.findBook("B005")->availableCopies() == 2);   // reconciled with the active issue
    CHECK(lib.fineRatePerDay() == 5.0 && lib.issuePeriodDays() == 14);  // defaults after a bad settings file
    CHECK(warnings.size() >= 3);
    int backups = 0;
    for (const auto& entry : fs::directory_iterator(kDir)) {
        if (entry.path().filename().string().find(".corrupt-") != std::string::npos) ++backups;
    }
    CHECK(backups >= 2);                                               // originals were preserved
    CHECK(slurp(std::string(kDir) + "/books.txt").find("this line is garbage") != std::string::npos);   // not overwritten yet
    CHECK(lib.addBook("N001", "New", "A", "C", 2020, 1).ok);           // next save rewrites with valid records only
    CHECK(slurp(std::string(kDir) + "/books.txt").find("garbage") == std::string::npos);
    bool garbageBackedUp = false;
    for (const auto& entry : fs::directory_iterator(kDir)) {
        if (entry.path().filename().string().find("books.txt.corrupt-") != std::string::npos &&
            slurp(entry.path().string()).find("this line is garbage") != std::string::npos) garbageBackedUp = true;
    }
    CHECK(garbageBackedUp);

    // Corrupted admin file
    spit(std::string(kDir) + "/admins.txt", "broken|record\n");
    Library lib2(fm);
    AdminManager admins(fm, lib2);
    warnings.clear();
    CHECK(admins.load(warnings, fatal) && !admins.hasAdmins() && !warnings.empty());
    fs::remove_all(kDir);
}

static void testAdmins() {
    section("Authentication and permissions");
    fs::remove_all(kDir);
    FileManager fm(kDir);
    Library lib(fm);
    AdminManager admins(fm, lib);
    std::vector<std::string> warnings;
    std::string fatal;
    CHECK(lib.load(warnings, fatal) && admins.load(warnings, fatal) && !admins.hasAdmins());

    CHECK(!admins.createFirstSuperAdmin("root", "Root User", "root@example.com", "weak").ok);
    CHECK(!admins.createFirstSuperAdmin("ro", "Root User", "root@example.com", kPass).ok);
    CHECK(admins.createFirstSuperAdmin("Root", "Root User", "root@example.com", kPass).ok);
    CHECK(!admins.createFirstSuperAdmin("other", "Other", "o@example.com", kPass).ok);   // only once

    const std::string stored = slurp(std::string(kDir) + "/admins.txt");
    CHECK(stored.find(kPass) == std::string::npos);                    // never plaintext
    CHECK(stored.find("200000") != std::string::npos);                 // iteration count recorded

    CHECK(!admins.login("root", "wrong").ok && !admins.login("nobody", kPass).ok);
    CHECK(!admins.isLoggedIn());
    CHECK(admins.login("ROOT", kPass).ok && admins.isLoggedIn() && admins.isSuperAdmin());

    CHECK(!admins.createAdmin("clerk", "Clerk One", "clerk@example.com", "weak", Role::Admin).ok);
    CHECK(admins.createAdmin("clerk", "Clerk One", "clerk@example.com", "Cl3rk!Passw", Role::Admin).ok);
    CHECK(!admins.createAdmin("CLERK", "Dup", "d@example.com", "Cl3rk!Passw", Role::Admin).ok);
    CHECK(!admins.setActive("root", false).ok);                         // not yourself
    CHECK(admins.setActive("clerk", false).ok);
    admins.logout();
    CHECK(!admins.isLoggedIn());
    CHECK(!admins.login("clerk", "Cl3rk!Passw").ok);                    // deactivated
    CHECK(admins.login("root", kPass).ok && admins.setActive("clerk", true).ok);
    admins.logout();

    CHECK(admins.login("clerk", "Cl3rk!Passw").ok && !admins.isSuperAdmin());
    CHECK(!admins.createAdmin("x1", "X One", "x1@example.com", "Xx1!Passw0rd", Role::Admin).ok);   // normal admin
    CHECK(!admins.setActive("root", false).ok);
    CHECK(!admins.resetPassword("root", "N3w!Passw0rd").ok);
    CHECK(!admins.changeOwnPassword("bad", "N3w!Passw0rd").ok);                // wrong current
    CHECK(!admins.changeOwnPassword("Cl3rk!Passw", "weak").ok);                // weak new
    CHECK(!admins.changeOwnPassword("Cl3rk!Passw", "Cl3rk!Passw").ok);         // same
    CHECK(admins.changeOwnPassword("Cl3rk!Passw", "N3w!Passw0rd").ok);
    CHECK(admins.updateOwnProfile("Clerk Updated", "clerk2@example.com").ok);
    CHECK(!admins.updateOwnProfile("1", "clerk2@example.com").ok);
    admins.logout();
    CHECK(!admins.login("clerk", "Cl3rk!Passw").ok && admins.login("clerk", "N3w!Passw0rd").ok);
    admins.logout();

    // Last active Super Admin protection
    CHECK(admins.login("root", kPass).ok);
    CHECK(admins.createAdmin("boss2", "Second Boss", "boss2@example.com", "B0ss!Passw0rd", Role::SuperAdmin).ok);
    CHECK(admins.setActive("boss2", false).ok);                         // allowed: root stays active
    CHECK(admins.setActive("boss2", true).ok);
    admins.logout();
    CHECK(admins.login("boss2", "B0ss!Passw0rd").ok);
    CHECK(admins.setActive("root", false).ok);                          // boss2 is still active
    CHECK(!admins.setActive("boss2", false).ok);                        // cannot deactivate self (also the last one)
    CHECK(admins.activeSuperAdminCount() == 1);
    admins.logout();
    CHECK(!admins.login("root", kPass).ok);                             // root is deactivated now
    CHECK(admins.login("boss2", "B0ss!Passw0rd").ok && admins.resetPassword("clerk", "Res3t!Passw0rd").ok);
    admins.logout();
    CHECK(admins.login("clerk", "Res3t!Passw0rd").ok);
    admins.logout();

    // Persistence of accounts
    Library lib2(fm);
    AdminManager reloaded(fm, lib2);
    warnings.clear();
    CHECK(lib2.load(warnings, fatal) && reloaded.load(warnings, fatal));
    CHECK(reloaded.admins().size() == 3 && reloaded.login("boss2", "B0ss!Passw0rd").ok);
    CHECK(!reloaded.login("root", kPass).ok);

    // Transaction history for admin activity
    bool created = false, deactivated = false, activated = false, changed = false;
    for (const Transaction& t : lib2.transactions()) {
        if (t.type() == Activity::AdminCreated) created = true;
        if (t.type() == Activity::AdminDeactivated) deactivated = true;
        if (t.type() == Activity::AdminActivated) activated = true;
        if (t.type() == Activity::PasswordChanged) changed = true;
    }
    CHECK(created && deactivated && activated && changed);
    fs::remove_all(kDir);
}

int main() {
    testDate();
    testSecurity();
    testValidators();
    testFileManager();
    testBooksMembersIssues();
    testMalformedFiles();
    testAdmins();
    fs::remove_all(kDir);
    std::cout << "\nPassed: " << g_passed << "   Failed: " << g_failed << '\n';
    return g_failed == 0 ? 0 : 1;
}
