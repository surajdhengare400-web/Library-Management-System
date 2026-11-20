#include "WebApp.h"

#include <fstream>
#include <sstream>

#include "Html.h"
#include "Security.h"
#include "Utils.h"

namespace {
constexpr std::time_t kSessionIdleSeconds = 2 * 60 * 60;
constexpr std::size_t kMaxSessions = 200;
constexpr int kMaxFailedLogins = 5;
constexpr std::time_t kLockSeconds = 30;

std::string pageHeadingNote(const std::string& text) { return "<p class=\"muted\">" + html::esc(text) + "</p>"; }
}  // namespace

WebApp::WebApp(const std::string& dataDirectory, const std::string& publicDirectory)
    : files_(dataDirectory), library_(files_), admins_(files_, library_), publicDirectory_(publicDirectory) {
    registerRoutes();
}

bool WebApp::init(std::vector<std::string>& warnings, std::string& fatalError) {
    if (!library_.load(warnings, fatalError)) return false;
    if (!admins_.load(warnings, fatalError)) return false;
    return true;
}

void WebApp::registerRoutes() {
    auto bind = [this](HttpResponse (WebApp::*fn)(Context&)) {
        return Handler([this, fn](Context& c) { return (this->*fn)(c); });
    };
    routes_["GET /"] = bind(&WebApp::dashboard);
    routes_["POST /logout"] = bind(&WebApp::logoutSubmit);

    routes_["GET /books"] = bind(&WebApp::booksList);
    routes_["GET /books/new"] = bind(&WebApp::bookNew);
    routes_["POST /books/add"] = bind(&WebApp::bookAdd);
    routes_["GET /books/edit"] = bind(&WebApp::bookEdit);
    routes_["POST /books/update"] = bind(&WebApp::bookUpdate);
    routes_["GET /books/delete"] = bind(&WebApp::bookDeleteConfirm);
    routes_["POST /books/delete"] = bind(&WebApp::bookDelete);

    routes_["GET /members"] = bind(&WebApp::membersList);
    routes_["GET /members/new"] = bind(&WebApp::memberNew);
    routes_["POST /members/add"] = bind(&WebApp::memberAdd);
    routes_["GET /members/edit"] = bind(&WebApp::memberEdit);
    routes_["POST /members/update"] = bind(&WebApp::memberUpdate);
    routes_["GET /members/view"] = bind(&WebApp::memberView);

    routes_["GET /issues"] = bind(&WebApp::issuesList);
    routes_["GET /issues/new"] = bind(&WebApp::issueNewForm);
    routes_["POST /issues/new"] = bind(&WebApp::issueNewSubmit);
    routes_["POST /issues/return"] = bind(&WebApp::issueReturn);
    routes_["POST /issues/payfine"] = bind(&WebApp::issuePayFine);

    routes_["GET /reports"] = bind(&WebApp::reportsPage);
    routes_["GET /history"] = bind(&WebApp::historyPage);

    routes_["GET /account"] = bind(&WebApp::accountPage);
    routes_["POST /account/password"] = bind(&WebApp::accountPassword);
    routes_["POST /account/profile"] = bind(&WebApp::accountProfile);

    routes_["GET /admin"] = bind(&WebApp::adminPage);
    routes_["POST /admin/create"] = bind(&WebApp::adminCreate);
    routes_["GET /admin/toggle"] = bind(&WebApp::adminToggleConfirm);
    routes_["POST /admin/toggle"] = bind(&WebApp::adminToggle);
    routes_["GET /admin/reset"] = bind(&WebApp::adminResetForm);
    routes_["POST /admin/reset"] = bind(&WebApp::adminReset);
    routes_["POST /admin/settings"] = bind(&WebApp::adminSettings);
}

// ------------------------------------------------------------------------------------------
// Request pipeline
// ------------------------------------------------------------------------------------------

