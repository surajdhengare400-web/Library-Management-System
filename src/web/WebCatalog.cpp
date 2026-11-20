// WebCatalog.cpp - website pages for books, members, issues, reports and history.
#include <algorithm>
#include <map>

#include "Html.h"
#include "Utils.h"
#include "WebApp.h"

namespace {

std::string linkTo(const std::string& path, const std::string& key, const std::string& value, const std::string& text,
                   const std::string& cssClass = "") {
    return "<a" + (cssClass.empty() ? std::string() : " class=\"" + cssClass + "\"") + " href=\"" + path + "?" + key + "=" +
           http::urlEncode(value) + "\">" + html::esc(text) + "</a>";
}

std::string optionTag(const std::string& value, const std::string& label, bool selected) {
    return "<option value=\"" + html::esc(value) + "\"" + (selected ? " selected" : "") + ">" + html::esc(label) + "</option>";
}

std::string tab(const std::string& view, const std::string& current, const std::string& label) {
    return "<a class=\"tab" + std::string(view == current ? " active" : "") + "\" href=\"/issues?view=" + view + "\">" +
           html::esc(label) + "</a>";
}

std::string bookRowActions(const Book& b) {
    return linkTo("/books/edit", "id", b.id(), "Edit", "btn btn-small") + " " +
           linkTo("/books/delete", "id", b.id(), "Delete", "btn btn-small btn-danger-outline");
}

std::string money(double v) { return html::esc(util::formatMoney(v)); }

}  // namespace

// ------------------------------------------------------------------------------------------
// Books
// ------------------------------------------------------------------------------------------

HttpResponse WebApp::booksList(Context& c) {
    const std::string field = param(c, "field").empty() ? "title" : param(c, "field");
    const std::string query = param(c, "q");
    std::vector<Book> books;
    if (query.empty()) {
        books = library_.books();
    } else if (field == "id") {
        if (const auto book = library_.findBook(query)) books.push_back(*book);
    } else {
        const BookField f = field == "author" ? BookField::Author : field == "category" ? BookField::Category : BookField::Title;
        books = library_.searchBooks(f, query);
    }

    std::string body = "<form method=\"get\" action=\"/books\" class=\"toolbar\">"
                       "<select name=\"field\" aria-label=\"Search by\">" +
                       optionTag("title", "Title", field == "title") + optionTag("author", "Author", field == "author") +
                       optionTag("category", "Category", field == "category") + optionTag("id", "Book ID", field == "id") +
                       "</select><input type=\"search\" name=\"q\" placeholder=\"Search books...\" value=\"" + html::esc(query) +
                       "\"><button type=\"submit\" class=\"btn btn-primary\">Search</button>"
                       "<a class=\"btn\" href=\"/books\">Show all</a><a class=\"btn btn-accent\" href=\"/books/new\">+ Add book</a></form>";
    std::vector<std::vector<std::string>> rows;
    for (const Book& b : books) {
        rows.push_back({html::esc(b.id()), html::esc(b.title()), html::esc(b.author()), html::esc(b.category()),
                        std::to_string(b.year()), std::to_string(b.totalCopies()),
                        b.availableCopies() == 0 ? html::badge("0 - none left", "danger") : html::badge(std::to_string(b.availableCopies()), "ok"),
                        bookRowActions(b)});
    }
    body += html::table({"ID", "Title", "Author", "Category", "Year", "Copies", "Available", "Actions"}, rows,
                        query.empty() ? "The catalogue is empty. Add your first book." : "No books matched your search.");
    return page(c, "Books", "books", body);
}

