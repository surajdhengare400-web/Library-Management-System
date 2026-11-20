# Contributing

1. Fork the repository and create a branch for your change.
2. Build and run the tests before and after your change:
   - Linux / macOS / MSYS2: `make test`
   - Windows (MinGW-w64): `build_tests.bat`
3. Keep the code warning-free with `-Wall -Wextra` (the build uses them).
4. Follow the existing structure:
   - business rules go in `src/core` (no printing, no HTML),
   - user-interface code goes in `src/console` or `src/web`,
   - every user-supplied value shown in HTML must go through `html::esc()`.
5. Add or update tests in `tests/` for every behaviour change, and note it in `CHANGELOG.md`.
6. Open a pull request that explains what changed and why.
