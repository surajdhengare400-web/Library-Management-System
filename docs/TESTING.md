# Testing

## Automated tests

```bash
make test            # Linux / macOS / MSYS2
build_tests.bat      # Windows (MinGW-w64)
```

| Suite | File | Checks |
|---|---|---|
| Core | `tests/test_core.cpp` | Dates, SHA-256 / PBKDF2 test vectors, validators, file handling, books, members, issues, fine calculation (fixed dates), persistence, malformed and missing files, authentication, roles and permissions |
| Web | `tests/test_web.cpp` | URL/cookie parsing, HTML escaping, setup/login/logout, CSRF and Origin checks, XSS escaping, book/member/issue workflows, reports, Super Admin-only pages, session invalidation, login lock-out |

The web tests call `WebApp::handle()` directly, so they need no network, browser or Python.

## Optional live test

`tests/web_e2e.py` (Python 3, standard library only) drives a running server over real HTTP:

```bash
./LibraryWeb --port 8099 --data TestData_live      # empty data folder!
python3 tests/web_e2e.py
```

## Manual checklist

- [ ] First start shows the setup page; a weak password is rejected.
- [ ] Login fails with a wrong password; succeeds with the right one; logout ends the session.
- [ ] Add / search / edit / delete a book; deleting an issued book is refused.
- [ ] Register a member; a duplicate ID or email is refused.
- [ ] Issue → return a book; the same book cannot be issued twice to one member.
- [ ] Create a normal Admin; they cannot open *Administration*.
- [ ] Restart the program: all data is still there.
- [ ] Damage a line in `Data/books.txt`: a warning appears and a `.corrupt-*.bak` copy is created.
