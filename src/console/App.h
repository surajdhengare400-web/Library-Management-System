// App.h - the menu-driven user interface. It owns the application objects (composition) and
// translates user input into calls on Library / AdminManager / ReportManager.
#pragma once
#include <functional>
#include <string>

#include "AdminManager.h"
#include "Console.h"
#include "FileManager.h"
#include "Library.h"
#include "ReportManager.h"

class App {
public:
    App();
    int run();

private:
    // start-up and authentication
    bool startup();
    void firstTimeSetup();
    bool loginFlow();
    void mainMenu();
    void guarded(const std::function<void()>& action);
    std::string promptNewPassword(const std::string& username);

    // menus
    void bookMenu();
    void memberMenu();
    void issueReturnMenu();
    void reportsMenu();
    void historyMenu();
    void accountMenu();
    void adminMenu();

    // book actions
    void addBook();
    void searchBookById();
    void searchBooksBy(BookField field, const std::string& label);
    void updateBook();
    void deleteBook();

    // member actions
    void registerMember();
    void searchMembers();
    void updateMember();
    void memberIssueHistory();

    // issue actions
    void issueBook();
    void returnBook();
    void payFine();

    // account / administration actions
    void showProfile();
    void changePassword();
    void updateProfile();
    void createAdmin();
    void toggleAdmin();
    void resetAdminPassword();
    void librarySettings();

    bool adminExists(const std::string& username) const;
    ui::Validator existingBookId() const;
    ui::Validator existingMemberId() const;

    FileManager files_;
    Library library_;
    AdminManager admins_;
    ReportManager reports_;
};
