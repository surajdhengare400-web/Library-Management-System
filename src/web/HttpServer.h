// HttpServer.h - a tiny single-threaded HTTP/1.1 server built on plain sockets (Winsock on
// Windows, POSIX sockets elsewhere). It only knows about requests and responses; all application
// logic lives in WebApp.
#pragma once
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

struct HttpRequest {
    std::string method;                                  // "GET", "POST", ...
    std::string path;                                    // "/books/edit"
    std::string rawQuery;
    std::map<std::string, std::string> headers;          // header names in lower case
    std::map<std::string, std::string> query;            // decoded ?a=b
    std::map<std::string, std::string> form;             // decoded application/x-www-form-urlencoded body
    std::map<std::string, std::string> cookies;
    std::string body;
    std::string remoteAddress;

    // Form value if present, otherwise query value, otherwise empty.
    std::string param(const std::string& name) const;
    std::string header(const std::string& lowerName) const;
};

struct HttpResponse {
    int status = 200;
    std::string contentType = "text/html; charset=utf-8";
    std::string body;
    std::vector<std::pair<std::string, std::string>> headers;

    static HttpResponse html(std::string body, int status = 200);
    static HttpResponse text(int status, std::string body);
    static HttpResponse redirect(const std::string& location);
};

namespace http {
std::string urlDecode(const std::string& text, bool plusAsSpace);
std::string urlEncode(const std::string& text);
std::map<std::string, std::string> parseUrlEncoded(const std::string& text);
std::map<std::string, std::string> parseCookies(const std::string& headerValue);
const char* statusText(int status);
// Parses the request head ("GET /x?y HTTP/1.1\r\nHeader: v\r\n...") into `request`.
bool parseRequestHead(const std::string& head, HttpRequest& request);
}  // namespace http

class HttpServer {
public:
    using Handler = std::function<HttpResponse(const HttpRequest&)>;

    HttpServer();
    ~HttpServer();
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    bool listen(const std::string& host, int port, std::string& error);
    void serve(const Handler& handler);      // blocks until Ctrl+C
    static void requestStop();

private:
    std::intptr_t socket_;
    bool networkReady_;
};
