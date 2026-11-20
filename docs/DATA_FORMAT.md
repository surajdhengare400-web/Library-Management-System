# Data Format

All files are UTF-8 text in the `Data/` folder, one record per line, fields separated by `|`.
Inside a field, `\` is written `\\`, `|` is written `\p`, and line breaks `\n` / `\r`.
Lines starting with `#` and empty lines are ignored.

| File | Fields |
|---|---|
| `books.txt` | `id｜title｜author｜category｜year｜totalCopies｜availableCopies` |
| `members.txt` | `id｜fullName｜email｜phone｜registrationDate` |
| `issue_records.txt` | `issueId｜bookId｜memberId｜issueDate｜dueDate｜returnDate or "-"｜fine｜finePaid(0/1)` |
| `transactions.txt` | `id｜type｜timestamp｜description` |
| `admins.txt` | `username｜role(SUPER_ADMIN/ADMIN)｜active(0/1)｜fullName｜email｜iterations｜saltHex｜hashHex｜createdAt` |
| `settings.txt` | `fineRatePerDay｜issuePeriodDays` |

(The `｜` above only avoids breaking this table; the real separator is the ASCII `|`.)

Dates are `YYYY-MM-DD`; timestamps are `YYYY-MM-DD HH:MM:SS` in local time. IDs are stored in upper case
(usernames in lower case).

## Safe saving

1. Data is written to `<file>.tmp`.
2. The temp file replaces the real file (atomic rename; fallback keeps `<file>.bak` until the swap succeeds).

## Loading problems

- Missing file → treated as empty (first run).
- Malformed or duplicate record → skipped; a warning is shown; the original is copied to
  `<file>.corrupt-<date-time>.bak` before any rewrite.
- File exists but cannot be opened → the program stops without touching it.
