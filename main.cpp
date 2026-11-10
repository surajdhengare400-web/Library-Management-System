#include <iostream>
#include <fstream>
#include <string>
#include <limits>
using namespace std;

const int MAX_BOOKS = 100;
const int MAX_MEMBERS = 100;

int readInt(const string& prompt) {
    int value;
    while (true) {
        cout << prompt;
        if (cin >> value) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
        cout << "Invalid input. Enter a number.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

class Book {
private:
    int bookID;
    string title, author;
    bool available;

public:
    Book() : bookID(0), available(true) {}

    Book(int id, const string& t, const string& a)
        : bookID(id), title(t), author(a), available(true) {}

    int getBookID() const { return bookID; }
    string getTitle() const { return title; }
    string getAuthor() const { return author; }
    bool isAvailable() const { return available; }

    void setAvailability(bool status) { available = status; }

    void setBookDetails(int id, const string& t, const string& a, bool status) {
        bookID = id;
        title = t;
        author = a;
        available = status;
    }

    void display() const {
        cout << "Book ID: " << bookID
             << "\nTitle: " << title
             << "\nAuthor: " << author
             << "\nStatus: " << (available ? "Available" : "Issued")
             << "\n-------------------------\n";
    }
};

class Member {
private:
    int memberID;
    string name, email;

public:
    Member() : memberID(0) {}

    Member(int id, const string& n, const string& e)
        : memberID(id), name(n), email(e) {}

    int getMemberID() const { return memberID; }
    string getName() const { return name; }
    string getEmail() const { return email; }

    void setMemberDetails(int id, const string& n, const string& e) {
        memberID = id;
        name = n;
        email = e;
    }

    void display() const {
        cout << "Member ID: " << memberID
             << "\nName: " << name
             << "\nEmail: " << email
             << "\n-------------------------\n";
    }
};

class Library {
private:
    Book books[MAX_BOOKS];
    Member members[MAX_MEMBERS];
    int bookCount, memberCount;

    // Each issued book is linked to the member who borrowed it.
    int issuedBookIDs[MAX_BOOKS];
    int issuedMemberIDs[MAX_BOOKS];
    int issueCount;

    int findBookIndex(int id) const {
        for (int i = 0; i < bookCount; i++)
            if (books[i].getBookID() == id) return i;
        return -1;
    }

    int findMemberIndex(int id) const {
        for (int i = 0; i < memberCount; i++)
            if (members[i].getMemberID() == id) return i;
        return -1;
    }

    int findIssueIndex(int bookID) const {
        for (int i = 0; i < issueCount; i++)
            if (issuedBookIDs[i] == bookID) return i;
        return -1;
    }

    void saveBooks() const {
        ofstream file("books.txt");
        if (!file) {
            cout << "Error: Could not save books.txt\n";
            return;
        }
        for (int i = 0; i < bookCount; i++) {
            file << books[i].getBookID() << '\n'
                 << books[i].getTitle() << '\n'
                 << books[i].getAuthor() << '\n'
                 << books[i].isAvailable() << '\n';
        }
    }

    void loadBooks() {
        ifstream file("books.txt");
        if (!file) return;

        int id, status;
        string title, author;
        while (bookCount < MAX_BOOKS && file >> id) {
            file.ignore(numeric_limits<streamsize>::max(), '\n');
            if (!getline(file, title)) break;
            if (!getline(file, author)) break;
            if (!(file >> status)) break;
            file.ignore(numeric_limits<streamsize>::max(), '\n');

            books[bookCount].setBookDetails(id, title, author, status != 0);
            bookCount++;
        }
    }

    void saveMembers() const {
        ofstream file("members.txt");
        if (!file) {
            cout << "Error: Could not save members.txt\n";
            return;
        }
        for (int i = 0; i < memberCount; i++) {
            file << members[i].getMemberID() << '\n'
                 << members[i].getName() << '\n'
                 << members[i].getEmail() << '\n';
        }
    }

    void loadMembers() {
        ifstream file("members.txt");
        if (!file) return;

        int id;
        string name, email;
        while (memberCount < MAX_MEMBERS && file >> id) {
            file.ignore(numeric_limits<streamsize>::max(), '\n');
            if (!getline(file, name)) break;
            if (!getline(file, email)) break;
            members[memberCount].setMemberDetails(id, name, email);
            memberCount++;
        }
    }

    void saveIssues() const {
        ofstream file("issued.txt");
        if (!file) {
            cout << "Error: Could not save issued.txt\n";
            return;
        }
        for (int i = 0; i < issueCount; i++)
            file << issuedBookIDs[i] << ' ' << issuedMemberIDs[i] << '\n';
    }

    void loadIssues() {
        ifstream file("issued.txt");
        if (!file) return;

        int bookID, memberID;
        while (issueCount < MAX_BOOKS && file >> bookID >> memberID) {
            int bi = findBookIndex(bookID);
            int mi = findMemberIndex(memberID);
            if (bi != -1 && mi != -1 && !books[bi].isAvailable()) {
                issuedBookIDs[issueCount] = bookID;
                issuedMemberIDs[issueCount] = memberID;
                issueCount++;
            }
        }
    }

public:
    Library() : bookCount(0), memberCount(0), issueCount(0) {
        loadBooks();
        loadMembers();
        loadIssues();
    }

    void addBook() {
        if (bookCount >= MAX_BOOKS) {
            cout << "Book storage is full.\n";
            return;
        }
        int id = readInt("Enter Book ID: ");
        if (findBookIndex(id) != -1) {
            cout << "A book with this ID already exists.\n";
            return;
        }
        string title, author;
        cout << "Enter Book Title: ";
        getline(cin, title);
        cout << "Enter Author Name: ";
        getline(cin, author);
        if (title.empty() || author.empty()) {
            cout << "Title and author cannot be empty.\n";
            return;
        }
        books[bookCount++] = Book(id, title, author);
        saveBooks();
        cout << "Book added successfully.\n";
    }

    void displayBooks() const {
        if (bookCount == 0) {
            cout << "No books found.\n";
            return;
        }
        for (int i = 0; i < bookCount; i++) books[i].display();
    }

    void searchBook() const {
        int id = readInt("Enter Book ID to search: ");
        for (int i = 0; i < bookCount; i++) {
            if (books[i].getBookID() == id) {
                books[i].display();
                return;
            }
        }
        cout << "Book not found.\n";
    }

    void registerMember() {
        if (memberCount >= MAX_MEMBERS) {
            cout << "Member storage is full.\n";
            return;
        }
        int id = readInt("Enter Member ID: ");
        if (findMemberIndex(id) != -1) {
            cout << "A member with this ID already exists.\n";
            return;
        }
        string name, email;
        cout << "Enter Member Name: ";
        getline(cin, name);
        cout << "Enter Email: ";
        getline(cin, email);
        if (name.empty() || email.empty()) {
            cout << "Name and email cannot be empty.\n";
            return;
        }
        members[memberCount++] = Member(id, name, email);
        saveMembers();
        cout << "Member registered successfully.\n";
    }

    void displayMembers() const {
        if (memberCount == 0) {
            cout << "No members registered.\n";
            return;
        }
        for (int i = 0; i < memberCount; i++) members[i].display();
    }

    void searchMember() const {
        int id = readInt("Enter Member ID to search: ");
        int index = findMemberIndex(id);
        if (index == -1) cout << "Member not found.\n";
        else members[index].display();
    }

    void issueBook() {
        int bookID = readInt("Enter Book ID to issue: ");
        int bi = findBookIndex(bookID);
        if (bi == -1) {
            cout << "Book not found.\n";
            return;
        }
        if (!books[bi].isAvailable()) {
            cout << "This book is already issued.\n";
            return;
        }

        int memberID = readInt("Enter registered Member ID: ");
        int mi = findMemberIndex(memberID);
        if (mi == -1) {
            cout << "Member not found. Register the member first.\n";
            return;
        }
        if (issueCount >= MAX_BOOKS) {
            cout << "Issue record storage is full.\n";
            return;
        }

        books[bi].setAvailability(false);
        issuedBookIDs[issueCount] = bookID;
        issuedMemberIDs[issueCount] = memberID;
        issueCount++;

        saveBooks();
        saveIssues();
        cout << "Book issued successfully to " << members[mi].getName() << ".\n";
    }

    void returnBook() {
        int bookID = readInt("Enter Book ID to return: ");
        int bi = findBookIndex(bookID);
        if (bi == -1) {
            cout << "Book not found.\n";
            return;
        }
        int ii = findIssueIndex(bookID);
        if (ii == -1) {
            cout << "This book is not currently issued.\n";
            return;
        }

        int memberIndex = findMemberIndex(issuedMemberIDs[ii]);
        cout << "Returned by Member ID: " << issuedMemberIDs[ii];
        if (memberIndex != -1)
            cout << " (" << members[memberIndex].getName() << ")";
        cout << '\n';

        books[bi].setAvailability(true);
        for (int i = ii; i < issueCount - 1; i++) {
            issuedBookIDs[i] = issuedBookIDs[i + 1];
            issuedMemberIDs[i] = issuedMemberIDs[i + 1];
        }
        issueCount--;

        saveBooks();
        saveIssues();
        cout << "Book returned successfully.\n";
    }

    void run() {
        int choice;
        do {
            cout << "\n========== LIBRARY MANAGEMENT SYSTEM ==========\n"
                 << "1. Add New Book\n"
                 << "2. Display All Books\n"
                 << "3. Search Book by ID\n"
                 << "4. Register New Member\n"
                 << "5. Display All Members\n"
                 << "6. Search Member by ID\n"
                 << "7. Issue Book to Member\n"
                 << "8. Return Book\n"
                 << "9. Exit\n";

            choice = readInt("Enter your choice: ");
            switch (choice) {
                case 1: addBook(); break;
                case 2: displayBooks(); break;
                case 3: searchBook(); break;
                case 4: registerMember(); break;
                case 5: displayMembers(); break;
                case 6: searchMember(); break;
                case 7: issueBook(); break;
                case 8: returnBook(); break;
                case 9: cout << "Thank you for using the Library Management System.\n"; break;
                default: cout << "Invalid choice. Please select 1-9.\n";
            }
        } while (choice != 9);
    }
};

int main() {
    Library library;
    library.run();
    return 0;
}