HttpResponse WebApp::handle(const HttpRequest& req) {
    if (req.method == "GET" && req.path == "/static/style.css") return serveStylesheet();
    if (req.path == "/favicon.ico") {
        HttpResponse r;
        r.status = 204;
        return r;
    }
    if (req.method != "GET" && req.method != "POST") return message(405, "Method not allowed", "Only GET and POST are supported.");
    if (req.method == "POST" && !originAllowed(req)) {
        return message(403, "Request blocked", "This request came from a different website and was refused.");
    }
    pruneSessions();

    if (!admins_.hasAdmins()) {
        if (req.path == "/setup") return req.method == "POST" ? setupSubmit(req) : setupPage("", {});
        return HttpResponse::redirect("/setup");
    }
    if (req.path == "/setup") return HttpResponse::redirect("/login");
    if (req.path == "/login") {
        if (req.method == "POST") return loginSubmit(req);
        return loginPage(req.query.count("created") ? "Super Admin account created. Please log in." : "", "");
    }

    Session* session = findSession(req);
    if (session == nullptr) return HttpResponse::redirect("/login");
    if (!admins_.resumeSession(session->username)) {
        sessions_.erase(req.cookies.at("sid"));
        return HttpResponse::redirect("/login");
    }
    Context ctx{req, session};

    if (req.method == "POST" &&
        !security::constantTimeEquals(req.param("csrf"), session->csrf)) {
        return message(403, "Invalid form", "The security token of this form is missing or expired. Go back, reload the page and try again.");
    }
    if (req.path == "/admin" || req.path.rfind("/admin/", 0) == 0) {
        if (!admins_.isSuperAdmin()) {
            return message(403, "Access denied", "Only a Super Admin can open the Administration area.");
        }
    }
    const auto route = routes_.find(req.method + " " + req.path);
    if (route == routes_.end()) return message(404, "Page not found", "The page you asked for does not exist.");
    return route->second(ctx);
}

WebApp::Session* WebApp::findSession(const HttpRequest& request) {
    const auto cookie = request.cookies.find("sid");
    if (cookie == request.cookies.end()) return nullptr;
    const auto it = sessions_.find(cookie->second);
    if (it == sessions_.end()) return nullptr;
    it->second.lastSeen = std::time(nullptr);
    return &it->second;
}

void WebApp::pruneSessions() {
    const std::time_t now = std::time(nullptr);
    for (auto it = sessions_.begin(); it != sessions_.end();) {
        if (now - it->second.lastSeen > kSessionIdleSeconds) it = sessions_.erase(it);
        else ++it;
    }
}

bool WebApp::originAllowed(const HttpRequest& request) const {
    const std::string origin = request.header("origin");
    if (origin.empty()) return true;
    const std::size_t scheme = origin.find("://");
    if (scheme == std::string::npos) return false;
    return origin.substr(scheme + 3) == request.header("host");
}

HttpResponse WebApp::serveStylesheet() const {
    std::ifstream in(publicDirectory_ + "/style.css", std::ios::binary);
    if (!in) return HttpResponse::text(404, "style.css not found. Start the server from the project folder.\n");
    std::stringstream buffer;
    buffer << in.rdbuf();
    HttpResponse r;
    r.contentType = "text/css; charset=utf-8";
    r.body = buffer.str();
    return r;
}

HttpResponse WebApp::page(Context& c, const std::string& title, const std::string& active, const std::string& body,
                          int status) {
    html::LayoutInfo info;
    info.title = title;
    info.active = active;
    info.csrf = c.session->csrf;
    info.superAdmin = admins_.isSuperAdmin();
    if (const auto admin = admins_.currentAdmin()) {
        info.userName = admin->fullName();
        info.roleName = roleToString(admin->role());
    }
    info.flashText = c.session->flashText;
    info.flashType = c.session->flashType;
    c.session->flashText.clear();
    c.session->flashType.clear();
    return HttpResponse::html(html::layout(info, body), status);
}

HttpResponse WebApp::message(int status, const std::string& title, const std::string& text) const {
    return HttpResponse::html(html::publicLayout(title, "<p>" + html::esc(text) + "</p><p><a class=\"btn btn-primary\" href=\"/\">Back to the start page</a></p>", "", ""), status);
}

void WebApp::flash(Context& c, const std::string& text, const std::string& type) {
    c.session->flashText = text;
    c.session->flashType = type;
}

