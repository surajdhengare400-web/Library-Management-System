Library Management System



A desktop-based Library Management System developed using C++, Qt Widgets and CMake.



Overview



The Library Management System is designed to simplify library operations through a graphical user interface. It provides tools for managing books, members, issuing and returning books, tracking transactions and generating library reports.



Features



1\. Dashboard



\- Centralized dashboard.

\- Navigation through different library modules.



2\. Book Management



\- Add new books.

\- View available books.

\- Search books.

\- Update book information.

\- Delete books.



3\. Member Management



\- Register members.

\- View member records.

\- Search and update members.



4\. Issue and Return



\- Issue books to registered members.

\- Return issued books.

\- Calculate overdue fines.



5\. Insights and Reports



\- Transaction history.

\- Library reports.



6\. Administrator Management



\- First-time Super Admin setup.

\- Multiple administrator accounts.

\- Super Admin and Admin roles.

\- Add and deactivate administrators.

\- Change password.

\- Update administrator profile.

\- Password hashing.



Technologies Used



\- Programming Language: C++

\- Framework: Qt Widgets

\- Build System: CMake

\- Language Standard: C++17

\- IDE: Qt Creator

\- Data Storage: Local files



Project Structure



LibraryManagementGUI/

│

├── main.cpp

├── mainwindow.cpp

├── mainwindow.h

├── mainwindow.ui

├── library.cpp

├── library.h

├── adminmanager.cpp

├── adminmanager.h

├── CMakeLists.txt

│

├── books.txt

├── members.txt

├── issued.txt

├── transactions.txt

└── admins.dat



How to Run



1\. Install Qt Creator and a compatible Qt version.

2\. Open the project in Qt Creator.

3\. Open "CMakeLists.txt".

4\. Configure the project using a compatible compiler.

5\. Build the project.

6\. Run the application.

7\. On first launch, create the Super Admin account.



Data Storage



The application stores library records and administrator information in local files. Data is maintained between application sessions.



Important: Keep the data files together with the application and maintain backups.



Future Improvements



\- Database integration.

\- Book barcode scanning.

\- Advanced analytics.

\- Improved report export functionality.



Developed For



B.Tech Computer Science and Engineering Project.



Project: Library Management System

Language: C++

Framework: Qt Widgets

