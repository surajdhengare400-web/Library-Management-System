#include "Html.h"

namespace html {

std::string esc(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 8);
    for (char c : text) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default: out += c;
        }
    }
    return out;
}

std::string table(const std::vector<std::string>& headers, const std::vector<std::vector<std::string>>& rows,
                  const std::string& emptyMessage) {
    if (rows.empty()) return "<p class=\"empty\">" + esc(emptyMessage) + "</p>";
    std::string out = "<div class=\"table-wrap\"><table><thead><tr>";
    for (const std::string& h : headers) out += "<th>" + esc(h) + "</th>";
    out += "</tr></thead><tbody>";
    for (const auto& row : rows) {
        out += "<tr>";
        for (const std::string& cell : row) out += "<td>" + cell + "</td>";
        out += "</tr>";
    }
    out += "</tbody></table></div><p class=\"count\">" + std::to_string(rows.size()) + " record(s)</p>";
    return out;
}

std::string badge(const std::string& text, const std::string& kind) {
    return "<span class=\"badge badge-" + kind + "\">" + esc(text) + "</span>";
}

std::string field(const std::string& label, const std::string& name, const std::string& value,
                  const std::string& type, const std::string& attributes) {
    return "<label class=\"field\"><span>" + esc(label) + "</span><input type=\"" + type + "\" name=\"" + esc(name) +
           "\" value=\"" + esc(value) + "\" " + attributes + "></label>";
}

std::string hidden(const std::string& name, const std::string& value) {
    return "<input type=\"hidden\" name=\"" + esc(name) + "\" value=\"" + esc(value) + "\">";
}

std::string statCard(const std::string& label, const std::string& value, const std::string& kind) {
    return "<div class=\"card stat " + kind + "\"><div class=\"stat-value\">" + esc(value) +
           "</div><div class=\"stat-label\">" + esc(label) + "</div></div>";
}

namespace {

std::string head(const std::string& title) {
    return "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n"
           "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n<title>" +
           esc(title) + " - Library Management System</title>\n"
           "<link rel=\"stylesheet\" href=\"/static/style.css\">\n</head>\n";
}

std::string flashBox(const std::string& text, const std::string& type) {
    if (text.empty()) return "";
    return "<div class=\"alert alert-" + esc(type.empty() ? "info" : type) + "\" role=\"alert\">" + esc(text) + "</div>";
}

std::string navItem(const std::string& key, const std::string& active, const std::string& href, const std::string& label) {
    return "<a href=\"" + href + "\"" + (key == active ? " class=\"active\" aria-current=\"page\"" : "") + ">" + label + "</a>";
}

}  // namespace

std::string layout(const LayoutInfo& info, const std::string& body) {
    std::string out = head(info.title);
    out += "<body>\n<div class=\"app\">\n<aside class=\"sidebar\">\n";
    out += "<div class=\"brand\"><span class=\"brand-mark\">LMS</span><span>Library<br>Management</span></div>\n<nav>\n";
    out += navItem("dashboard", info.active, "/", "Dashboard");
    out += navItem("books", info.active, "/books", "Books");
    out += navItem("members", info.active, "/members", "Members");
    out += navItem("issues", info.active, "/issues", "Issue &amp; Return");
    out += navItem("reports", info.active, "/reports", "Reports");
    out += navItem("history", info.active, "/history", "Activity History");
    out += navItem("account", info.active, "/account", "My Account");
    if (info.superAdmin) out += navItem("admin", info.active, "/admin", "Administration");
    out += "\n</nav>\n<div class=\"user-box\"><strong>" + esc(info.userName) + "</strong><span>" + esc(info.roleName) +
           "</span><form method=\"post\" action=\"/logout\">" + hidden("csrf", info.csrf) +
           "<button type=\"submit\" class=\"btn btn-ghost btn-block\">Log out</button></form></div>\n</aside>\n";
    out += "<main class=\"content\">\n<header class=\"page-head\"><h1>" + esc(info.title) + "</h1></header>\n";
    out += flashBox(info.flashText, info.flashType);
    out += body;
    out += "\n</main>\n</div>\n</body>\n</html>\n";
    return out;
}

std::string publicLayout(const std::string& title, const std::string& body, const std::string& flashText,
                         const std::string& flashType) {
    std::string out = head(title);
    out += "<body class=\"public\">\n<div class=\"login-card\">\n<div class=\"brand brand-center\"><span class=\"brand-mark\">LMS</span>"
           "<span>Library Management System</span></div>\n";
    out += "<h1>" + esc(title) + "</h1>\n";
    out += flashBox(flashText, flashType);
    out += body;
    out += "\n</div>\n</body>\n</html>\n";
    return out;
}

}  // namespace html
