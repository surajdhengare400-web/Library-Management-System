#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QLabel>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QStackedWidget>
#include <QSizePolicy>
#include <QStringList>
#include <QList>
#include <QScrollArea>

// =====================================================
// CUSTOM INPUT FORM
// =====================================================

static bool showInputForm(QWidget *parent,
                          const QString &title,
                          const QStringList &labels,
                          QStringList &values,
                          const QList<int> &passwordFields = {})
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    dialog.setMinimumWidth(430);

    dialog.setStyleSheet(R"(
        QDialog { background-color: #F4F7FC; }
        QLabel {
            color: #1E293B;
            font-size: 13px;
            background: transparent;
        }
        QLineEdit {
            background-color: white;
            color: #111827;
            border: 1px solid #CBD5E1;
            border-radius: 6px;
            padding: 9px;
            min-height: 25px;
            font-size: 14px;
        }
        QLineEdit:focus { border: 2px solid #2563EB; }
        QPushButton {
            background-color: #2563EB;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 9px 20px;
            min-width: 75px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #1D4ED8; }
    )");

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    mainLayout->setContentsMargins(25, 22, 25, 22);
    mainLayout->setSpacing(18);

    QLabel *heading = new QLabel(title);
    heading->setStyleSheet(
        "font-size: 19px; font-weight: bold; color: #172554;"
        );
    mainLayout->addWidget(heading);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(14);
    formLayout->setFieldGrowthPolicy(
        QFormLayout::AllNonFixedFieldsGrow
        );

    QList<QLineEdit *> fields;

    for (int i = 0; i < labels.size(); ++i) {
        QLineEdit *field = new QLineEdit(&dialog);
        field->setMinimumHeight(38);
        field->setPlaceholderText("Enter " + labels[i].toLower());

        if (passwordFields.contains(i))
            field->setEchoMode(QLineEdit::Password);

        formLayout->addRow(labels[i] + ":", field);
        fields.append(field);
    }

    mainLayout->addLayout(formLayout);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel
        );

    QObject::connect(
        buttons, &QDialogButtonBox::accepted,
        &dialog, &QDialog::accept
        );

    QObject::connect(
        buttons, &QDialogButtonBox::rejected,
        &dialog, &QDialog::reject
        );

    mainLayout->addWidget(buttons);

    if (!fields.isEmpty())
        fields.first()->setFocus();

    if (dialog.exec() != QDialog::Accepted)
        return false;

    values.clear();

    for (QLineEdit *field : fields)
        values.append(field->text());

    return true;
}

// =====================================================
// VALIDATION
// =====================================================

static bool getPositiveID(QWidget *parent,
                          const QString &text,
                          int &id,
                          const QString &fieldName)
{
    bool ok = false;
    int number = text.toInt(&ok);

    if (!ok || number <= 0) {
        QMessageBox::warning(
            parent, "Invalid ID",
            "Please enter a valid positive numeric " + fieldName + "."
            );
        return false;
    }

    id = number;
    return true;
}

static bool getNonNegativeDays(QWidget *parent,
                               const QString &text,
                               int &days)
{
    bool ok = false;
    int number = text.toInt(&ok);

    if (!ok || number < 0) {
        QMessageBox::warning(
            parent, "Invalid Days",
            "Please enter a valid number of overdue days (0 or more)."
            );
        return false;
    }

    days = number;
    return true;
}

// =====================================================
// MAIN WINDOW
// =====================================================

MainWindow::MainWindow(const AdminAccount &admin, QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    currentAdmin(admin)
{
    ui->setupUi(this);

    QString adminLoadError;

    if (!adminManager.load(&adminLoadError)) {
        QMessageBox::critical(
            this,
            "Administrator Data Error",
            adminLoadError
            );
        close();
        return;
    }

    setWindowTitle("Library Management System");
    setMinimumSize(1050, 700);
    resize(1200, 780);

    QWidget *central = ui->centralwidget;

    auto getButton = [this](const char *name) -> QPushButton * {
        return ui->centralwidget->findChild<QPushButton *>(
            QString::fromLatin1(name)
            );
    };

    // Existing library buttons

    QPushButton *addBookButton = getButton("addBookButton");
    QPushButton *displayBooksButton = getButton("displayBooksButton");
    QPushButton *searchBookButton = getButton("searchBookButton");
    QPushButton *searchByNameButton = getButton("searchByNameButton");
    QPushButton *updateBookButton = getButton("updateBookButton");
    QPushButton *deleteBookButton = getButton("deleteBookButton");

    QPushButton *registerMemberButton = getButton("registerMemberButton");
    QPushButton *displayMembersButton = getButton("displayMembersButton");
    QPushButton *searchMemberButton = getButton("searchMemberButton");
    QPushButton *updateMemberButton = getButton("updateMemberButton");

    QPushButton *issueBookButton = getButton("issueBookButton");
    QPushButton *returnBookButton = getButton("returnBookButton");

    QPushButton *transactionHistoryButton =
        getButton("transactionHistoryButton");

    QPushButton *reportsButton = getButton("reportsButton");
    QPushButton *exitButton = getButton("exitButton");

    if (central->layout())
        delete central->layout();

    for (QLabel *label : central->findChildren<QLabel *>())
        label->hide();

    // =================================================
    // ROOT LAYOUT
    // =================================================

    QHBoxLayout *root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // =================================================
    // SIDEBAR
    // =================================================

    QFrame *sidebar = new QFrame(central);
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(225);

    QVBoxLayout *sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(20, 25, 18, 22);
    sideLayout->setSpacing(9);

    QLabel *logo = new QLabel("LMS");
    logo->setObjectName("logo");

    QLabel *brand = new QLabel("LIBRARY\nMANAGEMENT");
    brand->setObjectName("brand");

    sideLayout->addWidget(logo);
    sideLayout->addWidget(brand);
    sideLayout->addSpacing(18);

    auto addHeading = [&](const QString &text) {
        QLabel *heading = new QLabel(text);
        heading->setObjectName("sideHeading");
        sideLayout->addWidget(heading);
    };

    auto addNavButton = [&](const QString &text) {
        QPushButton *button = new QPushButton(text);
        button->setObjectName("sideNavButton");
        button->setCursor(Qt::PointingHandCursor);
        button->setCheckable(true);
        button->setMinimumHeight(38);
        sideLayout->addWidget(button);
        return button;
    };

    addHeading("WORKSPACE");
    QPushButton *dashboardNav = addNavButton("▣   Dashboard");

    addHeading("CATALOG");
    QPushButton *booksNav = addNavButton("   Book Management");
    QPushButton *membersNav = addNavButton("   Members");

    addHeading("CIRCULATION");
    QPushButton *circulationNav = addNavButton("   Issue / Return");

    addHeading("INSIGHTS");
    QPushButton *historyNav = addNavButton("   Transaction History");
    QPushButton *reportsNav = addNavButton("   Library Reports");

    // Administrator navigation

    addHeading("ADMINISTRATION");

    QPushButton *adminNav = addNavButton("   Admin Management");
    QPushButton *settingsNav = addNavButton("   Account Settings");

    const bool isSuperAdmin =
        (currentAdmin.role == "Super Admin");

    if (!isSuperAdmin) {
        adminNav->hide();
    }

    sideLayout->addStretch();

    QLabel *accountFooter = new QLabel(
        currentAdmin.displayName + "\n" + currentAdmin.role
        );
    accountFooter->setObjectName("sideFooter");
    sideLayout->addWidget(accountFooter);

    root->addWidget(sidebar);

    // =================================================
    // STACKED PAGES
    // =================================================

    QStackedWidget *stack = new QStackedWidget(central);
    stack->setObjectName("mainPages");

    auto createPage = [&](const QString &title,
                          const QString &subtitle) -> QWidget * {

        QWidget *page = new QWidget();

        QVBoxLayout *layout = new QVBoxLayout(page);
        layout->setContentsMargins(32, 28, 32, 25);
        layout->setSpacing(18);

        QLabel *pageTitle = new QLabel(title);
        pageTitle->setObjectName("pageTitle");

        QLabel *pageSubtitle = new QLabel(subtitle);
        pageSubtitle->setObjectName("subtitle");

        layout->addWidget(pageTitle);
        layout->addWidget(pageSubtitle);

        return page;
    };

    auto createActionCard = [&](QWidget *page,
                                const QString &heading,
                                const QList<QPushButton *> &buttons) {

        QFrame *card = new QFrame(page);
        card->setObjectName("dashboardCard");

        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(18, 18, 18, 18);
        cardLayout->setSpacing(15);

        QLabel *title = new QLabel(heading);
        title->setObjectName("cardTitle");
        cardLayout->addWidget(title);

        QGridLayout *grid = new QGridLayout();
        grid->setSpacing(12);

        int index = 0;

        for (QPushButton *button : buttons) {
            if (!button)
                continue;

            button->setMinimumHeight(55);
            button->setSizePolicy(
                QSizePolicy::Expanding,
                QSizePolicy::Preferred
                );

            grid->addWidget(button, index / 2, index % 2);
            index++;
        }

        cardLayout->addLayout(grid);
        return card;
    };

    // =================================================
    // PAGE 1: DASHBOARD
    // =================================================

    QWidget *dashboardPage = createPage(
        "Administrator Dashboard",
        "Welcome to your Library Management System"
        );

    QVBoxLayout *dashboardLayout =
        qobject_cast<QVBoxLayout *>(dashboardPage->layout());

    QLabel *welcomeLabel = new QLabel(
        "Logged in as: " + currentAdmin.displayName +
        "  |  " + currentAdmin.role
        );
    welcomeLabel->setObjectName("subtitle");

    dashboardLayout->addWidget(welcomeLabel);

    QGridLayout *overviewGrid = new QGridLayout();
    overviewGrid->setSpacing(16);

    auto createOverviewCard = [&](const QString &heading,
                                  const QString &description,
                                  const QString &buttonText) {

        QFrame *card = new QFrame(dashboardPage);
        card->setObjectName("dashboardCard");

        QVBoxLayout *layout = new QVBoxLayout(card);
        layout->setContentsMargins(20, 20, 20, 20);
        layout->setSpacing(12);

        QLabel *title = new QLabel(heading);
        title->setObjectName("cardTitle");

        QLabel *detail = new QLabel(description);
        detail->setObjectName("subtitle");
        detail->setWordWrap(true);

        QPushButton *openButton = new QPushButton(buttonText);
        openButton->setObjectName("overviewButton");
        openButton->setMinimumHeight(45);

        layout->addWidget(title);
        layout->addWidget(detail);
        layout->addStretch();
        layout->addWidget(openButton);

        return qMakePair(card, openButton);
    };

    auto bookOverview = createOverviewCard(
        "BOOK MANAGEMENT",
        "Add, display, search, update and delete books.",
        "Open Book Management"
        );

    auto memberOverview = createOverviewCard(
        "MEMBER MANAGEMENT",
        "Register and manage library members.",
        "Open Members"
        );

    auto circulationOverview = createOverviewCard(
        "BOOK CIRCULATION",
        "Issue books and process returns.",
        "Open Issue / Return"
        );

    auto insightsOverview = createOverviewCard(
        "REPORTS & HISTORY",
        "View transaction history and library reports.",
        "Open Insights"
        );

    overviewGrid->addWidget(bookOverview.first, 0, 0);
    overviewGrid->addWidget(memberOverview.first, 0, 1);
    overviewGrid->addWidget(circulationOverview.first, 1, 0);
    overviewGrid->addWidget(insightsOverview.first, 1, 1);

    overviewGrid->setColumnStretch(0, 1);
    overviewGrid->setColumnStretch(1, 1);

    dashboardLayout->addLayout(overviewGrid);
    dashboardLayout->addStretch();

    if (exitButton) {
        exitButton->setMinimumHeight(45);
        exitButton->setMaximumWidth(170);
        dashboardLayout->addWidget(exitButton, 0, Qt::AlignRight);
    }

    // =================================================
    // PAGE 2: BOOK MANAGEMENT
    // =================================================

    QWidget *booksPage = createPage(
        "Book Management",
        "Manage the library book catalogue"
        );

    QVBoxLayout *booksLayout =
        qobject_cast<QVBoxLayout *>(booksPage->layout());

    booksLayout->addWidget(createActionCard(
        booksPage,
        "BOOK OPERATIONS",
        {
            addBookButton,
            displayBooksButton,
            searchBookButton,
            searchByNameButton,
            updateBookButton,
            deleteBookButton
        }
        ));

    booksLayout->addStretch();

    // =================================================
    // PAGE 3: MEMBERS
    // =================================================

    QWidget *membersPage = createPage(
        "Member Management",
        "Register and manage library members"
        );

    QVBoxLayout *membersLayout =
        qobject_cast<QVBoxLayout *>(membersPage->layout());

    membersLayout->addWidget(createActionCard(
        membersPage,
        "MEMBER OPERATIONS",
        {
            registerMemberButton,
            displayMembersButton,
            searchMemberButton,
            updateMemberButton
        }
        ));

    membersLayout->addStretch();

    // =================================================
    // PAGE 4: CIRCULATION
    // =================================================

    QWidget *circulationPage = createPage(
        "Book Circulation",
        "Manage book issues and returns"
        );

    QVBoxLayout *circulationLayout =
        qobject_cast<QVBoxLayout *>(circulationPage->layout());

    circulationLayout->addWidget(createActionCard(
        circulationPage,
        "ISSUE AND RETURN OPERATIONS",
        {
            issueBookButton,
            returnBookButton
        }
        ));

    circulationLayout->addStretch();

    // =================================================
    // PAGE 5: INSIGHTS
    // =================================================

    QWidget *insightsPage = createPage(
        "Insights & Reports",
        "Review library activity and transaction records"
        );

    QVBoxLayout *insightsLayout =
        qobject_cast<QVBoxLayout *>(insightsPage->layout());

    insightsLayout->addWidget(createActionCard(
        insightsPage,
        "REPORTS AND HISTORY",
        {
            transactionHistoryButton,
            reportsButton
        }
        ));

    insightsLayout->addStretch();

    // =================================================
    // PAGE 6: ADMIN MANAGEMENT
    // =================================================

    QWidget *adminPage = createPage(
        "Admin Management",
        "Manage administrator accounts and access"
        );

    QVBoxLayout *adminLayout =
        qobject_cast<QVBoxLayout *>(adminPage->layout());

    QLabel *adminInfo = new QLabel(
        "Only Super Admin accounts can manage administrators."
        );
    adminInfo->setObjectName("subtitle");
    adminInfo->setWordWrap(true);
    adminLayout->addWidget(adminInfo);

    QPushButton *addAdminButton =
        new QPushButton("Add New Administrator");

    QPushButton *viewAdminsButton =
        new QPushButton("View All Administrators");

    QPushButton *deactivateAdminButton =
        new QPushButton("Deactivate Administrator");

    adminLayout->addWidget(createActionCard(
        adminPage,
        "ADMINISTRATOR OPERATIONS",
        {
            addAdminButton,
            viewAdminsButton,
            deactivateAdminButton
        }
        ));

    adminLayout->addStretch();

    // =================================================
    // PAGE 7: ACCOUNT SETTINGS
    // =================================================

    QWidget *settingsPage = createPage(
        "Account Settings",
        "Manage your personal administrator account"
        );

    QVBoxLayout *settingsLayout =
        qobject_cast<QVBoxLayout *>(settingsPage->layout());

    QLabel *profileInfo = new QLabel(
        "Username: " + currentAdmin.username +
        "\nRole: " + currentAdmin.role
        );
    profileInfo->setObjectName("subtitle");

    settingsLayout->addWidget(profileInfo);

    QPushButton *updateProfileButton =
        new QPushButton("Update Profile");

    QPushButton *changePasswordButton =
        new QPushButton("Change Password");

    settingsLayout->addWidget(createActionCard(
        settingsPage,
        "PERSONAL ACCOUNT",
        {
            updateProfileButton,
            changePasswordButton
        }
        ));

    settingsLayout->addStretch();

    // =================================================
    // ADD ALL PAGES
    // =================================================

    stack->addWidget(dashboardPage);
    stack->addWidget(booksPage);
    stack->addWidget(membersPage);
    stack->addWidget(circulationPage);
    stack->addWidget(insightsPage);
    stack->addWidget(adminPage);
    stack->addWidget(settingsPage);

    root->addWidget(stack, 1);

    // =================================================
    // NAVIGATION
    // =================================================

    QList<QPushButton *> navButtons = {
        dashboardNav,
        booksNav,
        membersNav,
        circulationNav,
        historyNav,
        reportsNav,
        adminNav,
        settingsNav
    };

    auto selectNav = [navButtons](QPushButton *selected) {
        for (QPushButton *button : navButtons)
            button->setChecked(button == selected);
    };

    connect(dashboardNav, &QPushButton::clicked, this, [=]() {
        selectNav(dashboardNav);
        stack->setCurrentWidget(dashboardPage);
    });

    connect(booksNav, &QPushButton::clicked, this, [=]() {
        selectNav(booksNav);
        stack->setCurrentWidget(booksPage);
    });

    connect(membersNav, &QPushButton::clicked, this, [=]() {
        selectNav(membersNav);
        stack->setCurrentWidget(membersPage);
    });

    connect(circulationNav, &QPushButton::clicked, this, [=]() {
        selectNav(circulationNav);
        stack->setCurrentWidget(circulationPage);
    });

    connect(historyNav, &QPushButton::clicked, this, [=]() {
        selectNav(historyNav);
        stack->setCurrentWidget(insightsPage);
    });

    connect(reportsNav, &QPushButton::clicked, this, [=]() {
        selectNav(reportsNav);
        stack->setCurrentWidget(insightsPage);
    });

    connect(adminNav, &QPushButton::clicked, this, [=]() {
        if (!isSuperAdmin)
            return;

        selectNav(adminNav);
        stack->setCurrentWidget(adminPage);
    });

    connect(settingsNav, &QPushButton::clicked, this, [=]() {
        selectNav(settingsNav);
        stack->setCurrentWidget(settingsPage);
    });

    connect(bookOverview.second, &QPushButton::clicked, this, [=]() {
        selectNav(booksNav);
        stack->setCurrentWidget(booksPage);
    });

    connect(memberOverview.second, &QPushButton::clicked, this, [=]() {
        selectNav(membersNav);
        stack->setCurrentWidget(membersPage);
    });

    connect(circulationOverview.second, &QPushButton::clicked, this, [=]() {
        selectNav(circulationNav);
        stack->setCurrentWidget(circulationPage);
    });

    connect(insightsOverview.second, &QPushButton::clicked, this, [=]() {
        selectNav(historyNav);
        stack->setCurrentWidget(insightsPage);
    });

    selectNav(dashboardNav);
    stack->setCurrentWidget(dashboardPage);

    // =================================================
    // ADMIN MANAGEMENT ACTIONS
    // =================================================

    connect(addAdminButton, &QPushButton::clicked, this, [this, isSuperAdmin]() {
        if (!isSuperAdmin) {
            QMessageBox::warning(
                this, "Access Denied",
                "Only a Super Admin can add administrators."
                );
            return;
        }

        QStringList values;

        if (!showInputForm(
                this,
                "Add Administrator",
                {
                    "Username",
                    "Display Name",
                    "Email",
                    "Password"
                },
                values,
                {3}
                ))
            return;

        QString error;

        if (adminManager.addAdmin(
                currentAdmin.username,
                values[0],
                values[1],
                values[2],
                values[3],
                &error
                )) {
            QMessageBox::information(
                this, "Success",
                "Administrator account created successfully."
                );
        } else {
            QMessageBox::warning(this, "Unable to Add Admin", error);
        }
    });

    connect(viewAdminsButton, &QPushButton::clicked, this, [this]() {

        QVector<AdminAccount> allAdmins = adminManager.admins();

        QString details;

        if (allAdmins.isEmpty()) {
            details = "No administrator accounts found.";
        } else {
            for (const AdminAccount &admin : allAdmins) {
                details += "Username: " + admin.username + "\n";
                details += "Name: " + admin.displayName + "\n";
                details += "Email: " + admin.email + "\n";
                details += "Role: " + admin.role + "\n";
                details += "Status: " +
                           QString(admin.active ? "Active" : "Inactive");
                details += "\n\n----------------------\n\n";
            }
        }

        QMessageBox::information(
            this,
            "All Administrators",
            details
            );
    });

    connect(deactivateAdminButton, &QPushButton::clicked, this,
            [this, isSuperAdmin]() {

                if (!isSuperAdmin) {
                    QMessageBox::warning(
                        this, "Access Denied",
                        "Only a Super Admin can deactivate accounts."
                        );
                    return;
                }

                QStringList values;

                if (!showInputForm(
                        this,
                        "Deactivate Administrator",
                        {"Username"},
                        values
                        ))
                    return;

                const QString target = values[0];

                if (QMessageBox::question(
                        this,
                        "Confirm Deactivation",
                        "Deactivate administrator '" + target + "'?",
                        QMessageBox::Yes | QMessageBox::No
                        ) != QMessageBox::Yes)
                    return;

                QString error;

                if (adminManager.deactivateAdmin(
                        currentAdmin.username,
                        target,
                        &error
                        )) {
                    QMessageBox::information(
                        this, "Success",
                        "Administrator account deactivated."
                        );
                } else {
                    QMessageBox::warning(
                        this, "Unable to Deactivate", error
                        );
                }
            });

    // =================================================
    // ACCOUNT SETTINGS: UPDATE PROFILE
    // =================================================

    connect(updateProfileButton, &QPushButton::clicked, this, [this, profileInfo, accountFooter, welcomeLabel]() {
        QStringList values;

        if (!showInputForm(
                this,
                "Update Profile",
                {"Display Name", "Email"},
                values
                ))
            return;

        QString error;

        if (adminManager.updateProfile(
                currentAdmin.username,
                values[0],
                values[1],
                &error
                )) {
            currentAdmin.displayName = values[0].trimmed();
            currentAdmin.email = values[1].trimmed();

            profileInfo->setText(
                "Username: " + currentAdmin.username +
                "\nRole: " + currentAdmin.role
                );

            accountFooter->setText(
                currentAdmin.displayName + "\n" + currentAdmin.role
                );

            welcomeLabel->setText(
                "Logged in as: " + currentAdmin.displayName +
                "  |  " + currentAdmin.role
                );

            QMessageBox::information(
                this, "Success",
                "Your profile has been updated."
                );
        } else {
            QMessageBox::warning(
                this, "Update Failed", error
                );
        }
    });

    // =================================================
    // ACCOUNT SETTINGS: CHANGE PASSWORD
    // =================================================

    connect(changePasswordButton, &QPushButton::clicked, this, [this]() {
        QStringList values;

        if (!showInputForm(
                this,
                "Change Password",
                {
                    "Current Password",
                    "New Password",
                    "Confirm New Password"
                },
                values,
                {0, 1, 2}
                ))
            return;

        if (values[1] != values[2]) {
            QMessageBox::warning(
                this, "Password Mismatch",
                "The new passwords do not match."
                );
            return;
        }

        QString error;

        if (adminManager.changePassword(
                currentAdmin.username,
                values[0],
                values[1],
                &error
                )) {
            QMessageBox::information(
                this, "Success",
                "Your password has been changed successfully."
                );
        } else {
            QMessageBox::warning(
                this, "Password Change Failed", error
                );
        }
    });

    // =================================================
    // PROFESSIONAL STYLE
    // =================================================

    setStyleSheet(R"(
        QMainWindow, QWidget {
            background-color: #F4F7FC;
            font-family: "Segoe UI";
        }

        QFrame#sidebar {
            background-color: #172554;
            border: none;
        }

        QFrame#sidebar QLabel {
            background: transparent;
            border: none;
        }

        QLabel#logo {
            background-color: #2563EB;
            color: white;
            font-size: 23px;
            font-weight: bold;
            border-radius: 12px;
            padding: 12px;
        }

        QLabel#brand {
            color: white;
            font-size: 16px;
            font-weight: bold;
        }

        QLabel#sideHeading {
            color: #93A9D0;
            font-size: 10px;
            font-weight: bold;
            margin-top: 16px;
            padding-left: 5px;
        }

        QPushButton#sideNavButton {
            background: transparent;
            color: #CBD5E1;
            border: none;
            border-radius: 7px;
            text-align: left;
            padding: 9px;
            font-size: 12px;
            font-weight: 600;
        }

        QPushButton#sideNavButton:hover {
            background-color: #294477;
            color: white;
        }

        QPushButton#sideNavButton:checked {
            background-color: #294477;
            color: white;
            border-left: 3px solid #60A5FA;
        }

        QLabel#sideFooter {
            color: #AFC0DC;
            font-size: 10px;
        }

        QLabel#pageTitle {
            font-size: 27px;
            font-weight: bold;
            color: #172554;
        }

        QLabel#subtitle {
            font-size: 13px;
            color: #64748B;
            background: transparent;
        }

        QFrame#dashboardCard {
            background-color: white;
            border: 1px solid #E2E8F0;
            border-radius: 12px;
        }

        QLabel#cardTitle {
            color: #334155;
            font-size: 13px;
            font-weight: bold;
            background: transparent;
        }

        QPushButton {
            background-color: #EFF4FF;
            color: #1E40AF;
            border: 1px solid #DCE6F7;
            border-radius: 7px;
            padding: 12px;
            min-height: 44px;
            font-size: 13px;
            font-weight: 600;
        }

        QPushButton:hover {
            background-color: #DBEAFE;
            border: 1px solid #93C5FD;
        }

        QPushButton:pressed {
            background-color: #BFDBFE;
        }

        QPushButton#overviewButton {
            background-color: #2563EB;
            color: white;
            border: none;
        }

        QPushButton#overviewButton:hover {
            background-color: #1D4ED8;
        }

        QMessageBox{
            background-color: #ffffff;
        }

        QMessageBox QLabel {

        color: #1e293b;
        background-color: transparent;
        }

        QMessageBox QPushButton{
            color: #1e293b;
            background-color: #e2e8f0;
            padding: 6px 18px;
            border-radius: 5px;
        }
    )");

    // =================================================
    // ADD BOOK
    // =================================================

    connect(addBookButton, &QPushButton::clicked, this, [this]() {
        QStringList values;

        if (!showInputForm(
                this, "Add New Book",
                {"Book ID", "Book Title", "Author"}, values
                ))
            return;

        int id;

        if (!getPositiveID(this, values[0], id, "Book ID"))
            return;

        QMessageBox::information(
            this, "Add Book",
            library.addBook(id, values[1], values[2])
            );
    });

    // =================================================
    // DISPLAY BOOKS
    // =================================================

    connect(displayBooksButton, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(
            this, "All Books", library.displayBooks()
            );
    });

    // =================================================
    // SEARCH BOOK BY ID
    // =================================================

    connect(searchBookButton, &QPushButton::clicked, this, [this]() {
        QStringList values;

        if (!showInputForm(this, "Search Book", {"Book ID"}, values))
            return;

        int id;

        if (!getPositiveID(this, values[0], id, "Book ID"))
            return;

        QMessageBox::information(
            this, "Search Result", library.searchBook(id)
            );
    });

    // =================================================
    // SEARCH BOOK BY TITLE OR AUTHOR
    // =================================================

    connect(searchByNameButton, &QPushButton::clicked, this, [this]() {
        QStringList values;

        if (!showInputForm(
                this, "Search Book", {"Title or Author"}, values
                ))
            return;

        QMessageBox::information(
            this, "Search Results",
            library.searchBookByName(values[0])
            );
    });

    // =================================================
    // UPDATE BOOK
    // =================================================

    connect(updateBookButton, &QPushButton::clicked, this, [this]() {
        QStringList values;

        if (!showInputForm(
                this, "Update Book",
                {"Book ID", "New Title", "New Author"}, values
                ))
            return;

        int id;

        if (!getPositiveID(this, values[0], id, "Book ID"))
            return;

        QMessageBox::information(
            this, "Update Book",
            library.updateBook(id, values[1], values[2])
            );
    });

    // =================================================
    // DELETE BOOK
    // =================================================

    connect(deleteBookButton, &QPushButton::clicked, this, [this]() {
        QStringList values;

        if (!showInputForm(this, "Delete Book", {"Book ID"}, values))
            return;

        int id;

        if (!getPositiveID(this, values[0], id, "Book ID"))
            return;

        if (QMessageBox::question(
                this, "Confirm Delete",
                "Are you sure you want to delete this book?",
                QMessageBox::Yes | QMessageBox::No
                ) == QMessageBox::Yes) {

            QMessageBox::information(
                this, "Delete Book", library.deleteBook(id)
                );
        }
    });

    // =================================================
    // REGISTER MEMBER
    // =================================================

    connect(registerMemberButton, &QPushButton::clicked, this, [this]() {
        QStringList values;

        if (!showInputForm(
                this, "Register New Member",
                {"Member ID", "Member Name", "Email"}, values
                ))
            return;

        int id;

        if (!getPositiveID(this, values[0], id, "Member ID"))
            return;

        QMessageBox::information(
            this, "Register Member",
            library.registerMember(id, values[1], values[2])
            );
    });

    // =================================================
    // DISPLAY MEMBERS
    // =================================================

    connect(displayMembersButton, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(
            this, "All Members", library.displayMembers()
            );
    });

    // =================================================
    // SEARCH MEMBER
    // =================================================

    connect(searchMemberButton, &QPushButton::clicked, this, [this]() {
        QStringList values;

        if (!showInputForm(
                this, "Search Member", {"Member ID"}, values
                ))
            return;

        int id;

        if (!getPositiveID(this, values[0], id, "Member ID"))
            return;

        QMessageBox::information(
            this, "Search Result", library.searchMember(id)
            );
    });

    // =================================================
    // UPDATE MEMBER
    // =================================================

    connect(updateMemberButton, &QPushButton::clicked, this, [this]() {
        QStringList values;

        if (!showInputForm(
                this, "Update Member",
                {"Member ID", "New Name", "New Email"}, values
                ))
            return;

        int id;

        if (!getPositiveID(this, values[0], id, "Member ID"))
            return;

        QMessageBox::information(
            this, "Update Member",
            library.updateMember(id, values[1], values[2])
            );
    });

    // =================================================
    // ISSUE BOOK
    // =================================================

    connect(issueBookButton, &QPushButton::clicked, this, [this]() {
        QStringList values;

        if (!showInputForm(
                this, "Issue Book", {"Book ID", "Member ID"}, values
                ))
            return;

        int bookID;
        int memberID;

        if (!getPositiveID(this, values[0], bookID, "Book ID"))
            return;

        if (!getPositiveID(this, values[1], memberID, "Member ID"))
            return;

        QMessageBox::information(
            this, "Issue Book",
            library.issueBook(bookID, memberID)
            );
    });

    // =================================================
    // RETURN BOOK
    // =================================================

    connect(returnBookButton, &QPushButton::clicked, this, [this]() {
        QStringList values;

        if (!showInputForm(
                this, "Return Book",
                {"Book ID", "Overdue Days (0 if on time)"}, values
                ))
            return;

        int bookID;
        int days;

        if (!getPositiveID(this, values[0], bookID, "Book ID"))
            return;

        if (!getNonNegativeDays(this, values[1], days))
            return;

        QMessageBox::information(
            this, "Return Book",
            library.returnBook(bookID, days)
            );
    });

    // =================================================
    // TRANSACTION HISTORY
    // =================================================

    connect(
        transactionHistoryButton,
        &QPushButton::clicked,
        this,
        [this]() {
            QMessageBox::information(
                this, "Transaction History",
                library.transactionHistory()
                );
        }
        );

    // =================================================
    // LIBRARY REPORTS
    // =================================================

    connect(reportsButton, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(
            this, "Library Reports", library.reports()
            );
    });

    // =================================================
    // EXIT
    // =================================================

    connect(exitButton, &QPushButton::clicked,
            this, &QWidget::close);
}

// =====================================================
// DESTRUCTOR
// =====================================================

MainWindow::~MainWindow()
{
    delete ui;
}