#include "HttpServer.h"

#include <csignal>
#include <cstring>
#include <exception>
#include <iostream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
using SocketHandle = SOCKET;
static const SocketHandle kInvalidSocket = INVALID_SOCKET;
static void closeSocket(SocketHandle s) { ::closesocket(s); }
typedef int SendLength;
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
using SocketHandle = int;
static const SocketHandle kInvalidSocket = -1;
static void closeSocket(SocketHandle s) { ::close(s); }
typedef ssize_t SendLength;
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

namespace {

volatile std::sig_atomic_t g_stopRequested = 0;
extern "C" void onInterrupt(int) { g_stopRequested = 1; }

constexpr std::size_t kMaxHeadBytes = 32 * 1024;
constexpr std::size_t kMaxBodyBytes = 1024 * 1024;

std::string lower(std::string s) {
    for (char& c : s) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return s;
}

std::string trimSpaces(const std::string& s) {
    std::size_t b = 0, e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t')) ++b;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r')) --e;
    return s.substr(b, e - b);
}

int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool sendAll(SocketHandle s, const std::string& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
        const SendLength n = ::send(s, data.data() + sent, static_cast<int>(data.size() - sent), MSG_NOSIGNAL);
        if (n <= 0) return false;
        sent += static_cast<std::size_t>(n);
    }
    return true;
}

void setTimeouts(SocketHandle s, int seconds) {
#ifdef _WIN32
    DWORD ms = static_cast<DWORD>(seconds * 1000);
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&ms), sizeof(ms));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&ms), sizeof(ms));
#else
    timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
}

std::string serialize(const HttpResponse& r) {
    std::string out = "HTTP/1.1 " + std::to_string(r.status) + " " + http::statusText(r.status) + "\r\n";
    out += "Content-Type: " + r.contentType + "\r\n";
    out += "Content-Length: " + std::to_string(r.body.size()) + "\r\n";
    out += "Connection: close\r\n";
    out += "Cache-Control: no-store\r\n";
    out += "X-Content-Type-Options: nosniff\r\n";
    out += "X-Frame-Options: DENY\r\n";
    out += "Referrer-Policy: same-origin\r\n";
    out += "Content-Security-Policy: default-src 'self'; style-src 'self'; form-action 'self'; frame-ancestors 'none'\r\n";
    for (const auto& h : r.headers) out += h.first + ": " + h.second + "\r\n";
    out += "\r\n";
    out += r.body;
    return out;
}

// Reads one request from the socket. Returns an HTTP status code to answer with (0 = no answer possible).
int readRequest(SocketHandle client, HttpRequest& request) {
    std::string buffer;
    char chunk[4096];
    std::size_t headEnd = std::string::npos;
    while (headEnd == std::string::npos) {
        const int n = ::recv(client, chunk, sizeof(chunk), 0);
        if (n <= 0) return 0;
        buffer.append(chunk, static_cast<std::size_t>(n));
        headEnd = buffer.find("\r\n\r\n");
        if (headEnd == std::string::npos && buffer.size() > kMaxHeadBytes) return 431;
    }
    if (!http::parseRequestHead(buffer.substr(0, headEnd), request)) return 400;

    std::size_t contentLength = 0;
    const std::string lengthText = request.header("content-length");
    if (!lengthText.empty()) {
        for (char c : lengthText) {
            if (c < '0' || c > '9' || lengthText.size() > 9) return 400;
        }
        contentLength = static_cast<std::size_t>(std::stoul(lengthText));
        if (contentLength > kMaxBodyBytes) return 413;
    }
    request.body = buffer.substr(headEnd + 4);
    while (request.body.size() < contentLength) {
        const int n = ::recv(client, chunk, sizeof(chunk), 0);
        if (n <= 0) return 0;
        request.body.append(chunk, static_cast<std::size_t>(n));
        if (request.body.size() > kMaxBodyBytes) return 413;
    }
    request.body.resize(contentLength);
    const std::string type = lower(request.header("content-type"));
    if (type.find("application/x-www-form-urlencoded") != std::string::npos) {
        request.form = http::parseUrlEncoded(request.body);
    }
    return 200;
}

}  // namespace

