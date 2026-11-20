// main.cpp - program entry point. All logic lives in the classes; main only starts the App.
#include <exception>
#include <iostream>

#include "App.h"

int main() {
    try {
        App app;
        return app.run();
    } catch (const std::exception& ex) {
        std::cerr << "\nUnexpected error: " << ex.what() << '\n';
        return 2;
    }
}