std::string WebApp::bookForm(Context& c, bool editing, const FormValues& values) {
    auto v = [&](const char* key) {
        const auto it = values.find(key);
        return it == values.end() ? std::string() : it->second;
    };
    std::string inner;
    if (editing) {
        inner += html::hidden("id", v("id")) + "<p><strong>Book ID:</strong> " + html::esc(v("id")) + "</p>";
    } else {
        inner += html::field("Book ID (3-12 letters, digits, -)", "id", v("id"), "text", "required maxlength=\"12\"");
    }
    inner += html::field("Title", "title", v("title"), "text", "required maxlength=\"120\"") +
             html::field("Author", "author", v("author"), "text", "required maxlength=\"80\"") +
             html::field("Category", "category", v("category"), "text", "required maxlength=\"40\"") +
             html::field("Publication year", "year", v("year"), "number", "required min=\"1000\" max=\"9999\"") +
             html::field(editing ? "Total copies" : "Number of copies", editing ? "copies" : "copies", v("copies"), "number",
                         "required min=\"1\" max=\"" + std::to_string(Library::kMaxCopiesPerBook) + "\"") +
             "<div class=\"toolbar\"><button type=\"submit\" class=\"btn btn-primary\">" + std::string(editing ? "Save changes" : "Add book") +
             "</button><a class=\"btn\" href=\"/books\">Cancel</a></div>";
    return "<section class=\"panel narrow\">" + postForm(c, editing ? "/books/update" : "/books/add", inner, "stack") + "</section>";
}

HttpResponse WebApp::bookNew(Context& c) { return page(c, "Add Book", "books", bookForm(c, false, {})); }

namespace {
// Reads year and copies from the form; returns an error text (empty = ok).
std::string readBookNumbers(const std::string& yearText, const std::string& copiesText, int& year, int& copies) {
    long long y = 0, n = 0;
    if (!util::parseLong(yearText, y) || y < 0 || y > 9999) return "Publication year must be a valid year.";
    if (!util::parseLong(copiesText, n) || n < 0 || n > 100000) return "Number of copies must be a whole number.";
    year = static_cast<int>(y);
    copies = static_cast<int>(n);
    return "";
}
}  // namespace

HttpResponse WebApp::bookAdd(Context& c) {
    const FormValues values{{"id", param(c, "id")}, {"title", param(c, "title")}, {"author", param(c, "author")},
                            {"category", param(c, "category")}, {"year", param(c, "year")}, {"copies", param(c, "copies")}};
    int year = 0, copies = 0;
    std::string error = readBookNumbers(values.at("year"), values.at("copies"), year, copies);
    if (error.empty()) {
        const Result r = library_.addBook(values.at("id"), values.at("title"), values.at("author"), values.at("category"), year, copies);
        if (r.ok) return redirectWith(c, "/books", r);
        error = r.message;
    }
    flash(c, error, "error");
    return page(c, "Add Book", "books", bookForm(c, false, values));
}

HttpResponse WebApp::bookEdit(Context& c) {
    const auto book = library_.findBook(param(c, "id"));
    if (!book) return redirectWith(c, "/books", Result::failure("No book found with that ID."));
    const FormValues values{{"id", book->id()}, {"title", book->title()}, {"author", book->author()},
                            {"category", book->category()}, {"year", std::to_string(book->year())},
                            {"copies", std::to_string(book->totalCopies())}};
    std::string note = book->issuedCopies() > 0
                           ? "<p class=\"muted\">" + std::to_string(book->issuedCopies()) + " copy(ies) are currently issued; total copies cannot go below that.</p>"
                           : "";
    return page(c, "Edit Book", "books", note + bookForm(c, true, values));
}

HttpResponse WebApp::bookUpdate(Context& c) {
    const FormValues values{{"id", param(c, "id")}, {"title", param(c, "title")}, {"author", param(c, "author")},
                            {"category", param(c, "category")}, {"year", param(c, "year")}, {"copies", param(c, "copies")}};
    int year = 0, copies = 0;
    std::string error = readBookNumbers(values.at("year"), values.at("copies"), year, copies);
    if (error.empty()) {
        const Result r = library_.updateBook(values.at("id"), values.at("title"), values.at("author"), values.at("category"), year, copies);
        if (r.ok) return redirectWith(c, "/books", r);
        error = r.message;
    }
    flash(c, error, "error");
    return page(c, "Edit Book", "books", bookForm(c, true, values));
}

