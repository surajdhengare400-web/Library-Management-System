# Optional live smoke test (Python 3 standard library only). Start the server first:
#   ./LibraryWeb --port 8099 --data TestData_live      (use an EMPTY data folder)
# then run:  python3 tests/web_e2e.py
import http.client, re, urllib.parse, sys
HOST, PORT = "127.0.0.1", 8099
cookie = {}
fails = 0
def req(method, path, data=None, headers=None, follow=False):
    h = dict(headers or {})
    if cookie: h["Cookie"] = "; ".join(f"{k}={v}" for k, v in cookie.items())
    body = None
    if data is not None:
        body = urllib.parse.urlencode(data); h["Content-Type"] = "application/x-www-form-urlencoded"
    c = http.client.HTTPConnection(HOST, PORT, timeout=10)
    c.request(method, path, body, h); r = c.getresponse(); text = r.read().decode("utf-8", "replace")
    sc = r.getheader("Set-Cookie")
    if sc:
        k, _, v = sc.split(";")[0].partition("=")
        if v == "": cookie.pop(k, None)
        else: cookie[k] = v
    loc = r.getheader("Location")
    if follow and r.status in (301, 302, 303) and loc: return req("GET", loc)
    return r.status, text, loc
def check(name, cond):
    global fails
    print(("PASS " if cond else "FAIL ") + name)
    if not cond: fails += 1
def csrf():
    s, t, _ = req("GET", "/account"); return re.search(r'name="csrf" value="([0-9a-f]+)"', t).group(1)

PW = "Str0ng!Pass"
s, t, loc = req("GET", "/");                       check("no admin -> redirect /setup", s == 303 and loc == "/setup")
s, t, _ = req("GET", "/setup");                    check("setup page renders", s == 200 and "First-Time Setup" in t)
s, t, _ = req("POST", "/setup", dict(username="admin1", name="Test Admin", email="a@b.com", password=PW, confirm="x"))
check("setup: mismatch rejected", "do not match" in t)
s, t, _ = req("POST", "/setup", dict(username="admin1", name="Test Admin", email="a@b.com", password="weak", confirm="weak"))
check("setup: weak password rejected", "at least 8" in t)
s, t, loc = req("POST", "/setup", dict(username="admin1", name="Test Admin", email="a@b.com", password=PW, confirm=PW))
check("setup: super admin created", s == 303 and "created=1" in loc)
s, t, loc = req("GET", "/setup");                  check("setup closed afterwards", s == 303 and loc == "/login")
s, t, loc = req("GET", "/books");                  check("protected page -> /login", s == 303 and loc == "/login")
s, t, _ = req("POST", "/login", dict(username="admin1", password="bad")); check("bad login rejected", "Invalid username or password" in t and not cookie)
s, t, loc = req("POST", "/login", dict(username="admin1", password=PW)); check("login ok + cookie", s == 303 and "sid" in cookie)
s, t, _ = req("GET", "/", follow=False);           check("dashboard renders", s == 200 and "Dashboard" in t and "Book titles" in t)
tok = csrf()

s, t, _ = req("POST", "/books/add", dict(id="B001", title="x"));  check("POST without CSRF blocked (403)", s == 403)
s, t, _ = req("POST", "/books/add", dict(csrf="deadbeef", id="B001")); check("POST with wrong CSRF blocked", s == 403)
s, t, _ = req("POST", "/books/add", dict(id="B001", title="x"), headers={"Origin": "http://evil.example"}); check("cross-origin POST blocked", s == 403)

xss = "<script>alert(1)</script>"
s, t, loc = req("POST", "/books/add", dict(csrf=tok, id="B001", title=xss, author="Robert Martin", category="Programming", year="2008", copies="2"))
check("add book", s == 303)
s, t, _ = req("GET", "/books");  check("XSS title is escaped", "&lt;script&gt;" in t and "<script>" not in t)
s, t, _ = req("POST", "/books/add", dict(csrf=tok, id="B001", title="Dup", author="A", category="C", year="2000", copies="1")); check("duplicate book id rejected, form kept", "already exists" in t and 'value="Dup"' in t)
s, t, _ = req("POST", "/books/add", dict(csrf=tok, id="B002", title="T", author="A", category="C", year="abcd", copies="1")); check("bad year rejected", "valid year" in t)
s, t, _ = req("POST", "/books/add", dict(csrf=tok, id="B002", title="Algorithms", author="Sedgewick", category="CS", year="2011", copies="1")); check("add second book", s == 303)
s, t, _ = req("GET", "/books?field=author&q=sedge"); check("search by author", "Algorithms" in t and "Robert" not in t)
s, t, _ = req("GET", "/books?field=id&q=b001");      check("search by id", "B001" in t)
s, t, _ = req("GET", "/books/edit?id=B002");         check("edit form", s == 200 and "Save changes" in t)
s, t, _ = req("POST", "/books/update", dict(csrf=tok, id="B002", title="Algorithms 4e", author="Sedgewick", category="CS", year="2011", copies="3")); check("update book", s == 303)

