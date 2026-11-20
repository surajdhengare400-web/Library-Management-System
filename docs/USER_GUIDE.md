# User Guide

## First start

1. Start the program (`LibraryWeb.exe`, then open <http://127.0.0.1:8080/>).
2. The **First-Time Setup** page asks for a username, name, email and password. This creates the
   **Super Admin**. Password rules: 8-64 characters with an uppercase letter, a lowercase letter, a
   digit and a special character; no spaces.
3. Log in. You land on the **Dashboard**.

## Menu

| Item | Purpose |
|---|---|
| Dashboard | Key numbers, overdue books, recent activity and shortcuts |
| Books | Search, add, edit and delete books |
| Members | Search, register and edit members; open a member to see their issues |
| Issue & Return | Issue a book, return it, see active / overdue / completed issues and unpaid fines |
| Reports | Eight printable reports (use the browser's print function, Ctrl+P) |
| Activity History | Who did what and when; filter by activity type |
| My Account | Update your name/email, change your password |
| Administration | *(Super Admin only)* manage administrators and library settings |

## Everyday tasks

**Add a book** – Books → *+ Add book*. IDs are 3-12 letters/digits/`-` and must be unique.

**Register a member** – Members → *+ Register member*. Member ID and email must be unique.

**Issue a book** – Issue & Return → *+ Issue a book*. Type a member ID and a book ID (suggestions
appear). The due date is today + the issue period (14 days by default). A member can have at most 5
active issues and cannot borrow the same title twice at once.

**Return a book** – Issue & Return → *Return* next to the issue. Late returns get a fine of
(days late × fine rate). Use *Mark fine paid* when the fine is collected.

**Delete a book** – Books → *Delete* → confirm. Not possible while any copy is issued.

## Administrators (Super Admin)

- *Create administrator*: choose role **Admin** (daily work) or **Super Admin** (can manage admins).
- *Deactivate / Activate*: you cannot deactivate yourself, and the last active Super Admin cannot be
  deactivated.
- *Reset password*: sets a new password for another administrator.
- *Library settings*: fine per overdue day and issue period. Existing fines are not recalculated.

## Fines

Fine = days overdue × fine per day. *Outstanding fines* on the dashboard = unpaid fines on returned
books + fines currently accruing on overdue books.

## Console version

`LibraryConsole.exe` offers the same functions through numbered menus. Type `/c` at any prompt to
cancel the current operation.

## Troubleshooting

| Problem | Solution |
|---|---|
| "Could not listen on 127.0.0.1:8080" | Another program uses the port: `LibraryWeb.exe --port 8090` |
| Page has no styling | Start the server from the project folder (it needs `public/style.css`) |
| Forgot the only Super Admin password | Stop the program and delete `Data/admins.txt`; setup runs again. Books, members and history are kept |
| A warning about malformed records | A data file was damaged; valid records were kept and the original was copied to `*.corrupt-*.bak` |
| "Too many failed attempts" | Wait 30 seconds and try again |