HttpResponse WebApp::bookDeleteConfirm(Context& c) {
    const auto book = library_.findBook(param(c, "id"));
    if (!book) return redirectWith(c, "/books", Result::failure("No book found with that ID."));
    std::string body = "<section class=\"panel narrow\"><h2>Delete this book?</h2><p><strong>" + html::esc(book->title()) + "</strong> by " +
                       html::esc(book->author()) + " (" + html::esc(book->id()) + ")</p>";
    if (book->issuedCopies() > 0) {
        body += "<div class=\"alert alert-error\">It cannot be deleted while " + std::to_string(book->issuedCopies()) +
                " copy(ies) are issued.</div><a class=\"btn\" href=\"/books\">Back</a>";
    } else {
        body += "<p class=\"muted\">This action is permanent.</p>" +
                postForm(c, "/books/delete", html::hidden("id", book->id()) +
                                                 "<button type=\"submit\" class=\"btn btn-danger\">Yes, delete</button> <a class=\"btn\" href=\"/books\">Cancel</a>");
    }
    body += "</section>";
    return page(c, "Delete Book", "books", body);
}

HttpResponse WebApp::bookDelete(Context& c) { return redirectWith(c, "/books", library_.deleteBook(param(c, "id"))); }

// ------------------------------------------------------------------------------------------
// Members
// ------------------------------------------------------------------------------------------

HttpResponse WebApp::membersList(Context& c) {
    const std::string query = param(c, "q");
    const std::vector<Member> members = query.empty() ? library_.members() : library_.searchMembers(query);
    std::string body = "<form method=\"get\" action=\"/members\" class=\"toolbar\"><input type=\"search\" name=\"q\" "
                       "placeholder=\"Search by ID, name, email or phone...\" value=\"" + html::esc(query) +
                       "\"><button type=\"submit\" class=\"btn btn-primary\">Search</button><a class=\"btn\" href=\"/members\">Show all</a>"
                       "<a class=\"btn btn-accent\" href=\"/members/new\">+ Register member</a></form>";
    std::vector<std::vector<std::string>> rows;
    for (const Member& m : members) {
        rows.push_back({linkTo("/members/view", "id", m.id(), m.id()), html::esc(m.fullName()), html::esc(m.email()), html::esc(m.phone()),
                        html::esc(m.registrationDate().toString()),
                        linkTo("/members/view", "id", m.id(), "Issues", "btn btn-small") + " " +
                            linkTo("/members/edit", "id", m.id(), "Edit", "btn btn-small")});
    }
    body += html::table({"Member ID", "Name", "Email", "Phone", "Registered", "Actions"}, rows,
                        query.empty() ? "No members are registered yet." : "No members matched your search.");
    return page(c, "Members", "members", body);
}

std::string WebApp::memberForm(Context& c, bool editing, const FormValues& values) {
    auto v = [&](const char* key) {
        const auto it = values.find(key);
        return it == values.end() ? std::string() : it->second;
    };
    std::string inner = editing ? html::hidden("id", v("id")) + "<p><strong>Member ID:</strong> " + html::esc(v("id")) + "</p>"
                                : html::field("Member ID (3-12 letters, digits, -)", "id", v("id"), "text", "required maxlength=\"12\"");
    inner += html::field("Full name", "name", v("name"), "text", "required maxlength=\"60\"") +
             html::field("Email", "email", v("email"), "email", "required maxlength=\"100\"") +
             html::field("Phone number", "phone", v("phone"), "tel", "required maxlength=\"20\"") +
             "<div class=\"toolbar\"><button type=\"submit\" class=\"btn btn-primary\">" + std::string(editing ? "Save changes" : "Register member") +
             "</button><a class=\"btn\" href=\"/members\">Cancel</a></div>";
    return "<section class=\"panel narrow\">" + postForm(c, editing ? "/members/update" : "/members/add", inner, "stack") + "</section>";
}

HttpResponse WebApp::memberNew(Context& c) { return page(c, "Register Member", "members", memberForm(c, false, {})); }

HttpResponse WebApp::memberAdd(Context& c) {
    const FormValues values{{"id", param(c, "id")}, {"name", param(c, "name")}, {"email", param(c, "email")}, {"phone", param(c, "phone")}};
    const Result r = library_.registerMember(values.at("id"), values.at("name"), values.at("email"), values.at("phone"), Date::today());
    if (r.ok) return redirectWith(c, "/members", r);
    flash(c, r.message, "error");
    return page(c, "Register Member", "members", memberForm(c, false, values));
}

