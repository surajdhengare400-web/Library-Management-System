#include "adminmanager.h"
#include<utility>

#include <QFile>
#include <QDir>
#include <QSaveFile>
#include <QDataStream>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QMessageAuthenticationCode>
#include <QRegularExpression>

namespace
{
constexpr quint32 FILE_MAGIC = 0x4C4D5341;
constexpr quint16 FILE_VERSION = 1;
constexpr int PBKDF2_ITERATIONS = 120000;
constexpr int SALT_SIZE = 16;
constexpr int HASH_SIZE = 32;
constexpr int MAX_ADMINS = 1000;

const QString SUPER_ADMIN_ROLE = "Super Admin";
const QString ADMIN_ROLE = "Admin";
}

AdminManager::AdminManager()
{
    dataFilePath = QDir::current().filePath("admins.dat");
}

bool AdminManager::load(QString *error)
{
    accounts.clear();

    QFile file(dataFilePath);

    if (!file.exists()) {
        loaded = true;
        return true;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = "Unable to open the administrator data file.";

        loaded = false;
        return false;
    }

    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_5_0);

    quint32 magic = 0;
    quint16 version = 0;
    quint32 count = 0;

    in >> magic >> version >> count;

    if (in.status() != QDataStream::Ok ||
        magic != FILE_MAGIC ||
        version != FILE_VERSION ||
        count > MAX_ADMINS) {

        if (error)
            *error = "Administrator data file is invalid or corrupted.";

        loaded = false;
        return false;
    }

    QVector<StoredAdmin> loadedAccounts;

    for (quint32 i = 0; i < count; ++i) {
        StoredAdmin admin;

        in >> admin.username
            >> admin.displayName
            >> admin.email
            >> admin.role
            >> admin.salt
            >> admin.passwordHash
            >> admin.active;

        if (in.status() != QDataStream::Ok ||
            admin.username.trimmed().isEmpty() ||
            (admin.role != SUPER_ADMIN_ROLE &&
             admin.role != ADMIN_ROLE) ||
            admin.salt.size() != SALT_SIZE ||
            admin.passwordHash.size() != HASH_SIZE) {

            if (error)
                *error = "Administrator records are corrupted.";

            loaded = false;
            return false;
        }

        loadedAccounts.append(admin);
    }

    if (!file.atEnd()) {
        if (error)
            *error = "Unexpected extra data in administrator file.";

        loaded = false;
        return false;
    }

    accounts = loadedAccounts;
    loaded = true;
    return true;
}

bool AdminManager::hasAccounts() const
{
    return !accounts.isEmpty();
}

int AdminManager::findAdmin(const QString &username) const
{
    for (int i = 0; i < accounts.size(); ++i) {
        if (accounts[i].username.compare(
                username.trimmed(),
                Qt::CaseInsensitive) == 0) {
            return i;
        }
    }

    return -1;
}

QByteArray AdminManager::generateSalt()
{
    QByteArray salt;

    for (int i = 0; i < SALT_SIZE; ++i) {
        quint32 value = QRandomGenerator::system()->generate();
        salt.append(static_cast<char>(value & 0xFF));
    }

    return salt;
}

QByteArray AdminManager::hmacSha256(
    const QByteArray &key,
    const QByteArray &message)
{
    return QMessageAuthenticationCode::hash(
        message,
        key,
        QCryptographicHash::Sha256
        );
}

QByteArray AdminManager::pbkdf2(
    const QString &password,
    const QByteArray &salt)
{
    const QByteArray passwordBytes = password.toUtf8();

    QByteArray blockInput = salt;
    blockInput.append("\0\0\0\1", 4);

    QByteArray u = hmacSha256(passwordBytes, blockInput);
    QByteArray result = u;

    for (int i = 1; i < PBKDF2_ITERATIONS; ++i) {
        u = hmacSha256(passwordBytes, u);

        for (int j = 0; j < result.size(); ++j)
            result[j] = static_cast<char>(
                static_cast<unsigned char>(result[j]) ^
                static_cast<unsigned char>(u[j])
                );
    }

    return result;
}

QByteArray AdminManager::hashPassword(
    const QString &password,
    const QByteArray &salt)
{
    return pbkdf2(password, salt);
}

