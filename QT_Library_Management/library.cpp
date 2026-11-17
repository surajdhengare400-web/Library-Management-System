#include "library.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>

static const int MAX_RECORDS = 100;
static const int FINE_PER_DAY = 5;

Library::Library()
{
    dataPath = QDir::currentPath();
    loadBooks();
    loadMembers();
}

int Library::findBook(int id) const
{
    for (int i = 0; i < books.size(); ++i)
        if (books[i].id == id) return i;

    return -1;
}

int Library::findMember(int id) const
{
    for (int i = 0; i < members.size(); ++i)
        if (members[i].id == id) return i;

    return -1;
}

void Library::loadBooks()
{
    QFile file(dataPath + "/books.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);

    while (!in.atEnd() && books.size() < MAX_RECORDS) {
        Book b;
        QString available;

        in >> b.id;
        in.readLine();
        if (in.atEnd()) break;

        b.title = in.readLine();
        if (in.atEnd()) break;

        b.author = in.readLine();
        if (in.atEnd()) break;

        available = in.readLine();
        b.available = (available.trimmed() == "1");

        books.append(b);
    }
}

void Library::loadMembers()
{
    QFile file(dataPath + "/members.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);

    while (!in.atEnd() && members.size() < MAX_RECORDS) {
        Member m;

        in >> m.id;
        in.readLine();
        if (in.atEnd()) break;

        m.name = in.readLine();
        if (in.atEnd()) break;

        m.email = in.readLine();
        members.append(m);
    }
}

void Library::saveBooks() const
{
    QFile file(dataPath + "/books.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&file);

    for (const Book &b : books) {
        out << b.id << '\n'
            << b.title << '\n'
            << b.author << '\n'
            << (b.available ? 1 : 0) << '\n';
    }
}

void Library::saveMembers() const
{
    QFile file(dataPath + "/members.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&file);

    for (const Member &m : members) {
        out << m.id << '\n'
            << m.name << '\n'
            << m.email << '\n';
    }
}

void Library::saveIssues() const
{
    QFile file(dataPath + "/issued.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&file);

    for (const Book &b : books) {
        if (!b.available) {
            // Find the member from the existing issue records separately.
            // This function is completed by issueBook/returnBook.
        }
    }
}

void Library::logTransaction(const QString &action, int bookID,
                             int memberID, int days, int fine)
{
    QFile file(dataPath + "/transactions.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << action
        << " | Book ID: " << bookID
        << " | Member ID: " << memberID
        << " | Date: " << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
        << " | Days overdue: " << days
        << " | Fine: Rs." << fine << '\n';
}

QString Library::addBook(int id, const QString &title, const QString &author)
{
    if (books.size() >= MAX_RECORDS)
        return "Book storage is full!";

    if (id <= 0) return "Enter a valid Book ID.";
    if (findBook(id) != -1) return "Book ID already exists!";
    if (title.trimmed().isEmpty() || author.trimmed().isEmpty())
        return "Title and author cannot be empty!";

    Book b;
    b.id = id;
    b.title = title.trimmed();
    b.author = author.trimmed();
    b.available = true;

    books.append(b);
    saveBooks();

    return "Book added successfully!";
}

QString Library::displayBooks() const
{
    if (books.isEmpty()) return "No books found.";

    QString result = "========== BOOK LIST ==========\n\n";

    for (const Book &b : books) {
        result += "ID: " + QString::number(b.id)
        + "\nTitle: " + b.title
            + "\nAuthor: " + b.author
            + "\nStatus: " + (b.available ? "Available" : "Issued")
            + "\n\n";
    }

    return result;
}

QString Library::searchBook(int id) const
{
    int i = findBook(id);
    if (i == -1) return "Book not found!";

    const Book &b = books[i];

    return "Book ID: " + QString::number(b.id)
           + "\nTitle: " + b.title
           + "\nAuthor: " + b.author
           + "\nStatus: " + (b.available ? "Available" : "Issued");
}

QString Library::searchBookByName(const QString &keyword) const
{
    QString key = keyword.trimmed();
    if (key.isEmpty()) return "Enter a title or author to search.";

    QString result;

    for (const Book &b : books) {
        if (b.title.contains(key, Qt::CaseInsensitive)
            || b.author.contains(key, Qt::CaseInsensitive)) {
            result += "ID: " + QString::number(b.id)
            + "\nTitle: " + b.title
                + "\nAuthor: " + b.author
                + "\nStatus: " + (b.available ? "Available" : "Issued")
                + "\n\n";
        }
    }

    return result.isEmpty() ? "No matching book found!" : result;
}

QString Library::updateBook(int id, const QString &title, const QString &author)
{
    int i = findBook(id);
    if (i == -1) return "Book not found!";

    if (title.trimmed().isEmpty() || author.trimmed().isEmpty())
        return "Title and author cannot be empty!";

    books[i].title = title.trimmed();
    books[i].author = author.trimmed();
    saveBooks();

    return "Book updated successfully!";
}

QString Library::deleteBook(int id)
{
    int i = findBook(id);
    if (i == -1) return "Book not found!";
    if (!books[i].available) return "Cannot delete an issued book!";

    books.removeAt(i);
    saveBooks();

    return "Book deleted successfully!";
}

QString Library::registerMember(int id, const QString &name, const QString &email)
{
    if (members.size() >= MAX_RECORDS)
        return "Member storage is full!";

    if (id <= 0) return "Enter a valid Member ID.";
    if (findMember(id) != -1) return "Member ID already exists!";
    if (name.trimmed().isEmpty() || email.trimmed().isEmpty())
        return "Name and email cannot be empty!";

    Member m;
    m.id = id;
    m.name = name.trimmed();
    m.email = email.trimmed();

    members.append(m);
    saveMembers();

    return "Member registered successfully!";
}

QString Library::displayMembers() const
{
    if (members.isEmpty()) return "No members found.";

    QString result = "========== MEMBER LIST ==========\n\n";

    for (const Member &m : members) {
        result += "ID: " + QString::number(m.id)
        + "\nName: " + m.name
            + "\nEmail: " + m.email
            + "\n\n";
    }

    return result;
}

QString Library::searchMember(int id) const
{
    int i = findMember(id);
    if (i == -1) return "Member not found!";

    const Member &m = members[i];

    return "Member ID: " + QString::number(m.id)
           + "\nName: " + m.name
           + "\nEmail: " + m.email;
}

QString Library::updateMember(int id, const QString &name, const QString &email)
{
    int i = findMember(id);
    if (i == -1) return "Member not found!";

    if (name.trimmed().isEmpty() || email.trimmed().isEmpty())
        return "Name and email cannot be empty!";

    members[i].name = name.trimmed();
    members[i].email = email.trimmed();
    saveMembers();

    return "Member updated successfully!";
}

QString Library::issueBook(int bookID, int memberID)
{
    int bi = findBook(bookID);
    int mi = findMember(memberID);

    if (bi == -1) return "Book not found!";
    if (mi == -1) return "Member not found! Register the member first.";
    if (!books[bi].available) return "Book is already issued!";

    QFile file(dataPath + "/issued.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return "Could not save the issue record.";

    QTextStream out(&file);
    out << bookID << " " << memberID << '\n';

    books[bi].available = false;
    saveBooks();
    logTransaction("ISSUED", bookID, memberID);

    return "Book issued successfully!";
}

QString Library::returnBook(int bookID, int overdueDays)
{
    int bi = findBook(bookID);

    if (bi == -1) return "Book not found!";
    if (books[bi].available) return "Book is not currently issued!";
    if (overdueDays < 0) return "Invalid number of overdue days!";

    QFile file(dataPath + "/issued.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return "Issue record not found. Check issued.txt.";

    QVector<QPair<int, int>> records;
    QTextStream in(&file);
    int memberID = -1;

    while (!in.atEnd()) {
        int b = 0, m = 0;
        in >> b >> m;
        if (in.status() != QTextStream::Ok) break;

        if (b == bookID && memberID == -1)
            memberID = m;
        else
            records.append(qMakePair(b, m));
    }
    file.close();

    if (memberID == -1)
        return "Issue record not found. Check issued.txt.";

    QFile outFile(dataPath + "/issued.txt");
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return "Could not update the issue record.";

    QTextStream out(&outFile);
    for (const auto &record : records)
        out << record.first << " " << record.second << '\n';

    books[bi].available = true;
    saveBooks();

    int fine = overdueDays * FINE_PER_DAY;
    logTransaction("RETURNED", bookID, memberID, overdueDays, fine);

    return "Book returned successfully!\nFine: Rs." + QString::number(fine);
}

QString Library::transactionHistory() const
{
    QFile file(dataPath + "/transactions.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return "No transaction history found.";

    QTextStream in(&file);
    QString result = "========== TRANSACTION HISTORY ==========\n\n";

    while (!in.atEnd())
        result += in.readLine() + '\n';

    return result.trimmed();
}

QString Library::reports() const
{
    int available = 0;
    int issued = 0;

    for (const Book &b : books) {
        if (b.available) available++;
        else issued++;
    }

    return "========== LIBRARY REPORT ==========\n\n"
           "Total Books: " + QString::number(books.size())
           + "\nAvailable Books: " + QString::number(available)
           + "\nIssued Books: " + QString::number(issued)
           + "\nTotal Members: " + QString::number(members.size())
           + "\nFine Rate: Rs.5 per overdue day";
}