HttpResponse WebApp::redirectWith(Context& c, const std::string& location, const Result& result) {
    flash(c, result.message, result.ok ? "success" : "error");
    return HttpResponse::redirect(location);
}

std::string WebApp::csrfField(Context& c) const { return html::hidden("csrf", c.session->csrf); }

std::string WebApp::postForm(Context& c, const std::string& action, const std::string& inner,
                             const std::string& cssClass) const {
    return "<form method=\"post\" action=\"" + html::esc(action) + "\"" +
           (cssClass.empty() ? "" : " class=\"" + cssClass + "\"") + ">" + csrfField(c) + inner + "</form>";
}

std::string WebApp::param(const Context& c, const std::string& name) { return util::trim(c.req.param(name)); }
std::string WebApp::rawParam(const Context& c, const std::string& name) { return c.req.param(name); }

// ------------------------------------------------------------------------------------------
// Sign-in
// ------------------------------------------------------------------------------------------

HttpResponse WebApp::loginPage(const std::string& notice, const std::string& error, int status) const {
    std::string form =
        "<form method=\"post\" action=\"/login\" class=\"stack\">" +
        html::field("Username", "username", "", "text", "autocomplete=\"username\" required autofocus") +
        html::field("Password", "password", "", "password", "autocomplete=\"current-password\" required") +
        "<button type=\"submit\" class=\"btn btn-primary btn-block\">Log in</button></form>";
    const std::string text = !error.empty() ? error : notice;
    return HttpResponse::html(html::publicLayout("Administrator Login", form, text, !error.empty() ? "error" : "success"), status);
}

HttpResponse WebApp::loginSubmit(const HttpRequest& request) {
    const std::time_t now = std::time(nullptr);
    if (now < lockedUntil_) {
        return loginPage("", "Too many failed attempts. Please wait " + std::to_string(lockedUntil_ - now) + " second(s).", 429);
    }
    const Result result = admins_.login(request.param("username"), request.param("password"));
    if (!result.ok) {
        if (++failedLogins_ >= kMaxFailedLogins) {
            failedLogins_ = 0;
            lockedUntil_ = now + kLockSeconds;
        }
        return loginPage("", result.message, 200);
    }
    failedLogins_ = 0;
    if (sessions_.size() >= kMaxSessions) sessions_.clear();
    const std::string token = security::randomHex(32);
    Session session;
    session.username = admins_.currentUsername();
    session.csrf = security::randomHex(16);
    session.lastSeen = now;
    session.flashText = result.message;
    session.flashType = "success";
    sessions_[token] = session;
    HttpResponse response = HttpResponse::redirect("/");
    response.headers.emplace_back("Set-Cookie", "sid=" + token + "; Path=/; HttpOnly; SameSite=Strict; Max-Age=7200");
    return response;
}

HttpResponse WebApp::setupPage(const std::string& error, const FormValues& values, int status) const {
    auto value = [&](const char* key) {
        const auto it = values.find(key);
        return it == values.end() ? std::string() : it->second;
    };
    std::string form =
        "<p class=\"muted\">No administrator exists yet. Create the Super Admin account to start. There is no default password.</p>"
        "<form method=\"post\" action=\"/setup\" class=\"stack\">" +
        html::field("Username (3-20 letters, digits, _)", "username", value("username"), "text", "required autofocus") +
        html::field("Full name", "name", value("name"), "text", "required") +
        html::field("Email", "email", value("email"), "email", "required") +
        html::field("Password", "password", "", "password", "autocomplete=\"new-password\" required") +
        html::field("Confirm password", "confirm", "", "password", "autocomplete=\"new-password\" required") +
        "<p class=\"hint\">8-64 characters with an uppercase letter, a lowercase letter, a digit and a special character; no spaces.</p>"
        "<button type=\"submit\" class=\"btn btn-primary btn-block\">Create Super Admin</button></form>";
    return HttpResponse::html(html::publicLayout("First-Time Setup", form, error, "error"), status);
}