HttpResponse WebApp::memberEdit(Context& c) {
    const auto m = library_.findMember(param(c, "id"));
    if (!m) return redirectWith(c, "/members", Result::failure("No member found with that ID."));
    return page(c, "Edit Member", "members",
                memberForm(c, true, {{"id", m->id()}, {"name", m->fullName()}, {"email", m->email()}, {"phone", m->phone()}}));
}

HttpResponse WebApp::memberUpdate(Context& c) {
    const FormValues values{{"id", param(c, "id")}, {"name", param(c, "name")}, {"email", param(c, "email")}, {"phone", param(c, "phone")}};
    const Result r = library_.updateMember(values.at("id"), values.at("name"), values.at("email"), values.at("phone"));
    if (r.ok) return redirectWith(c, "/members", r);
    flash(c, r.message, "error");
    return page(c, "Edit Member", "members", memberForm(c, true, values));
}

HttpResponse WebApp::memberView(Context& c) {
    const auto m = library_.findMember(param(c, "id"));
    if (!m) return redirectWith(c, "/members", Result::failure("No member found with that ID."));
    const Date today = Date::today();
    std::string body = "<section class=\"panel\"><h2>" + html::esc(m->fullName()) + "</h2>" +
                       html::table({"Member ID", "Email", "Phone", "Registered"},
                                   {{html::esc(m->id()), html::esc(m->email()), html::esc(m->phone()), html::esc(m->registrationDate().toString())}}, "") +
                       "<div class=\"toolbar\"><a class=\"btn\" href=\"/members\">Back</a>" + linkTo("/members/edit", "id", m->id(), "Edit member", "btn") +
                       linkTo("/issues/new", "member", m->id(), "Issue a book", "btn btn-primary") + "</div></section>";
    body += "<section class=\"panel\"><h2>Issue history</h2>" + issuesTable(c, library_.issuesForMember(m->id()), today, true, true) + "</section>";
    return page(c, "Member Details", "members", body);
}

// ------------------------------------------------------------------------------------------
// Issues
// ------------------------------------------------------------------------------------------

std::string WebApp::issuesTable(Context& c, const std::vector<IssueRecord>& issues, const Date& asOf, bool showReturned, bool actions) {
    std::vector<std::string> headers = {"Issue", "Book", "Member", "Issued", "Due"};
    if (showReturned) headers.push_back("Returned");
    headers.push_back("Status");
    headers.push_back("Fine");
    if (actions) headers.push_back("Action");

    std::vector<std::vector<std::string>> rows;
    for (const IssueRecord& l : issues) {
        std::string status, fine = "-", action;
        if (l.isReturned()) {
            status = l.overdueDays(asOf) > 0 ? html::badge("Returned late", "warn") : html::badge("Returned", "ok");
            if (l.fineAmount() > 0.0) {
                fine = money(l.fineAmount()) + " " + (l.isFinePaid() ? html::badge("paid", "ok") : html::badge("unpaid", "danger"));
            }
            if (l.hasUnpaidFine()) {
                action = postForm(c, "/issues/payfine", html::hidden("issue", std::to_string(l.id())) +
                                                              "<button type=\"submit\" class=\"btn btn-small\">Mark fine paid</button>", "inline");
            }
        } else {
            if (l.isOverdue(asOf)) {
                status = html::badge("Overdue " + std::to_string(l.overdueDays(asOf)) + " day(s)", "danger");
                fine = money(library_.fineFor(l, asOf)) + " " + html::badge("accruing", "warn");
            } else {
                const long long left = l.dueDate().daysAfter(asOf);
                status = left == 0 ? html::badge("Due today", "warn") : html::badge("Due in " + std::to_string(left) + " day(s)", "ok");
            }
            action = postForm(c, "/issues/return", html::hidden("issue", std::to_string(l.id())) +
                                                      "<button type=\"submit\" class=\"btn btn-small btn-primary\">Return</button>", "inline");
        }
        std::vector<std::string> row = {html::esc(util::formatId('I', l.id())),
                                        html::esc(l.bookId()) + " &middot; " + html::esc(library_.bookTitle(l.bookId())),
                                        linkTo("/members/view", "id", l.memberId(), l.memberId()) + " &middot; " + html::esc(library_.memberName(l.memberId())),
                                        html::esc(l.issueDate().toString()), html::esc(l.dueDate().toString())};
        if (showReturned) row.push_back(l.isReturned() ? html::esc(l.returnDate().toString()) : "-");
        row.push_back(status);
        row.push_back(fine);
        if (actions) row.push_back(action.empty() ? "" : action);
        rows.push_back(std::move(row));
    }
    return html::table(headers, rows, "No issues to show.");
}

