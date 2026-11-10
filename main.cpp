#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <limits>
#include <cctype>
using namespace std;

const int MAX = 100;
const int FINE_PER_DAY = 5;

class Book {
public:
    int id;
    string title, author;
    bool available;

    Book() {
        id = 0;
        available = true;
    }
};

class Member {
public:
    int id;
    string name, email;
};

class Library {
private:
    Book books[MAX];
    Member members[MAX];
    int bookCount = 0;
    int memberCount = 0;

    int findBook(int id) {
        for (int i = 0; i < bookCount; i++)
            if (books[i].id == id) return i;
        return -1;
    }

    int findMember(int id) {
        for (int i = 0; i < memberCount; i++)
            if (members[i].id == id) return i;
        return -1;
    }

    void saveBooks() {
        ofstream file("books.txt");
        for (int i = 0; i < bookCount; i++) {
            file << books[i].id << '\n'
                 << books[i].title << '\n'
                 << books[i].author << '\n'
                 << books[i].available << '\n';
        }
    }

    void saveMembers() {
        ofstream file("members.txt");
        for (int i = 0; i < memberCount; i++) {
            file << members[i].id << '\n'
                 << members[i].name << '\n'
                 << members[i].email << '\n';
        }
    }

    void logTransaction(const string& action, int bookID,
                        int memberID, int days = 0, int fine = 0) {
        ofstream file("transactions.txt", ios::app);
        time_t now = time(0);
        char* date = ctime(&now);

        file << action << " | Book ID: " << bookID
             << " | Member ID: " << memberID
             << " | Date: " << (date ? date : "Unknown")
             << "Days overdue: " << days
             << " | Fine: Rs." << fine << "\n";
    }

    void loadBooks() {
        ifstream file("books.txt");
        while (bookCount < MAX && (file >> books[bookCount].id)) {
            file.ignore(numeric_limits<streamsize>::max(), '\n');
            getline(file, books[bookCount].title);
            getline(file, books[bookCount].author);
            file >> books[bookCount].available;
            file.ignore(numeric_limits<streamsize>::max(), '\n');
            bookCount++;
        }
    }

    void loadMembers() {
        ifstream file("members.txt");
        while (memberCount < MAX && (file >> members[memberCount].id)) {
            file.ignore(numeric_limits<streamsize>::max(), '\n');
            getline(file, members[memberCount].name);
            getline(file, members[memberCount].email);
            memberCount++;
        }
    }

    void saveIssues(int issueBooks[], int issueMembers[], int count) {
        ofstream file("issued.txt");
        for (int i = 0; i < count; i++)
            file << issueBooks[i] << " " << issueMembers[i] << '\n';
    }

    int loadIssues(int issueBooks[], int issueMembers[]) {
        ifstream file("issued.txt");
        int count = 0, bookID, memberID;

        while (count < MAX && file >> bookID >> memberID) {
            int bi = findBook(bookID);
            int mi = findMember(memberID);

            if (bi != -1 && mi != -1 && !books[bi].available) {
                issueBooks[count] = bookID;
                issueMembers[count] = memberID;
                count++;
            }
        }
        return count;
    }

    string lower(string s) {
        for (char& c : s)
            c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        return s;
    }

public:
    Library() {
        loadBooks();
        loadMembers();
    }

    void addBook() {
        if (bookCount >= MAX) {
            cout << "Book storage is full!\n";
            return;
        }

        Book b;
        cout << "Enter Book ID: ";
        cin >> b.id;

        if (findBook(b.id) != -1) {
            cout << "Book ID already exists!\n";
            return;
        }

        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Enter Title: ";
        getline(cin, b.title);
        cout << "Enter Author: ";
        getline(cin, b.author);

        if (b.title.empty() || b.author.empty()) {
            cout << "Title and author cannot be empty!\n";
            return;
        }

        b.available = true;
        books[bookCount++] = b;
        saveBooks();
        cout << "Book added successfully!\n";
    }

    void displayBooks() {
        if (bookCount == 0) {
            cout << "No books found.\n";
            return;
        }

        cout << "\n------ BOOK LIST ------\n";
        for (int i = 0; i < bookCount; i++) {
            cout << "\nID: " << books[i].id
                 << "\nTitle: " << books[i].title
                 << "\nAuthor: " << books[i].author
                 << "\nStatus: "
                 << (books[i].available ? "Available" : "Issued")
                 << "\n";
        }
    }