HttpResponse WebApp::setupSubmit(const HttpRequest& request) {
    FormValues values{{"username", util::trim(request.param("username"))},
                      {"name", util::trim(request.param("name"))},
                      {"email", util::trim(request.param("email"))}};
    if (request.param("password") != request.param("confirm")) {
        return setupPage("The two passwords do not match.", values);
    }
    const Result result = admins_.createFirstSuperAdmin(values["username"], values["name"], values["email"],
                                                        request.param("password"));
    if (!result.ok) return setupPage(result.message, values);
    return HttpResponse::redirect("/login?created=1");
}

HttpResponse WebApp::logoutSubmit(Context& c) {
    const auto cookie = c.req.cookies.find("sid");
    if (cookie != c.req.cookies.end()) sessions_.erase(cookie->second);
    admins_.logout();
    HttpResponse response = HttpResponse::redirect("/login");
    response.headers.emplace_back("Set-Cookie", "sid=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0");
    return response;
}

// ------------------------------------------------------------------------------------------
// Dashboard
// ------------------------------------------------------------------------------------------

HttpResponse WebApp::dashboard(Context& c) {
    const Date today = Date::today();
    std::string body = "<div class=\"cards\">";
    body += html::statCard("Book titles", std::to_string(library_.books().size()));
    body += html::statCard("Total copies", std::to_string(library_.totalCopies()));
    body += html::statCard("Available copies", std::to_string(library_.availableCopies()), "good");
    body += html::statCard("Issued copies", std::to_string(library_.issuedCopies()));
    body += html::statCard("Registered members", std::to_string(library_.members().size()));
    body += html::statCard("Active issues", std::to_string(library_.activeIssues().size()));
    const std::size_t overdue = library_.overdueIssues(today).size();
    body += html::statCard("Overdue issues", std::to_string(overdue), overdue > 0 ? "bad" : "");
    body += html::statCard("Administrators", std::to_string(admins_.admins().size()));
    const double fines = library_.outstandingFines(today);
    body += html::statCard("Outstanding fines", util::formatMoney(fines), fines > 0.0 ? "bad" : "");
    body += "</div>";

    body += "<div class=\"toolbar\"><a class=\"btn btn-primary\" href=\"/issues/new\">Issue a book</a>"
            "<a class=\"btn\" href=\"/books/new\">Add book</a><a class=\"btn\" href=\"/members/new\">Register member</a></div>";

    body += "<section class=\"panel\"><h2>Overdue books</h2>";
    std::vector<IssueRecord> overdueIssues = library_.overdueIssues(today);
    if (overdueIssues.size() > 5) overdueIssues.erase(overdueIssues.begin() + 5, overdueIssues.end());
    body += issuesTable(c, overdueIssues, today, false, false);
    body += "</section>";

    body += "<section class=\"panel\"><h2>Recent activity</h2>";
    std::vector<std::vector<std::string>> rows;
    const auto& all = library_.transactions();
    for (std::size_t i = all.size(), shown = 0; i > 0 && shown < 8; --i, ++shown) {
        const Transaction& t = all[i - 1];
        rows.push_back({html::esc(t.timestamp()), html::badge(t.type(), "info"), html::esc(t.description())});
    }
    body += html::table({"Time", "Activity", "Description"}, rows, "No activity recorded yet.");
    body += "</section>";
    return page(c, "Dashboard", "dashboard", body);
}

// ------------------------------------------------------------------------------------------
// My account
// ------------------------------------------------------------------------------------------

HttpResponse WebApp::accountPage(Context& c) {
    const auto admin = admins_.currentAdmin();
    if (!admin) return HttpResponse::redirect("/login");
    std::string body = "<div class=\"grid-2\"><section class=\"panel\"><h2>Profile</h2>";
    body += html::table({"Field", "Value"},
                        {{"Username", html::esc(admin->username())},
                         {"Role", html::esc(roleToString(admin->role()))},
                         {"Created", html::esc(admin->createdAt())}},
                        "");
    body += postForm(c, "/account/profile",
                     html::field("Full name", "name", admin->fullName(), "text", "required") +
                         html::field("Email", "email", admin->email(), "email", "required") +
                         "<button type=\"submit\" class=\"btn btn-primary\">Update profile</button>",
                     "stack");
    body += "</section><section class=\"panel\"><h2>Change password</h2>";
    body += postForm(c, "/account/password",
                     html::field("Current password", "current", "", "password", "autocomplete=\"current-password\" required") +
                         html::field("New password", "password", "", "password", "autocomplete=\"new-password\" required") +
                         html::field("Confirm new password", "confirm", "", "password", "autocomplete=\"new-password\" required") +
                         "<p class=\"hint\">8-64 characters with an uppercase letter, a lowercase letter, a digit and a special character; no spaces.</p>"
                         "<button type=\"submit\" class=\"btn btn-primary\">Change password</button>",
                     "stack");
    body += "</section></div>";
    return page(c, "My Account", "account", body);
}