// ------------------------------------------------------------------------------------------
// http helpers
// ------------------------------------------------------------------------------------------

namespace http {

std::string urlDecode(const std::string& text, bool plusAsSpace) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '+' && plusAsSpace) {
            out += ' ';
        } else if (c == '%' && i + 2 < text.size() + 0 && hexValue(text[i + 1]) >= 0 && hexValue(text[i + 2]) >= 0) {
            out += static_cast<char>(hexValue(text[i + 1]) * 16 + hexValue(text[i + 2]));
            i += 2;
        } else {
            out += c;
        }
    }
    return out;
}

std::string urlEncode(const std::string& text) {
    static const char digits[] = "0123456789ABCDEF";
    std::string out;
    for (char ch : text) {
        const unsigned char c = static_cast<unsigned char>(ch);
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' ||
            c == '.' || c == '~') {
            out += ch;
        } else {
            out += '%';
            out += digits[c >> 4];
            out += digits[c & 0x0F];
        }
    }
    return out;
}

std::map<std::string, std::string> parseUrlEncoded(const std::string& text) {
    std::map<std::string, std::string> result;
    std::size_t start = 0;
    while (start <= text.size()) {
        std::size_t amp = text.find('&', start);
        if (amp == std::string::npos) amp = text.size();
        const std::string pair = text.substr(start, amp - start);
        if (!pair.empty()) {
            const std::size_t eq = pair.find('=');
            const std::string key = urlDecode(pair.substr(0, eq), true);
            const std::string value = eq == std::string::npos ? "" : urlDecode(pair.substr(eq + 1), true);
            if (!key.empty()) result[key] = value;
        }
        start = amp + 1;
    }
    return result;
}

std::map<std::string, std::string> parseCookies(const std::string& headerValue) {
    std::map<std::string, std::string> cookies;
    std::size_t start = 0;
    while (start < headerValue.size()) {
        std::size_t semi = headerValue.find(';', start);
        if (semi == std::string::npos) semi = headerValue.size();
        const std::string part = trimSpaces(headerValue.substr(start, semi - start));
        const std::size_t eq = part.find('=');
        if (eq != std::string::npos && eq > 0) cookies[part.substr(0, eq)] = part.substr(eq + 1);
        start = semi + 1;
    }
    return cookies;
}

const char* statusText(int status) {
    switch (status) {
        case 200: return "OK";
        case 204: return "No Content";
        case 302: return "Found";
        case 303: return "See Other";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 429: return "Too Many Requests";
        case 431: return "Request Header Fields Too Large";
        case 500: return "Internal Server Error";
        default: return "OK";
    }
}

bool parseRequestHead(const std::string& head, HttpRequest& request) {
    const std::size_t lineEnd = head.find("\r\n");
    const std::string requestLine = head.substr(0, lineEnd);
    const std::size_t sp1 = requestLine.find(' ');
    const std::size_t sp2 = requestLine.rfind(' ');
    if (sp1 == std::string::npos || sp2 == sp1) return false;
    request.method = requestLine.substr(0, sp1);
    const std::string target = requestLine.substr(sp1 + 1, sp2 - sp1 - 1);
    const std::string version = requestLine.substr(sp2 + 1);
    if (version.rfind("HTTP/1.", 0) != 0 || target.empty() || target[0] != '/') return false;
    const std::size_t q = target.find('?');
    request.path = urlDecode(target.substr(0, q), false);
    request.rawQuery = q == std::string::npos ? "" : target.substr(q + 1);
    request.query = parseUrlEncoded(request.rawQuery);

    std::size_t pos = lineEnd == std::string::npos ? head.size() : lineEnd + 2;
    while (pos < head.size()) {
        std::size_t end = head.find("\r\n", pos);
        if (end == std::string::npos) end = head.size();
        const std::string line = head.substr(pos, end - pos);
        const std::size_t colon = line.find(':');
        if (colon != std::string::npos && colon > 0) {
            request.headers[lower(trimSpaces(line.substr(0, colon)))] = trimSpaces(line.substr(colon + 1));
        }
        pos = end + 2;
    }
    request.cookies = parseCookies(request.header("cookie"));
    return true;
}

}  // namespace http

