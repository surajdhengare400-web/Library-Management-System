#ifndef LIBRARY_H
#define LIBRARY_H

#include <QString>
#include <QVector>

struct Book {
    int id = 0;
    QString title;
    QString author;
    bool available = true;
};

struct Member {
    int id = 0;
    QString name;
    QString email;
};

class Library {
private:
    QVector<Book> books;
    QVector<Member> members;
    QString dataPath;

    int findBook(int id) const;
    int findMember(int id) const;

    void loadBooks();
    void loadMembers();
    void saveBooks() const;
    void saveMembers() const;
    void saveIssues() const;
    void logTransaction(const QString &action, int bookID,
                        int memberID, int days = 0, int fine = 0);

public:
    Library();

    QString addBook(int id, const QString &title, const QString &author);
    QString displayBooks() const;
    QString searchBook(int id) const;
    QString searchBookByName(const QString &keyword) const;
    QString updateBook(int id, const QString &title, const QString &author);
    QString deleteBook(int id);

    QString registerMember(int id, const QString &name, const QString &email);
    QString displayMembers() const;
    QString searchMember(int id) const;
    QString updateMember(int id, const QString &name, const QString &email);

    QString issueBook(int bookID, int memberID);
    QString returnBook(int bookID, int overdueDays);

    QString transactionHistory() const;
    QString reports() const;
};

#endif