HttpResponse WebApp::accountPassword(Context& c) {
    if (rawParam(c, "password") != rawParam(c, "confirm")) {
        return redirectWith(c, "/account", Result::failure("The new password and its confirmation do not match."));
    }
    return redirectWith(c, "/account", admins_.changeOwnPassword(rawParam(c, "current"), rawParam(c, "password")));
}

HttpResponse WebApp::accountProfile(Context& c) {
    return redirectWith(c, "/account", admins_.updateOwnProfile(param(c, "name"), param(c, "email")));
}

// ------------------------------------------------------------------------------------------
// Administration (Super Admin only - enforced in handle() and again inside AdminManager)
// ------------------------------------------------------------------------------------------

HttpResponse WebApp::adminPage(Context& c) {
    std::vector<std::vector<std::string>> rows;
    for (const Admin& a : admins_.admins()) {
        const std::string user = http::urlEncode(a.username());
        rows.push_back({html::esc(a.username()), html::esc(a.fullName()), html::esc(a.email()),
                        html::badge(roleToString(a.role()), a.isSuperAdmin() ? "info" : "muted"),
                        a.isActive() ? html::badge("Active", "ok") : html::badge("Inactive", "danger"),
                        "<a class=\"btn btn-small\" href=\"/admin/toggle?user=" + user + "\">" +
                            (a.isActive() ? "Deactivate" : "Activate") + "</a> "
                            "<a class=\"btn btn-small\" href=\"/admin/reset?user=" + user + "\">Reset password</a>"});
    }
    std::string body = "<section class=\"panel\"><h2>Administrators</h2>" +
                       html::table({"Username", "Name", "Email", "Role", "Status", "Actions"}, rows, "No administrators.") +
                       "</section>";
    body += "<div class=\"grid-2\"><section class=\"panel\"><h2>Create administrator</h2>";
    body += postForm(c, "/admin/create",
                     html::field("Username", "username", "", "text", "required") +
                         html::field("Full name", "name", "", "text", "required") +
                         html::field("Email", "email", "", "email", "required") +
                         "<label class=\"field\"><span>Role</span><select name=\"role\"><option value=\"admin\">Admin</option>"
                         "<option value=\"super\">Super Admin</option></select></label>" +
                         html::field("Password", "password", "", "password", "autocomplete=\"new-password\" required") +
                         html::field("Confirm password", "confirm", "", "password", "autocomplete=\"new-password\" required") +
                         "<button type=\"submit\" class=\"btn btn-primary\">Create account</button>",
                     "stack");
    body += "</section><section class=\"panel\"><h2>Library settings</h2>";
    char rate[32];
    std::snprintf(rate, sizeof(rate), "%.2f", library_.fineRatePerDay());
    body += postForm(c, "/admin/settings",
                     html::field("Fine per overdue day (" "\xE2\x82\xB9" ")", "fine_rate", rate, "number", "min=\"0\" max=\"10000\" step=\"0.01\" required") +
                         html::field("Issue period (days)", "issue_days", std::to_string(library_.issuePeriodDays()), "number", "min=\"1\" max=\"90\" step=\"1\" required") +
                         pageHeadingNote("Changing the fine rate does not alter fines that were already recorded.") +
                         "<button type=\"submit\" class=\"btn btn-primary\">Save settings</button>",
                     "stack");
    body += "</section></div>";
    return page(c, "Administration", "admin", body);
}