    void searchBook() {
        int id;
        cout << "Enter Book ID: ";
        cin >> id;

        int i = findBook(id);
        if (i == -1) {
            cout << "Book not found!\n";
            return;
        }

        cout << "\nTitle: " << books[i].title
             << "\nAuthor: " << books[i].author
             << "\nStatus: "
             << (books[i].available ? "Available" : "Issued")
             << '\n';
    }

    void searchBookByName() {
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        string key;
        cout << "Enter title or author to search: ";
        getline(cin, key);
        key = lower(key);

        bool found = false;
        for (int i = 0; i < bookCount; i++) {
            if (lower(books[i].title).find(key) != string::npos ||
                lower(books[i].author).find(key) != string::npos) {
                cout << "\nID: " << books[i].id
                     << "\nTitle: " << books[i].title
                     << "\nAuthor: " << books[i].author
                     << "\nStatus: "
                     << (books[i].available ? "Available" : "Issued")
                     << '\n';
                found = true;
            }
        }

        if (!found) cout << "No matching book found!\n";
    }

    void updateBook() {
        int id;
        cout << "Enter Book ID to update: ";
        cin >> id;

        int i = findBook(id);
        if (i == -1) {
            cout << "Book not found!\n";
            return;
        }

        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Enter new title: ";
        getline(cin, books[i].title);
        cout << "Enter new author: ";
        getline(cin, books[i].author);

        if (books[i].title.empty() || books[i].author.empty()) {
            cout << "Invalid details. Update cancelled.\n";
            return;
        }

        saveBooks();
        cout << "Book updated successfully!\n";
    }

    void deleteBook() {
        int id;
        cout << "Enter Book ID to delete: ";
        cin >> id;

        int i = findBook(id);
        if (i == -1) {
            cout << "Book not found!\n";
            return;
        }

        if (!books[i].available) {
            cout << "Cannot delete an issued book!\n";
            return;
        }

        for (int j = i; j < bookCount - 1; j++)
            books[j] = books[j + 1];

        bookCount--;
        saveBooks();
        cout << "Book deleted successfully!\n";
    }

    void registerMember() {
        if (memberCount >= MAX) {
            cout << "Member storage is full!\n";
            return;
        }

        Member m;
        cout << "Enter Member ID: ";
        cin >> m.id;

        if (findMember(m.id) != -1) {
            cout << "Member ID already exists!\n";
            return;
        }

        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Enter Name: ";
        getline(cin, m.name);
        cout << "Enter Email: ";
        getline(cin, m.email);

        if (m.name.empty() || m.email.empty()) {
            cout << "Name and email cannot be empty!\n";
            return;
        }

        members[memberCount++] = m;
        saveMembers();
        cout << "Member registered successfully!\n";
    }

    void displayMembers() {
        if (memberCount == 0) {
            cout << "No members found.\n";
            return;
        }

        cout << "\n------ MEMBER LIST ------\n";
        for (int i = 0; i < memberCount; i++) {
            cout << "\nID: " << members[i].id
                 << "\nName: " << members[i].name
                 << "\nEmail: " << members[i].email << '\n';
        }
    }

    void searchMember() {
        int id;
        cout << "Enter Member ID: ";
        cin >> id;

        int i = findMember(id);
        if (i == -1) {
            cout << "Member not found!\n";
            return;
        }

        cout << "\nName: " << members[i].name
             << "\nEmail: " << members[i].email << '\n';
    }

    void updateMember() {
        int id;
        cout << "Enter Member ID to update: ";
        cin >> id;

        int i = findMember(id);
        if (i == -1) {
            cout << "Member not found!\n";
            return;
        }

        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Enter new name: ";
        getline(cin, members[i].name);
        cout << "Enter new email: ";
        getline(cin, members[i].email);

        if (members[i].name.empty() || members[i].email.empty()) {
            cout << "Invalid details. Update cancelled.\n";
            return;
        }

        saveMembers();
        cout << "Member updated successfully!\n";
    }

    void issueBook() {
        int issueBooks[MAX], issueMembers[MAX];
        int count = loadIssues(issueBooks, issueMembers);

        if (count >= MAX) {
            cout << "Issue record storage is full!\n";
            return;
        }

        int bookID, memberID;
        cout << "Enter Book ID: ";
        cin >> bookID;
        cout << "Enter Member ID: ";
        cin >> memberID;

        int bi = findBook(bookID);
        int mi = findMember(memberID);

        if (bi == -1) {
            cout << "Book not found!\n";
            return;
        }

        if (mi == -1) {
            cout << "Member not found! Register member first.\n";
            return;
        }

        if (!books[bi].available) {
            cout << "Book is already issued!\n";
            return;
        }

        books[bi].available = false;
        issueBooks[count] = bookID;
        issueMembers[count] = memberID;
        count++;

        saveBooks();
        saveIssues(issueBooks, issueMembers, count);
        logTransaction("ISSUED", bookID, memberID);

        cout << "Book issued successfully!\n";
    }

