# Makefile for Linux, macOS and MSYS2/MinGW (GNU make). Windows users can use build.bat instead.
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
CORE     := $(wildcard src/core/*.cpp)
CONSOLE  := $(wildcard src/console/*.cpp)
WEB      := $(wildcard src/web/*.cpp)
TESTCONS := src/console/Console.cpp src/console/ReportManager.cpp

ifeq ($(OS),Windows_NT)
  EXE  := .exe
  LIBS := -lws2_32
endif

.PHONY: all console web test run-web clean

all: console web

console: LibraryConsole$(EXE)
web: LibraryWeb$(EXE)

LibraryConsole$(EXE): $(CORE) $(CONSOLE)
	$(CXX) $(CXXFLAGS) -Isrc/core -Isrc/console $^ -o $@

LibraryWeb$(EXE): $(CORE) $(WEB)
	$(CXX) $(CXXFLAGS) -Isrc/core -Isrc/web $^ $(LIBS) -o $@

build/test_core$(EXE): tests/test_core.cpp $(CORE) $(TESTCONS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc/core -Isrc/console $^ -o $@

build/test_web$(EXE): tests/test_web.cpp $(CORE) src/web/HttpServer.cpp src/web/Html.cpp src/web/WebApp.cpp src/web/WebCatalog.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -Isrc/core -Isrc/web $^ $(LIBS) -o $@

test: build/test_core$(EXE) build/test_web$(EXE)
	./build/test_core$(EXE)
	./build/test_web$(EXE)

run-web: LibraryWeb$(EXE)
	./LibraryWeb$(EXE)

clean:
	rm -rf build LibraryConsole LibraryWeb LibraryConsole.exe LibraryWeb.exe TestData_*