HttpResponse WebApp::issuesList(Context& c) {
    const Date today = Date::today();
    const std::string view = param(c, "view").empty() ? "active" : param(c, "view");
    std::vector<IssueRecord> issues;
    bool returned = false;
    if (view == "completed") { issues = library_.completedIssues(); returned = true; }
    else if (view == "overdue") issues = library_.overdueIssues(today);
    else if (view == "fines") { issues = library_.unpaidFineIssues(); returned = true; }
    else issues = library_.activeIssues();

    std::string body = "<div class=\"toolbar\"><a class=\"btn btn-accent\" href=\"/issues/new\">+ Issue a book</a></div><div class=\"tabs\">" +
                       tab("active", view, "Active") + tab("overdue", view, "Overdue") + tab("completed", view, "Completed") +
                       tab("fines", view, "Unpaid fines") + "</div>";
    body += "<p class=\"muted\">Issue period: " + std::to_string(library_.issuePeriodDays()) + " days. Fine: " + html::esc(util::formatMoney(library_.fineRatePerDay())) +
            " per overdue day.</p>";
    body += issuesTable(c, issues, today, returned, true);
    return page(c, "Issue & Return", "issues", body);
}

HttpResponse WebApp::issueNewForm(Context& c) {
    std::string members, books;
    for (const Member& m : library_.members()) members += "<option value=\"" + html::esc(m.id()) + "\">" + html::esc(m.fullName()) + "</option>";
    for (const Book& b : library_.books()) {
        if (b.hasAvailableCopy()) books += "<option value=\"" + html::esc(b.id()) + "\">" + html::esc(b.title()) + " (" + std::to_string(b.availableCopies()) + " available)</option>";
    }
    std::string inner = "<datalist id=\"member-list\">" + members + "</datalist><datalist id=\"book-list\">" + books + "</datalist>" +
                        html::field("Member ID", "member", param(c, "member"), "text", "list=\"member-list\" required autocomplete=\"off\"") +
                        html::field("Book ID", "book", param(c, "book"), "text", "list=\"book-list\" required autocomplete=\"off\"") +
                        "<p class=\"hint\">Start typing to see matching members and available books. Issue period: " +
                        std::to_string(library_.issuePeriodDays()) + " days.</p>"
                        "<div class=\"toolbar\"><button type=\"submit\" class=\"btn btn-primary\">Issue book</button><a class=\"btn\" href=\"/issues\">Cancel</a></div>";
    return page(c, "Issue Book", "issues", "<section class=\"panel narrow\">" + postForm(c, "/issues/new", inner, "stack") + "</section>");
}

HttpResponse WebApp::issueNewSubmit(Context& c) {
    const Result r = library_.issueBook(param(c, "book"), param(c, "member"), Date::today());
    if (r.ok) return redirectWith(c, "/issues", r);
    flash(c, r.message, "error");
    return issueNewForm(c);
}

HttpResponse WebApp::issueReturn(Context& c) {
    long long id = 0;
    if (!util::parseLong(param(c, "issue"), id) || id < 1 || id > 99999999) {
        return redirectWith(c, "/issues", Result::failure("Invalid issue number."));
    }
    const Result r = library_.returnBook(static_cast<int>(id), Date::today());
    const auto issue = library_.findIssue(static_cast<int>(id));
    flash(c, r.message + (r.ok && issue && issue->hasUnpaidFine() ? " Use 'Mark fine paid' once the fine is collected." : ""), r.ok ? "success" : "error");
    return HttpResponse::redirect(r.ok && issue && issue->hasUnpaidFine() ? "/issues?view=fines" : "/issues");
}

