#include "mainwindow.h"
#include "adminmanager.h"

#include <QApplication>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFormLayout>
#include <QDialogButtonBox>

static bool createFirstAdmin(AdminManager &manager)
{
    QDialog dialog;
    dialog.setWindowTitle("Initial Administrator Setup");
    dialog.setMinimumWidth(400);

    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout();

    auto *username = new QLineEdit();
    auto *name = new QLineEdit();
    auto *email = new QLineEdit();
    auto *password = new QLineEdit();
    auto *confirm = new QLineEdit();

    password->setEchoMode(QLineEdit::Password);
    confirm->setEchoMode(QLineEdit::Password);

    form->addRow("Username:", username);
    form->addRow("Display name:", name);
    form->addRow("Email:", email);
    form->addRow("Password:", password);
    form->addRow("Confirm password:", confirm);

    layout->addWidget(new QLabel(
        "Create your first Super Admin account."
        ));
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel
        );

    layout->addWidget(buttons);

    QObject::connect(buttons, &QDialogButtonBox::accepted,
                     &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected,
                     &dialog, &QDialog::reject);

    while (dialog.exec() == QDialog::Accepted) {
        if (password->text() != confirm->text()) {
            QMessageBox::warning(
                &dialog, "Password mismatch",
                "The passwords do not match."
                );
            continue;
        }

        QString error;

        if (manager.createInitialSuperAdmin(
                username->text(),
                name->text(),
                email->text(),
                password->text(),
                &error)) {
            return true;
        }

        QMessageBox::warning(&dialog, "Setup failed", error);
        return false;
    }

    return false;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setStyle("Fusion");

    AdminManager manager;
    QString error;

    if (!manager.load(&error)) {
        QMessageBox::critical(
            nullptr,
            "Administrator Data Error",
            error + "\n\nThe application will close. "
                    "Your administrator data has not been replaced."
            );
        return 1;
    }

    if (!manager.hasAccounts()) {
        if (!createFirstAdmin(manager))
            return 0;
    }

    QDialog login;
    login.setWindowTitle("Library Management System - Login");
    login.setMinimumSize(420, 320);

    auto *layout = new QVBoxLayout(&login);
    auto *heading = new QLabel("LIBRARY MANAGEMENT SYSTEM");
    auto *subtitle = new QLabel("ADMINISTRATOR LOGIN");

    auto *username = new QLineEdit();
    auto *password = new QLineEdit();

    username->setPlaceholderText("Enter username");
    password->setPlaceholderText("Enter password");
    password->setEchoMode(QLineEdit::Password);

    heading->setAlignment(Qt::AlignCenter);
    subtitle->setAlignment(Qt::AlignCenter);

    auto *form = new QFormLayout();
    form->addRow("Username:", username);
    form->addRow("Password:", password);

    auto *loginButton = new QPushButton("LOGIN");
    auto *exitButton = new QPushButton("EXIT");

    layout->addWidget(heading);
    layout->addWidget(subtitle);
    layout->addLayout(form);
    layout->addWidget(loginButton);
    layout->addWidget(exitButton);

    AdminAccount currentAdmin;
    bool authenticated = false;

    QObject::connect(loginButton, &QPushButton::clicked, [&]() {
        QString loginError;

        if (manager.authenticate(
                username->text(),
                password->text(),
                currentAdmin,
                &loginError)) {
            authenticated = true;
            login.accept();
        } else {
            QMessageBox::warning(
                &login, "Login Failed", loginError
                );
            password->clear();
            password->setFocus();
        }
    });

    QObject::connect(exitButton, &QPushButton::clicked,
                     &login, &QDialog::reject);

    QObject::connect(password, &QLineEdit::returnPressed,
                     loginButton, &QPushButton::click);

    if (login.exec() != QDialog::Accepted || !authenticated)
        return 0;

    MainWindow w(currentAdmin);
    w.show();

    return a.exec();
}