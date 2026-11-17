#ifndef ADMINMANAGER_H
#define ADMINMANAGER_H

#include <QString>
#include <QVector>
#include <QByteArray>

struct AdminAccount
{
    QString username;
    QString displayName;
    QString email;
    QString role;
    bool active = true;
};

class AdminManager
{
public:
    AdminManager();

    bool load(QString *error = nullptr);
    bool hasAccounts() const;

    bool authenticate(
        const QString &username,
        const QString &password,
        AdminAccount &account,
        QString *error = nullptr
        ) const;

    bool createInitialSuperAdmin(
        const QString &username,
        const QString &displayName,
        const QString &email,
        const QString &password,
        QString *error = nullptr
        );

    bool addAdmin(
        const QString &actorUsername,
        const QString &username,
        const QString &displayName,
        const QString &email,
        const QString &password,
        QString *error = nullptr
        );

    bool changePassword(
        const QString &username,
        const QString &currentPassword,
        const QString &newPassword,
        QString *error = nullptr
        );

    bool updateProfile(
        const QString &username,
        const QString &newDisplayName,
        const QString &newEmail,
        QString *error = nullptr
        );

    bool deactivateAdmin(
        const QString &actorUsername,
        const QString &targetUsername,
        QString *error = nullptr
        );

    QVector<AdminAccount> admins() const;

private:
    struct StoredAdmin
    {
        QString username;
        QString displayName;
        QString email;
        QString role;
        QByteArray salt;
        QByteArray passwordHash;
        bool active = true;
    };

    QVector<StoredAdmin> accounts;
    QString dataFilePath;
    bool loaded = false;

    int findAdmin(const QString &username) const;
    bool save(QString *error = nullptr) const;

    static QByteArray generateSalt();
    static QByteArray hashPassword(
        const QString &password,
        const QByteArray &salt
        );

    static bool passwordsMatch(
        const QString &password,
        const QByteArray &salt,
        const QByteArray &storedHash
        );

    static bool validPassword(
        const QString &password,
        QString *error = nullptr
        );

    static bool validUsername(
        const QString &username,
        QString *error = nullptr
        );

    static QByteArray hmacSha256(
        const QByteArray &key,
        const QByteArray &message
        );

    static QByteArray pbkdf2(
        const QString &password,
        const QByteArray &salt
        );
};

#endif