s, t, _ = req("POST", "/members/add", dict(csrf=tok, id="M001", name="Asha Verma", email="asha@example.com", phone="9876543210")); check("register member", s == 303)
s, t, _ = req("POST", "/members/add", dict(csrf=tok, id="M002", name="Ravi", email="asha@example.com", phone="9876543211")); check("duplicate email rejected", "already registered" in t)
s, t, _ = req("GET", "/members?q=asha"); check("search member", "Asha Verma" in t)

s, t, _ = req("POST", "/issues/new", dict(csrf=tok, member="M001", book="B001")); check("issue book", s == 303)
s, t, _ = req("POST", "/issues/new", dict(csrf=tok, member="M001", book="B001")); check("duplicate active issue rejected", "already has" in t)
s, t, _ = req("POST", "/issues/new", dict(csrf=tok, member="NOPE1", book="B001")); check("invalid member rejected", "No member found" in t)
s, t, _ = req("GET", "/issues");  check("active issues list with Return button", "I0001" in t and "Return" in t)
s, t, _ = req("POST", "/books/delete", dict(csrf=tok, id="B001")); s2, t2, _ = req("GET", "/books"); check("delete blocked while issued", "B001" in t2)
s, t, _ = req("GET", "/books/delete?id=B001"); check("delete confirm page warns", "cannot be deleted" in t)
s, t, _ = req("POST", "/issues/return", dict(csrf=tok, issue="1")); check("return book", s == 303)
s, t, _ = req("GET", "/issues?view=completed"); check("completed issues list", "Returned" in t)
s, t, _ = req("POST", "/issues/return", dict(csrf=tok, issue="1")); s2, t2, _ = req("GET", "/issues"); check("double return flagged", "already returned" in t2)
s, t, _ = req("GET", "/members/view?id=M001"); check("member issue history", "Issue history" in t and "I0001" in t)

for typ in ["inventory", "members", "issued", "returned", "overdue", "fines", "statistics"]:
    s, t, _ = req("GET", "/reports?type=" + typ); check("report " + typ, s == 200 and "Report:" in t)
s, t, _ = req("GET", "/history"); check("history shows activity", "BOOK_ISSUED" in t and "BOOK_RETURNED" in t)
s, t, _ = req("GET", "/history?type=BOOK_ADDED"); check("history filter", "2 record(s)" in t and "BOOK_ADDED" in t)

s, t, _ = req("GET", "/admin"); check("admin page (Super Admin)", s == 200 and "Create administrator" in t)
s, t, _ = req("POST", "/admin/create", dict(csrf=tok, username="clerk", name="Clerk One", email="c@example.com", role="admin", password="Cl3rk!Passw", confirm="Cl3rk!Passw")); check("create Admin", s == 303)
s, t, _ = req("POST", "/admin/settings", dict(csrf=tok, fine_rate="7.5", issue_days="10")); s2, t2, _ = req("GET", "/admin"); check("settings saved", 'value="7.50"' in t2 and 'value="10"' in t2)
s, t, _ = req("GET", "/admin/toggle?user=admin1"); s2, t2, _ = req("POST", "/admin/toggle", dict(csrf=tok, user="admin1", activate="0")); s3, t3, _ = req("GET", "/admin")
check("cannot deactivate yourself", "cannot deactivate your own" in t3)
s, t, _ = req("POST", "/account/password", dict(csrf=tok, current=PW, password="N3w!Passw0rd", confirm="N3w!Passw0rd")); s2, t2, _ = req("GET", "/account"); check("change password", "Password changed" in t2)

# logout, then log in as the normal Admin
s, t, loc = req("POST", "/logout", dict(csrf=tok)); check("logout", s == 303 and not cookie)
s, t, _ = req("GET", "/books"); check("session gone after logout", s == 303)
s, t, loc = req("POST", "/login", dict(username="clerk", password="Cl3rk!Passw")); check("login as normal admin", s == 303)
s, t, _ = req("GET", "/admin"); check("normal admin denied /admin (403)", s == 403)
s, t, _ = req("GET", "/books"); check("normal admin sees books", s == 200 and "Administration" not in t)
s, t, _ = req("GET", "/nope"); check("404 page", s == 404)
print("\nFAILED:", fails)
sys.exit(1 if fails else 0)
