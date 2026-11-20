// tests/test_web.cpp - tests the website layer in-process (no sockets): HTTP helpers, HTML escaping,
// login/session handling, CSRF protection, permissions and the main workflows.
#include <filesystem>
#include <iostream>

#include "Html.h"
#include "HttpServer.h"
#include "WebApp.h"

namespace fs = std::filesystem;

static int g_passed = 0, g_failed = 0;
#define CHECK(cond)                                                                      \
    do {                                                                                 \
        if (cond) { ++g_passed; }                                                        \
        else { ++g_failed; std::cout << "  FAIL (line " << __LINE__ << "): " #cond "\n"; } \
    } while (0)

namespace {

// A minimal browser: keeps the session cookie and the CSRF token of the last page.
struct Browser {
    WebApp& app;
    std::string sid;
    std::string csrf;

    HttpResponse send(const std::string& method, const std::string& target, std::map<std::string, std::string> form = {},
                      const std::string& origin = "") {
        HttpRequest r;
        r.method = method;
        const std::size_t q = target.find('?');
        r.path = target.substr(0, q);
        r.rawQuery = q == std::string::npos ? "" : target.substr(q + 1);
        r.query = http::parseUrlEncoded(r.rawQuery);
        r.headers["host"] = "127.0.0.1:8080";
        if (!origin.empty()) r.headers["origin"] = origin;
        if (!sid.empty()) r.cookies["sid"] = sid;
        if (method == "POST") {
            if (!form.count("csrf") && !csrf.empty()) form["csrf"] = csrf;
            r.form = form;
        }
        HttpResponse resp = app.handle(r);
        for (const auto& h : resp.headers) {
            if (h.first == "Set-Cookie") {
                const std::string value = h.second.substr(0, h.second.find(';'));
                const std::string token = value.substr(4);
                sid = token;
            }
        }
        const std::string marker = "name=\"csrf\" value=\"";
        const std::size_t at = resp.body.find(marker);
        if (at != std::string::npos) {
            const std::size_t start = at + marker.size();
            csrf = resp.body.substr(start, resp.body.find('"', start) - start);
        }
        return resp;
    }
    std::string location(const HttpResponse& r) const {
        for (const auto& h : r.headers) if (h.first == "Location") return h.second;
        return "";
    }
};

}  // namespace

int main() {
    // ---- helpers ----
    CHECK(http::urlDecode("a%20b+c%2F", true) == "a b c/");
    CHECK(http::urlDecode("100%", true) == "100%");
    CHECK(http::urlEncode("a b&c=d/é") == "a%20b%26c%3Dd%2F%C3%A9");
    CHECK(http::parseUrlEncoded("x=1&y=hello+world&z=%26").at("y") == "hello world");
    CHECK(http::parseUrlEncoded("x=1&y=hello+world&z=%26").at("z") == "&");
    CHECK(http::parseCookies("a=1; sid=abc; b=2").at("sid") == "abc");
    HttpRequest parsed;
    CHECK(http::parseRequestHead("GET /books?q=a%20b HTTP/1.1\r\nHost: x\r\nCookie: sid=1\r\nX-Test:  v ", parsed));
    CHECK(parsed.path == "/books" && parsed.query.at("q") == "a b" && parsed.header("host") == "x" && parsed.cookies.at("sid") == "1");
    HttpRequest bad;
    CHECK(!http::parseRequestHead("NONSENSE", bad) && !http::parseRequestHead("GET nopath HTTP/1.1", bad));
    CHECK(html::esc("<a href=\"x\">&'</a>") == "&lt;a href=&quot;x&quot;&gt;&amp;&#39;&lt;/a&gt;");

    // ---- application ----
    const std::string dir = "TestData_web";
    fs::remove_all(dir);
    WebApp app(dir, "public");
    std::vector<std::string> warnings;
    std::string fatal;
    CHECK(app.init(warnings, fatal));
    Browser b{app, "", ""};
    const std::string pw = "Str0ng!Pass";

    HttpResponse r = b.send("GET", "/");
    CHECK(r.status == 303 && b.location(r) == "/setup");
    CHECK(b.send("GET", "/setup").status == 200);
    r = b.send("POST", "/setup", {{"username", "root"}, {"name", "Root User"}, {"email", "r@example.com"}, {"password", pw}, {"confirm", "different"}});
    CHECK(r.status == 200 && r.body.find("do not match") != std::string::npos);
    r = b.send("POST", "/setup", {{"username", "root"}, {"name", "Root User"}, {"email", "r@example.com"}, {"password", pw}, {"confirm", pw}});
    CHECK(r.status == 303);
    CHECK(b.send("GET", "/books").status == 303);                                   // must log in
    r = b.send("POST", "/login", {{"username", "root"}, {"password", "wrong"}});
    CHECK(r.status == 200 && b.sid.empty() && r.body.find("Invalid username or password") != std::string::npos);
    r = b.send("POST", "/login", {{"username", "root"}, {"password", pw}});
    CHECK(r.status == 303 && !b.sid.empty() && b.sid.size() == 64);
    r = b.send("GET", "/");
    CHECK(r.status == 200 && r.body.find("Dashboard") != std::string::npos && !b.csrf.empty());

    // CSRF and origin protection
    const std::string goodCsrf = b.csrf;
    b.csrf = "";
    CHECK(b.send("POST", "/books/add", {{"id", "B001"}}).status == 403);
    CHECK(b.send("POST", "/books/add", {{"id", "B001"}, {"csrf", "0000"}}).status == 403);
    b.csrf = goodCsrf;
    CHECK(b.send("POST", "/books/add", {{"id", "B001"}}, "http://evil.example").status == 403);

    // books
    r = b.send("POST", "/books/add", {{"id", "B001"}, {"title", "<b>Bold & \"Quoted\"</b>"}, {"author", "Robert Martin"}, {"category", "Programming"}, {"year", "2008"}, {"copies", "2"}});
    CHECK(r.status == 303);
    r = b.send("GET", "/books");
    CHECK(r.body.find("&lt;b&gt;Bold &amp; &quot;Quoted&quot;&lt;/b&gt;") != std::string::npos && r.body.find("<b>Bold") == std::string::npos);
    r = b.send("POST", "/books/add", {{"id", "B001"}, {"title", "Dup"}, {"author", "A"}, {"category", "C"}, {"year", "2000"}, {"copies", "1"}});
    CHECK(r.status == 200 && r.body.find("already exists") != std::string::npos);
    r = b.send("POST", "/books/add", {{"id", "B002"}, {"title", "T"}, {"author", "A"}, {"category", "C"}, {"year", "2000"}, {"copies", "-4"}});
    CHECK(r.status == 200 && r.body.find("Add book") != std::string::npos);
    CHECK(b.send("GET", "/books?field=author&q=martin").body.find("Robert Martin") != std::string::npos);

    // members and issues
    CHECK(b.send("POST", "/members/add", {{"id", "M001"}, {"name", "Asha Verma"}, {"email", "asha@example.com"}, {"phone", "9876543210"}}).status == 303);
    CHECK(b.send("POST", "/issues/new", {{"member", "M001"}, {"book", "B001"}}).status == 303);
    r = b.send("POST", "/issues/new", {{"member", "M001"}, {"book", "B001"}});
    CHECK(r.body.find("already has") != std::string::npos);
    CHECK(b.send("GET", "/issues").body.find("I0001") != std::string::npos);
    r = b.send("GET", "/books/delete?id=B001");
    CHECK(r.body.find("cannot be deleted") != std::string::npos);
    CHECK(b.send("POST", "/issues/return", {{"issue", "1"}}).status == 303);
    CHECK(b.send("GET", "/issues?view=completed").body.find("Returned") != std::string::npos);
    for (const char* type : {"inventory", "members", "issued", "returned", "overdue", "fines", "statistics"}) {
        CHECK(b.send("GET", std::string("/reports?type=") + type).status == 200);
    }
    CHECK(b.send("GET", "/history").body.find("BOOK_RETURNED") != std::string::npos);

    // administration and permissions
    CHECK(b.send("GET", "/admin").status == 200);
    CHECK(b.send("POST", "/admin/create", {{"username", "clerk"}, {"name", "Clerk One"}, {"email", "c@example.com"}, {"role", "admin"}, {"password", "Cl3rk!Passw"}, {"confirm", "Cl3rk!Passw"}}).status == 303);
    r = b.send("GET", "/admin");
    CHECK(r.body.find("clerk") != std::string::npos);
    b.send("POST", "/admin/toggle", {{"user", "root"}, {"activate", "0"}});
    CHECK(b.send("GET", "/admin").body.find("cannot deactivate your own") != std::string::npos);

    r = b.send("POST", "/logout");
    CHECK(r.status == 303 && b.sid.empty());
    CHECK(b.send("GET", "/books").status == 303);

    Browser clerk{app, "", ""};
    CHECK(clerk.send("POST", "/login", {{"username", "clerk"}, {"password", "Cl3rk!Passw"}}).status == 303);
    CHECK(clerk.send("GET", "/books").status == 200);
    CHECK(clerk.send("GET", "/admin").status == 403);
    CHECK(clerk.send("GET", "/admin/reset?user=root").status == 403);
    clerk.send("GET", "/");
    CHECK(clerk.send("POST", "/admin/create", {{"username", "x1"}, {"name", "X One"}, {"email", "x@example.com"}, {"role", "super"}, {"password", "Xx1!Passw0rd"}, {"confirm", "Xx1!Passw0rd"}}).status == 403);
    CHECK(clerk.send("GET", "/missing").status == 404);

    // a deactivated account loses its session immediately
    Browser boss{app, "", ""};
    boss.send("POST", "/login", {{"username", "root"}, {"password", pw}});
    boss.send("GET", "/");
    boss.send("POST", "/admin/toggle", {{"user", "clerk"}, {"activate", "0"}});
    CHECK(clerk.send("GET", "/books").status == 303);

    // lock-out after repeated failures
    Browser attacker{app, "", ""};
    for (int i = 0; i < 5; ++i) attacker.send("POST", "/login", {{"username", "root"}, {"password", "nope"}});
    CHECK(attacker.send("POST", "/login", {{"username", "root"}, {"password", pw}}).status == 429);

    fs::remove_all(dir);
    std::cout << "Passed: " << g_passed << "   Failed: " << g_failed << '\n';
    return g_failed == 0 ? 0 : 1;
}