HttpResponse WebApp::issuePayFine(Context& c) {
    long long id = 0;
    if (!util::parseLong(param(c, "issue"), id) || id < 1 || id > 99999999) {
        return redirectWith(c, "/issues?view=fines", Result::failure("Invalid issue number."));
    }
    return redirectWith(c, "/issues?view=fines", library_.payFine(static_cast<int>(id)));
}

// ------------------------------------------------------------------------------------------
// Reports and history
// ------------------------------------------------------------------------------------------

HttpResponse WebApp::reportsPage(Context& c) {
    const Date today = Date::today();
    const std::string type = param(c, "type");
    struct Item { const char* key; const char* label; };
    const Item items[] = {{"inventory", "Book inventory"}, {"members", "Member records"}, {"issued", "Currently issued books"},
                          {"returned", "Returned books"}, {"overdue", "Overdue books"}, {"fines", "Fine records"},
                          {"statistics", "Library statistics"}, {"history", "Transaction history"}};
    std::string body = "<div class=\"tabs no-print\">";
    for (const Item& it : items) {
        body += "<a class=\"tab" + std::string(type == it.key ? " active" : "") + "\" href=\"/reports?type=" + it.key + "\">" + it.label + "</a>";
    }
    body += "</div>";
    std::string title = "Reports";
    if (type.empty()) {
        body += "<p class=\"muted\">Choose a report above. Reports can be printed from your browser (Ctrl+P).</p>";
    } else if (type == "inventory") {
        title = "Report: Book Inventory";
        std::vector<std::vector<std::string>> rows;
        for (const Book& b : library_.books()) {
            rows.push_back({html::esc(b.id()), html::esc(b.title()), html::esc(b.author()), html::esc(b.category()), std::to_string(b.year()),
                            std::to_string(b.totalCopies()), std::to_string(b.availableCopies()), std::to_string(b.issuedCopies())});
        }
        body += html::table({"ID", "Title", "Author", "Category", "Year", "Total", "Available", "Issued"}, rows, "The catalogue is empty.");
        body += "<p class=\"muted\">Total copies " + std::to_string(library_.totalCopies()) + " &middot; available " + std::to_string(library_.availableCopies()) +
                " &middot; issued " + std::to_string(library_.issuedCopies()) + "</p>";
    } else if (type == "members") {
        title = "Report: Member Records";
        std::vector<std::vector<std::string>> rows;
        for (const Member& m : library_.members()) {
            rows.push_back({html::esc(m.id()), html::esc(m.fullName()), html::esc(m.email()), html::esc(m.phone()), html::esc(m.registrationDate().toString()),
                            std::to_string(library_.activeIssueCountForMember(m.id()))});
        }
        body += html::table({"Member ID", "Name", "Email", "Phone", "Registered", "Active issues"}, rows, "No members are registered.");
    } else if (type == "issued") {
        title = "Report: Currently Issued Books";
        body += issuesTable(c, library_.activeIssues(), today, false, false);
    } else if (type == "returned") {
        title = "Report: Returned Books";
        body += issuesTable(c, library_.completedIssues(), today, true, false);
    } else if (type == "overdue") {
        title = "Report: Overdue Books";
        body += issuesTable(c, library_.overdueIssues(today), today, false, false);
        body += "<p class=\"muted\">Accruing fines: " + money(library_.accruingFines(today)) + "</p>";
    } else if (type == "fines") {
        title = "Report: Fine Records";
        std::vector<std::vector<std::string>> rows;
        for (const IssueRecord& l : library_.issues()) {
            const double fine = library_.fineFor(l, today);
            if (fine <= 0.0) continue;
            rows.push_back({html::esc(util::formatId('I', l.id())), html::esc(l.memberId()) + " &middot; " + html::esc(library_.memberName(l.memberId())),
                            html::esc(library_.bookTitle(l.bookId())), std::to_string(l.overdueDays(today)), money(fine),
                            l.isActive() ? html::badge("Accruing", "warn") : (l.isFinePaid() ? html::badge("Paid", "ok") : html::badge("Unpaid", "danger"))});
        }
        body += html::table({"Issue", "Member", "Book", "Days late", "Fine", "Status"}, rows, "No fines have been charged.");
        body += "<p class=\"muted\">Collected " + money(library_.collectedFines()) + " &middot; unpaid " + money(library_.unpaidRecordedFines()) +
                " &middot; accruing " + money(library_.accruingFines(today)) + " &middot; outstanding " + money(library_.outstandingFines(today)) + "</p>";
    } else if (type == "statistics") {
        title = "Report: Library Statistics";
        std::size_t supers = 0;
        for (const Admin& a : admins_.admins()) if (a.isSuperAdmin()) ++supers;
        const int total = library_.totalCopies();
        const int utilisation = total > 0 ? (library_.issuedCopies() * 100 + total / 2) / total : 0;
        body += html::table({"Metric", "Value"},
                            {{"Book titles", std::to_string(library_.books().size())}, {"Total copies", std::to_string(total)},
                             {"Available copies", std::to_string(library_.availableCopies())}, {"Issued copies", std::to_string(library_.issuedCopies())},
                             {"Collection utilisation", std::to_string(utilisation) + "%"}, {"Registered members", std::to_string(library_.members().size())},
                             {"Active issues", std::to_string(library_.activeIssues().size())}, {"Completed issues", std::to_string(library_.completedIssues().size())},
                             {"Overdue issues", std::to_string(library_.overdueIssues(today).size())},
                             {"Administrators (total / Super)", std::to_string(admins_.admins().size()) + " / " + std::to_string(supers)},
                             {"Fine per day", money(library_.fineRatePerDay())}, {"Issue period", std::to_string(library_.issuePeriodDays()) + " days"},
                             {"Fines collected", money(library_.collectedFines())}, {"Fines outstanding", money(library_.outstandingFines(today))},
                             {"Transactions recorded", std::to_string(library_.transactions().size())}}, "");
        std::map<std::string, std::pair<int, int>> byCategory;
        for (const Book& b : library_.books()) {
            byCategory[b.category()].first += 1;
            byCategory[b.category()].second += b.totalCopies();
        }
        std::vector<std::vector<std::string>> rows;
        for (const auto& e : byCategory) rows.push_back({html::esc(e.first), std::to_string(e.second.first), std::to_string(e.second.second)});
        body += "<h2>Books by category</h2>" + html::table({"Category", "Titles", "Copies"}, rows, "No books yet.");
    } else if (type == "history") {
        return HttpResponse::redirect("/history");
    } else {
        body += "<div class=\"alert alert-error\">Unknown report.</div>";
    }
    return page(c, title, "reports", body);
}

HttpResponse WebApp::historyPage(Context& c) {
    const std::string type = param(c, "type");
    std::vector<std::vector<std::string>> rows;
    const auto& all = library_.transactions();
    for (std::size_t i = all.size(); i > 0; --i) {
        const Transaction& t = all[i - 1];
        if (!type.empty() && t.type() != type) continue;
        rows.push_back({html::esc(util::formatId('T', t.id())), html::esc(t.timestamp()), html::badge(t.type(), "info"), html::esc(t.description())});
    }
    std::vector<std::string> types;
    for (const Transaction& t : all) {
        if (std::find(types.begin(), types.end(), t.type()) == types.end()) types.push_back(t.type());
    }
    std::string body = "<form method=\"get\" action=\"/history\" class=\"toolbar\"><select name=\"type\" aria-label=\"Activity type\">" + optionTag("", "All activities", type.empty());
    for (const std::string& t : types) body += optionTag(t, t, t == type);
    body += "</select><button type=\"submit\" class=\"btn btn-primary\">Filter</button></form>";
    body += html::table({"ID", "Date & time", "Activity", "Description"}, rows, "No activity recorded yet.");
    return page(c, "Activity History", "history", body);
}