HttpResponse WebApp::adminCreate(Context& c) {
    if (rawParam(c, "password") != rawParam(c, "confirm")) {
        return redirectWith(c, "/admin", Result::failure("The password and its confirmation do not match."));
    }
    const Role role = param(c, "role") == "super" ? Role::SuperAdmin : Role::Admin;
    return redirectWith(c, "/admin", admins_.createAdmin(param(c, "username"), param(c, "name"), param(c, "email"),
                                                          rawParam(c, "password"), role));
}

HttpResponse WebApp::adminToggleConfirm(Context& c) {
    const std::string user = util::toLower(param(c, "user"));
    for (const Admin& a : admins_.admins()) {
        if (a.username() != user) continue;
        const std::string verb = a.isActive() ? "Deactivate" : "Activate";
        std::string body = "<section class=\"panel narrow\"><h2>" + verb + " account?</h2><p>" + verb + " the account <strong>" +
                           html::esc(a.username()) + "</strong> (" + html::esc(a.fullName()) + ")?</p>" +
                           postForm(c, "/admin/toggle",
                                    html::hidden("user", a.username()) + html::hidden("activate", a.isActive() ? "0" : "1") +
                                        "<button type=\"submit\" class=\"btn " + std::string(a.isActive() ? "btn-danger" : "btn-primary") +
                                        "\">" + verb + "</button> <a class=\"btn\" href=\"/admin\">Cancel</a>") +
                           "</section>";
        return page(c, verb + " Administrator", "admin", body);
    }
    return redirectWith(c, "/admin", Result::failure("No administrator found with that username."));
}

HttpResponse WebApp::adminToggle(Context& c) {
    return redirectWith(c, "/admin", admins_.setActive(param(c, "user"), param(c, "activate") == "1"));
}

HttpResponse WebApp::adminResetForm(Context& c) {
    const std::string user = util::toLower(param(c, "user"));
    for (const Admin& a : admins_.admins()) {
        if (a.username() != user) continue;
        std::string body = "<section class=\"panel narrow\"><h2>Reset password for " + html::esc(a.username()) + "</h2>" +
                           postForm(c, "/admin/reset",
                                    html::hidden("user", a.username()) +
                                        html::field("New password", "password", "", "password", "autocomplete=\"new-password\" required") +
                                        html::field("Confirm new password", "confirm", "", "password", "autocomplete=\"new-password\" required") +
                                        "<button type=\"submit\" class=\"btn btn-primary\">Reset password</button> "
                                        "<a class=\"btn\" href=\"/admin\">Cancel</a>",
                                    "stack") +
                           "</section>";
        return page(c, "Reset Password", "admin", body);
    }
    return redirectWith(c, "/admin", Result::failure("No administrator found with that username."));
}

HttpResponse WebApp::adminReset(Context& c) {
    if (rawParam(c, "password") != rawParam(c, "confirm")) {
        return redirectWith(c, "/admin", Result::failure("The password and its confirmation do not match."));
    }
    return redirectWith(c, "/admin", admins_.resetPassword(param(c, "user"), rawParam(c, "password")));
}

HttpResponse WebApp::adminSettings(Context& c) {
    double rate = 0.0;
    long long days = 0;
    if (!util::parseDouble(param(c, "fine_rate"), rate) || !util::parseLong(param(c, "issue_days"), days)) {
        return redirectWith(c, "/admin", Result::failure("Please enter a valid fine rate and issue period."));
    }
    Result last = Result::success("Settings are unchanged.");
    std::string messages;
    bool ok = true;
    if (util::roundMoney(rate) != library_.fineRatePerDay()) {
        last = library_.setFineRate(rate);
        ok = ok && last.ok;
        messages += last.message + " ";
    }
    if (days != library_.issuePeriodDays()) {
        last = library_.setIssuePeriod(static_cast<int>(days < 0 ? 0 : (days > 1000 ? 1000 : days)));
        ok = ok && last.ok;
        messages += last.message;
    }
    if (messages.empty()) messages = last.message;
    return redirectWith(c, "/admin", ok ? Result::success(util::trim(messages)) : Result::failure(util::trim(messages)));
}
