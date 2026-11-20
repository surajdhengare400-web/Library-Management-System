# Library Management System

A complete library management system written in **C++17** with classes and modules, using only the
standard library (no external dependencies, no database). It has **two user interfaces on top of one
shared core**:

- a **website** (HTML + CSS) served by a small C++ HTTP server built into the program, and
- a **menu-driven console application**.

[![CI](https://github.com/YOUR-USERNAME/YOUR-REPOSITORY/actions/workflows/ci.yml/badge.svg)](../../actions)

> Replace `YOUR-USERNAME/YOUR-REPOSITORY` in the badge above after you create the GitHub repository.

## Features

| Area | What you can do |
|---|---|
| Administrators | First-time Super Admin setup, login/logout, Super Admin and Admin roles, create / activate / deactivate admins, reset passwords, change own password, update profile |
| Books | Add, list, search (ID, title, author, category), edit, delete (blocked while copies are issued), total / available copies |
| Members | Register, list, search, edit, see a member's issue history, duplicate ID / email protection |
| Issue & return | Issue to registered members, return, due dates, automatic fines (default ₹5 per overdue day), mark fines paid, active / overdue / completed issues |
| Reports | Inventory, members, issued, returned, overdue, fines, statistics, transaction history (printable from the browser) |
| Dashboard | Titles, copies, available / issued copies, members, active and overdue issues, administrators, outstanding fines |
| History | Every important action is logged (who, what, when) and can be filtered |
| Storage | Permanent text files in `Data/`, saved after every change with safe temporary-file replacement |

## Quick start

### Windows 10/11 (VS Code + MinGW-w64)

You need MinGW-w64 `g++` 9 or newer (`g++ --version`). Open the project folder in VS Code, open a
terminal and run:

```bat
build.bat
.\LibraryWeb.exe
```

Then open **http://127.0.0.1:8080/** in your browser. On the first start you are asked to create the
Super Admin account (there is no default username or password).

For the console version run `.\LibraryConsole.exe` instead.

Manual build commands (what `build.bat` runs):

```bat
g++ -std=c++17 -O2 -Wall -Wextra -Isrc\core -Isrc\web src\core\*.cpp src\web\*.cpp -lws2_32 -o LibraryWeb.exe
g++ -std=c++17 -O2 -Wall -Wextra -Isrc\core -Isrc\console src\core\*.cpp src\console\*.cpp -o LibraryConsole.exe
```

### Linux / macOS / MSYS2

```bash
make            # builds LibraryWeb and LibraryConsole
./LibraryWeb    # then open http://127.0.0.1:8080/
make test       # runs all automated tests
```

### Web server options

```
LibraryWeb [--port 8080] [--host 127.0.0.1] [--data Data] [--public public]
```

Run it from the project folder so it finds `public/style.css`. By default it listens on
`127.0.0.1` only (this computer). Press **Ctrl+C** to stop; all data is already saved.

## Project structure

```
.
├── src/
│   ├── core/       business logic (no printing, no HTML): Library, AdminManager, Book, Member,
│   │               IssueRecord, Transaction, Admin, Person, FileManager, Security, Date, Utils ...
│   ├── console/    console interface: App, Console (menus/tables), ReportManager, main.cpp
│   └── web/        website: HttpServer (sockets), WebApp + WebCatalog (pages), Html, main.cpp
├── public/         style.css (the website design)
├── tests/          test_core.cpp, test_web.cpp, web_e2e.py (optional live test)
├── docs/           ARCHITECTURE, USER_GUIDE, WEB_INTERFACE, DATA_FORMAT, TESTING
├── .github/workflows/ci.yml   build + tests on Linux and Windows
├── Makefile, build.bat, build_tests.bat
├── README.md, CONTRIBUTING.md, CHANGELOG.md, LICENSE
└── Data/           created automatically at run time (ignored by git)
```

## Object-oriented design

| Concept | Where |
|---|---|
| Encapsulation | `Book` keeps `0 <= available <= total` itself; `Admin` hides salt and hash; fields are private |
| Inheritance | `Person` (abstract) → `Member`, `Admin` |
| Polymorphism | `Person::summary()`; `Serializable::serialize()` implemented by `Book`, `Member`, `IssueRecord`, `Transaction`, `Admin`; `FileManager::saveRecords<T>` saves any of them |
| Composition | `Library` owns books, members, issues, transactions; `WebApp` / `App` own `Library` + `AdminManager` |
| Separation of concerns | core rules ≠ file handling ≠ presentation; both UIs reuse the same core |

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for class diagrams and the request flow.

## Security

- No default credentials; passwords are never stored. Hashing is **PBKDF2-HMAC-SHA256**
  (200,000 iterations, random per-account salt), implemented in `src/core/Security.cpp` and checked
  against the official test vectors.
- Website: random session cookies (`HttpOnly`, `SameSite=Strict`), CSRF token on every POST,
  `Origin` check, HTML escaping of all data, restrictive `Content-Security-Policy`, login lock-out
  after 5 failures, idle session expiry (2 hours), permissions checked on the server for every request.
- The website speaks plain HTTP and listens on `127.0.0.1` by default. Do not expose it to the
  internet as it is; put it behind an HTTPS reverse proxy if you ever need remote access.

## Documentation

- [User guide](docs/USER_GUIDE.md) – how to use every screen
- [Architecture](docs/ARCHITECTURE.md) – classes, modules, request flow
- [Web interface](docs/WEB_INTERFACE.md) – routes, security measures, server behaviour
- [Data format](docs/DATA_FORMAT.md) – the files in `Data/`
- [Testing](docs/TESTING.md) – automated tests and a manual checklist

## Testing

```bash
make test            # Linux / macOS / MSYS2
build_tests.bat      # Windows
```

## Limitations

- Single-user style data store (plain text files): run only one program (web *or* console) at a time
  on the same `Data` folder.
- The built-in web server handles one request at a time; it is meant for a single library desk, not
  for heavy traffic.
- Names are best entered in plain ASCII in the console version (Windows consoles read non-ASCII
  keyboard input using a legacy code page). The website accepts any text.

## License

MIT – see [LICENSE](LICENSE).