    void returnBook() {
        int issueBooks[MAX], issueMembers[MAX];
        int count = loadIssues(issueBooks, issueMembers);

        int bookID;
        cout << "Enter Book ID to return: ";
        cin >> bookID;

        int bi = findBook(bookID);
        if (bi == -1) {
            cout << "Book not found!\n";
            return;
        }

        if (books[bi].available) {
            cout << "Book is not currently issued!\n";
            return;
        }

        int pos = -1;
        for (int i = 0; i < count; i++) {
            if (issueBooks[i] == bookID) {
                pos = i;
                break;
            }
        }

        if (pos == -1) {
            cout << "Issue record not found. Check issued.txt.\n";
            return;
        }

        int days;
        cout << "Enter overdue days (0 if returned on time): ";
        cin >> days;

        if (days < 0) {
            cout << "Invalid number of days!\n";
            return;
        }

        int fine = days * FINE_PER_DAY;
        int memberID = issueMembers[pos];

        books[bi].available = true;

        for (int i = pos; i < count - 1; i++) {
            issueBooks[i] = issueBooks[i + 1];
            issueMembers[i] = issueMembers[i + 1];
        }
        count--;

        saveBooks();
        saveIssues(issueBooks, issueMembers, count);
        logTransaction("RETURNED", bookID, memberID, days, fine);

        cout << "Book returned successfully!\n";
        cout << "Fine: Rs." << fine << '\n';
    }

    void transactionHistory() {
        ifstream file("transactions.txt");
        if (!file) {
            cout << "No transaction history found.\n";
            return;
        }

        string line;
        cout << "\n------ TRANSACTION HISTORY ------\n";
        while (getline(file, line))
            cout << line << '\n';
    }

    void reports() {
        int available = 0, issued = 0;

        for (int i = 0; i < bookCount; i++) {
            if (books[i].available) available++;
            else issued++;
        }

        cout << "\n------ LIBRARY REPORT ------\n";
        cout << "Total Books: " << bookCount << '\n';
        cout << "Available Books: " << available << '\n';
        cout << "Issued Books: " << issued << '\n';
        cout << "Total Members: " << memberCount << '\n';
        cout << "Fine Rate: Rs." << FINE_PER_DAY
             << " per overdue day\n";
    }
};

bool adminLogin() {
    string username, password;

    cout << "========== ADMIN LOGIN ==========\n";
    cout << "Username: ";
    cin >> username;
    cout << "Password: ";
    cin >> password;

    if (username == "admin" && password == "admin123") {
        cout << "\nLogin successful!\n";
        return true;
    }

    cout << "\nInvalid username or password!\n";
    return false;
}

int main() {
    if (!adminLogin()) return 0;

    Library library;
    int choice;

    do {
        cout << "\n========== LIBRARY MANAGEMENT SYSTEM ==========\n";
        cout << "1. Add New Book\n";
        cout << "2. Display All Books\n";
        cout << "3. Search Book by ID\n";
        cout << "4. Search Book by Title/Author\n";
        cout << "5. Update Book Details\n";
        cout << "6. Delete Book\n";
        cout << "7. Register New Member\n";
        cout << "8. Display All Members\n";
        cout << "9. Search Member\n";
        cout << "10. Update Member Details\n";
        cout << "11. Issue Book\n";
        cout << "12. Return Book\n";
        cout << "13. Transaction History\n";
        cout << "14. Library Reports\n";
        cout << "15. Exit\n";
        cout << "Enter your choice: ";

        if (!(cin >> choice)) {
            cout << "Invalid input! Enter a number.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        switch (choice) {
            case 1: library.addBook(); break;
            case 2: library.displayBooks(); break;
            case 3: library.searchBook(); break;
            case 4: library.searchBookByName(); break;
            case 5: library.updateBook(); break;
            case 6: library.deleteBook(); break;
            case 7: library.registerMember(); break;
            case 8: library.displayMembers(); break;
            case 9: library.searchMember(); break;
            case 10: library.updateMember(); break;
            case 11: library.issueBook(); break;
            case 12: library.returnBook(); break;
            case 13: library.transactionHistory(); break;
            case 14: library.reports(); break;
            case 15: cout << "Thank you for using the system!\n"; break;
            default: cout << "Invalid choice! Try again.\n";
        }

    } while (choice != 15);

    return 0;
}