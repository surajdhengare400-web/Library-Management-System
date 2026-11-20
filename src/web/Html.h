// Html.h - small helpers that build the HTML pages (escaping, tables, forms, page layout).
// Every piece of user data goes through html::esc() before it is written into a page.
#pragma once
#include <string>
#include <vector>

namespace html {

std::string esc(const std::string& text);

// Builds a table. Header texts are escaped; cells must already be safe HTML (use esc() / badge()).
std::string table(const std::vector<std::string>& headers, const std::vector<std::vector<std::string>>& rows,
                  const std::string& emptyMessage);
std::string badge(const std::string& text, const std::string& kind);     // kind: ok, warn, danger, info, muted
std::string field(const std::string& label, const std::string& name, const std::string& value,
                  const std::string& type = "text", const std::string& attributes = "");
std::string hidden(const std::string& name, const std::string& value);
std::string statCard(const std::string& label, const std::string& value, const std::string& kind = "");

struct LayoutInfo {
    std::string title;
    std::string active;          // key of the highlighted menu item
    std::string userName;
    std::string roleName;
    bool superAdmin = false;
    std::string csrf;
    std::string flashText;
    std::string flashType;       // success, error, info
};

std::string layout(const LayoutInfo& info, const std::string& body);
std::string publicLayout(const std::string& title, const std::string& body, const std::string& flashText,
                         const std::string& flashType);

}  // namespace html
