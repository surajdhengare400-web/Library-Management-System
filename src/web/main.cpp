// main.cpp (web) - starts the Library Management website on http://127.0.0.1:8080
#include <cstdlib>
#include <iostream>
#include <string>

#include "HttpServer.h"
#include "WebApp.h"

int main(int argc, char** argv) {
    std::string host = "127.0.0.1";
    std::string dataDir = "Data";
    std::string publicDir = "public";
    int port = 8080;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        const bool hasValue = i + 1 < argc;
        if (arg == "--port" && hasValue) port = std::atoi(argv[++i]);
        else if (arg == "--host" && hasValue) host = argv[++i];
        else if (arg == "--data" && hasValue) dataDir = argv[++i];
        else if (arg == "--public" && hasValue) publicDir = argv[++i];
        else {
            std::cout << "Usage: LibraryWeb [--port 8080] [--host 127.0.0.1] [--data Data] [--public public]\n";
            return arg == "--help" ? 0 : 1;
        }
    }
    if (port < 1 || port > 65535) {
        std::cerr << "Invalid port number.\n";
        return 1;
    }

    WebApp app(dataDir, publicDir);
    std::vector<std::string> warnings;
    std::string fatal;
    if (!app.init(warnings, fatal)) {
        std::cerr << "ERROR: " << fatal << "\nThe server stopped to protect your existing data.\n";
        return 1;
    }
    for (const std::string& w : warnings) std::cout << "WARNING: " << w << '\n';

    HttpServer server;
    std::string error;
    if (!server.listen(host, port, error)) {
        std::cerr << "ERROR: " << error << '\n';
        return 1;
    }
    std::cout << "Library Management System (web)\n"
              << "Open this address in your browser:  http://" << host << ':' << port << "/\n"
              << "Press Ctrl+C to stop the server." << std::endl;
    server.serve([&app](const HttpRequest& request) { return app.handle(request); });
    std::cout << "\nServer stopped. All data is already saved.\n";
    return 0;
}