bool AdminManager::passwordsMatch(
    const QString &password,
    const QByteArray &salt,
    const QByteArray &storedHash)
{
    QByteArray calculated = hashPassword(password, salt);

    if (calculated.size() != storedHash.size())
        return false;

    unsigned char difference = 0;

    for (int i = 0; i < calculated.size(); ++i) {
        difference |=
            static_cast<unsigned char>(calculated[i]) ^
            static_cast<unsigned char>(storedHash[i]);
    }

    return difference == 0;
}

bool AdminManager::validPassword(
    const QString &password,
    QString *error)
{
    if (password.length() < 8) {
        if (error)
            *error = "Password must contain at least 8 characters.";
        return false;
    }

    if (password.length() > 128) {
        if (error)
            *error = "Password cannot exceed 128 characters.";
        return false;
    }

    if (password.trimmed().isEmpty()) {
        if (error)
            *error = "Password cannot be empty.";
        return false;
    }

    return true;
}

bool AdminManager::validUsername(
    const QString &username,
    QString *error)
{
    static const QRegularExpression pattern(
        "^[A-Za-z0-9_.-]{3,30}$"
        );

    if (!pattern.match(username).hasMatch()) {
        if (error)
            *error =
                "Username must be 3-30 characters and contain only "
                "letters, numbers, dots, underscores or hyphens.";

        return false;
    }

    return true;
}

bool AdminManager::save(QString *error) const
{
    QSaveFile file(dataFilePath);

    if (!file.open(QIODevice::WriteOnly)) {
        if (error)
            *error = "Unable to save administrator accounts.";

        return false;
    }

    QDataStream out(&file);
    out.setVersion(QDataStream::Qt_5_0);

    out << FILE_MAGIC
        << FILE_VERSION
        << static_cast<quint32>(accounts.size());

    for (const StoredAdmin &admin :std::as_const(accounts)) {
        out << admin.username
            << admin.displayName
            << admin.email
            << admin.role
            << admin.salt
            << admin.passwordHash
            << admin.active;
    }

    if (out.status() != QDataStream::Ok) {
        file.cancelWriting();

        if (error)
            *error = "Failed while writing administrator data.";

        return false;
    }

    if (!file.commit()) {
        if (error)
            *error = "Unable to finish saving administrator data.";

        return false;
    }

    return true;
}

bool AdminManager::authenticate(
    const QString &username,
    const QString &password,
    AdminAccount &account,
    QString *error) const
{
    if (!loaded) {
        if (error)
            *error = "Administrator data has not loaded successfully.";
        return false;
    }

    int index = findAdmin(username);

    if (index < 0 || !accounts[index].active) {
        if (error)
            *error = "Invalid username or password.";
        return false;
    }

    const StoredAdmin &stored = accounts[index];

    if (!passwordsMatch(
            password,
            stored.salt,
            stored.passwordHash)) {

        if (error)
            *error = "Invalid username or password.";

        return false;
    }

    account.username = stored.username;
    account.displayName = stored.displayName;
    account.email = stored.email;
    account.role = stored.role;
    account.active = stored.active;

    return true;
}

bool AdminManager::createInitialSuperAdmin(
    const QString &username,
    const QString &displayName,
    const QString &email,
    const QString &password,
    QString *error)
{
    if (!loaded) {
        if (error)
            *error = "Administrator data is not ready.";
        return false;
    }

    if (!accounts.isEmpty()) {
        if (error)
            *error = "An administrator account already exists.";
        return false;
    }

    if (!validUsername(username, error) ||
        !validPassword(password, error))
        return false;

    if (displayName.trimmed().isEmpty()) {
        if (error)
            *error = "Display name cannot be empty.";
        return false;
    }

    StoredAdmin admin;
    admin.username = username.trimmed();
    admin.displayName = displayName.trimmed();
    admin.email = email.trimmed();
    admin.role = SUPER_ADMIN_ROLE;
    admin.salt = generateSalt();
    admin.passwordHash = hashPassword(password, admin.salt);
    admin.active = true;

    accounts.append(admin);

    if (!save(error)) {
        accounts.clear();
        return false;
    }

    return true;
}