std::string HttpRequest::param(const std::string& name) const {
    auto f = form.find(name);
    if (f != form.end()) return f->second;
    auto q = query.find(name);
    return q != query.end() ? q->second : std::string();
}

std::string HttpRequest::header(const std::string& lowerName) const {
    auto it = headers.find(lowerName);
    return it != headers.end() ? it->second : std::string();
}

HttpResponse HttpResponse::html(std::string body, int status) {
    HttpResponse r;
    r.status = status;
    r.body = std::move(body);
    return r;
}

HttpResponse HttpResponse::text(int status, std::string body) {
    HttpResponse r;
    r.status = status;
    r.contentType = "text/plain; charset=utf-8";
    r.body = std::move(body);
    return r;
}

HttpResponse HttpResponse::redirect(const std::string& location) {
    HttpResponse r;
    r.status = 303;
    r.contentType = "text/plain; charset=utf-8";
    r.body = "Redirecting...";
    r.headers.emplace_back("Location", location);
    return r;
}

// ------------------------------------------------------------------------------------------
// HttpServer
// ------------------------------------------------------------------------------------------

HttpServer::HttpServer() : socket_(static_cast<std::intptr_t>(kInvalidSocket)), networkReady_(false) {}

HttpServer::~HttpServer() {
    if (socket_ != static_cast<std::intptr_t>(kInvalidSocket)) closeSocket(static_cast<SocketHandle>(socket_));
#ifdef _WIN32
    if (networkReady_) WSACleanup();
#endif
}

void HttpServer::requestStop() { g_stopRequested = 1; }

bool HttpServer::listen(const std::string& host, int port, std::string& error) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        error = "Could not initialise Winsock.";
        return false;
    }
    networkReady_ = true;
#else
    std::signal(SIGPIPE, SIG_IGN);
#endif
    SocketHandle s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (s == kInvalidSocket) {
        error = "Could not create a network socket.";
        return false;
    }
#ifndef _WIN32
    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#endif
    sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<unsigned short>(port));
    if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1) {
        closeSocket(s);
        error = "Invalid IPv4 address '" + host + "'.";
        return false;
    }
    if (::bind(s, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        closeSocket(s);
        error = "Could not listen on " + host + ":" + std::to_string(port) + " (is the port already in use? try --port 8090).";
        return false;
    }
    if (::listen(s, 16) != 0) {
        closeSocket(s);
        error = "listen() failed.";
        return false;
    }
    socket_ = static_cast<std::intptr_t>(s);
    return true;
}

void HttpServer::serve(const Handler& handler) {
    std::signal(SIGINT, onInterrupt);
    const SocketHandle server = static_cast<SocketHandle>(socket_);
    while (!g_stopRequested) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(server, &readSet);
        timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        const int ready = ::select(static_cast<int>(server) + 1, &readSet, nullptr, nullptr, &timeout);
        if (ready <= 0) continue;

        sockaddr_in peer;
#ifdef _WIN32
        int peerLength = sizeof(peer);
#else
        socklen_t peerLength = sizeof(peer);
#endif
        SocketHandle client = ::accept(server, reinterpret_cast<sockaddr*>(&peer), &peerLength);
        if (client == kInvalidSocket) continue;
        setTimeouts(client, 10);

        HttpRequest request;
        char ip[64] = "";
        inet_ntop(AF_INET, &peer.sin_addr, ip, sizeof(ip));
        request.remoteAddress = ip;

        HttpResponse response;
        const int status = readRequest(client, request);
        if (status == 0) {
            closeSocket(client);
            continue;
        }
        if (status != 200) {
            response = HttpResponse::text(status, std::string(http::statusText(status)) + "\n");
        } else {
            try {
                response = handler(request);
            } catch (const std::exception& ex) {
                std::cerr << "  [server] unhandled error: " << ex.what() << '\n';
                response = HttpResponse::text(500, "Internal server error\n");
            }
        }
        sendAll(client, serialize(response));
        closeSocket(client);
    }
}
