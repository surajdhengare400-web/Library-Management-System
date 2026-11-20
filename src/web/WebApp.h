// WebApp.h - the website. It turns HTTP requests into calls on Library / AdminManager and renders
// HTML pages. The server is single-threaded, so one request is handled at a time.
#pragma once
#include <ctime>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "AdminManager.h"
#include "FileManager.h"
#include "HttpServer.h"
#include "Library.h"

class WebApp {
public:
    WebApp(const std::string& dataDirectory, const std::string& publicDirectory);

    bool init(std::vector<std::string>& warnings, std::string& fatalError);
    HttpResponse handle(const HttpRequest& request);

private:
    struct Session {
        std::string username;
        std::string csrf;
        std::time_t lastSeen = 0;
        std::string flashText;
        std::string flashType;
    };
    struct Context {
        const HttpRequest& req;
        Session* session;
    };
    using Handler = std::function<HttpResponse(Context&)>;
    using FormValues = std::map<std::string, std::string>;

    // ---- infrastructure (WebApp.cpp) ----
    void registerRoutes();
    Session* findSession(const HttpRequest& request);
    void pruneSessions();
    bool originAllowed(const HttpRequest& request) const;
    HttpResponse serveStylesheet() const;
    HttpResponse page(Context& c, const std::string& title, const std::string& active, const std::string& body,
                      int status = 200);
    HttpResponse message(int status, const std::string& title, const std::string& text) const;
    HttpResponse redirectWith(Context& c, const std::string& location, const Result& result);
    void flash(Context& c, const std::string& text, const std::string& type);
    std::string csrfField(Context& c) const;
    std::string postForm(Context& c, const std::string& action, const std::string& inner,
                         const std::string& cssClass = "") const;
    static std::string param(const Context& c, const std::string& name);       // trimmed
    static std::string rawParam(const Context& c, const std::string& name);    // untouched (passwords)

    // ---- sign-in (WebApp.cpp) ----
    HttpResponse loginPage(const std::string& notice, const std::string& error, int status = 200) const;
    HttpResponse loginSubmit(const HttpRequest& request);
    HttpResponse setupPage(const std::string& error, const FormValues& values, int status = 200) const;
    HttpResponse setupSubmit(const HttpRequest& request);
    HttpResponse logoutSubmit(Context& c);

    // ---- dashboard, account, administration (WebApp.cpp) ----
    HttpResponse dashboard(Context& c);
    HttpResponse accountPage(Context& c);
    HttpResponse accountPassword(Context& c);
    HttpResponse accountProfile(Context& c);
    HttpResponse adminPage(Context& c);
    HttpResponse adminCreate(Context& c);
    HttpResponse adminToggleConfirm(Context& c);
    HttpResponse adminToggle(Context& c);
    HttpResponse adminResetForm(Context& c);
    HttpResponse adminReset(Context& c);
    HttpResponse adminSettings(Context& c);

    // ---- books, members, issues, reports (WebCatalog.cpp) ----
    HttpResponse booksList(Context& c);
    HttpResponse bookNew(Context& c);
    HttpResponse bookAdd(Context& c);
    HttpResponse bookEdit(Context& c);
    HttpResponse bookUpdate(Context& c);
    HttpResponse bookDeleteConfirm(Context& c);
    HttpResponse bookDelete(Context& c);
    HttpResponse membersList(Context& c);
    HttpResponse memberNew(Context& c);
    HttpResponse memberAdd(Context& c);
    HttpResponse memberEdit(Context& c);
    HttpResponse memberUpdate(Context& c);
    HttpResponse memberView(Context& c);
    HttpResponse issuesList(Context& c);
    HttpResponse issueNewForm(Context& c);
    HttpResponse issueNewSubmit(Context& c);
    HttpResponse issueReturn(Context& c);
    HttpResponse issuePayFine(Context& c);
    HttpResponse reportsPage(Context& c);
    HttpResponse historyPage(Context& c);

    std::string bookForm(Context& c, bool editing, const FormValues& values);
    std::string memberForm(Context& c, bool editing, const FormValues& values);
    std::string issuesTable(Context& c, const std::vector<IssueRecord>& issues, const Date& asOf, bool showReturned, bool actions);

    FileManager files_;
    Library library_;
    AdminManager admins_;
    std::string publicDirectory_;
    std::map<std::string, Handler> routes_;
    std::map<std::string, Session> sessions_;
    int failedLogins_ = 0;
    std::time_t lockedUntil_ = 0;
};