bool AdminManager::addAdmin(
    const QString &actorUsername,
    const QString &username,
    const QString &displayName,
    const QString &email,
    const QString &password,
    QString *error)
{
    int actorIndex = findAdmin(actorUsername);

    if (actorIndex < 0 ||
        !accounts[actorIndex].active ||
        accounts[actorIndex].role != SUPER_ADMIN_ROLE) {

        if (error)
            *error = "Only an active Super Admin can add administrators.";

        return false;
    }

    if (accounts.size() >= MAX_ADMINS) {
        if (error)
            *error = "Maximum administrator limit reached.";
        return false;
    }

    if (!validUsername(username, error) ||
        !validPassword(password, error))
        return false;

    if (findAdmin(username) >= 0) {
        if (error)
            *error = "That username is already in use.";
        return false;
    }

    if (displayName.trimmed().isEmpty()) {
        if (error)
            *error = "Display name cannot be empty.";
        return false;
    }

    StoredAdmin admin;
    admin.username = username.trimmed();
    admin.displayName = displayName.trimmed();
    admin.email = email.trimmed();
    admin.role = ADMIN_ROLE;
    admin.salt = generateSalt();
    admin.passwordHash = hashPassword(password, admin.salt);
    admin.active = true;

    accounts.append(admin);

    if (!save(error)) {
        accounts.removeLast();
        return false;
    }

    return true;
}

bool AdminManager::changePassword(
    const QString &username,
    const QString &currentPassword,
    const QString &newPassword,
    QString *error)
{
    int index = findAdmin(username);

    if (index < 0 || !accounts[index].active) {
        if (error)
            *error = "Administrator account not found.";
        return false;
    }

    StoredAdmin &admin = accounts[index];

    if (!passwordsMatch(
            currentPassword,
            admin.salt,
            admin.passwordHash)) {

        if (error)
            *error = "Current password is incorrect.";

        return false;
    }

    if (!validPassword(newPassword, error))
        return false;

    QByteArray oldSalt = admin.salt;
    QByteArray oldHash = admin.passwordHash;

    admin.salt = generateSalt();
    admin.passwordHash = hashPassword(newPassword, admin.salt);

    if (!save(error)) {
        admin.salt = oldSalt;
        admin.passwordHash = oldHash;
        return false;
    }

    return true;
}

bool AdminManager::updateProfile(
    const QString &username,
    const QString &newDisplayName,
    const QString &newEmail,
    QString *error)
{
    int index = findAdmin(username);

    if (index < 0 || !accounts[index].active) {
        if (error)
            *error = "Administrator account not found.";
        return false;
    }

    if (newDisplayName.trimmed().isEmpty()) {
        if (error)
            *error = "Display name cannot be empty.";
        return false;
    }

    QString oldName = accounts[index].displayName;
    QString oldEmail = accounts[index].email;

    accounts[index].displayName = newDisplayName.trimmed();
    accounts[index].email = newEmail.trimmed();

    if (!save(error)) {
        accounts[index].displayName = oldName;
        accounts[index].email = oldEmail;
        return false;
    }

    return true;
}

bool AdminManager::deactivateAdmin(
    const QString &actorUsername,
    const QString &targetUsername,
    QString *error)
{
    int actorIndex = findAdmin(actorUsername);
    int targetIndex = findAdmin(targetUsername);

    if (actorIndex < 0 ||
        !accounts[actorIndex].active ||
        accounts[actorIndex].role != SUPER_ADMIN_ROLE) {

        if (error)
            *error = "Only an active Super Admin can deactivate accounts.";

        return false;
    }

    if (targetIndex < 0 || !accounts[targetIndex].active) {
        if (error)
            *error = "Target administrator was not found or is inactive.";
        return false;
    }

    if (actorIndex == targetIndex) {
        if (error)
            *error = "You cannot deactivate your own account.";
        return false;
    }

    if (accounts[targetIndex].role == SUPER_ADMIN_ROLE) {
        int activeSuperAdmins = 0;

        for (const StoredAdmin &admin : accounts) {
            if (admin.active && admin.role == SUPER_ADMIN_ROLE)
                ++activeSuperAdmins;
        }

        if (activeSuperAdmins <= 1) {
            if (error)
                *error = "The last active Super Admin cannot be deactivated.";
            return false;
        }
    }

    accounts[targetIndex].active = false;

    if (!save(error)) {
        accounts[targetIndex].active = true;
        return false;
    }

    return true;
}

QVector<AdminAccount> AdminManager::admins() const
{
    QVector<AdminAccount> result;

    for (const StoredAdmin &stored : accounts) {
        AdminAccount account;
        account.username = stored.username;
        account.displayName = stored.displayName;
        account.email = stored.email;
        account.role = stored.role;
        account.active = stored.active;

        result.append(account);
    }

    return result;
}