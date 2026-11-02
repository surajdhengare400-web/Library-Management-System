Library Management System

Project Overview

The Library Management System is a console-based application developed using C++. It aims to simplify basic library operations through Object-Oriented Programming (OOP) concepts.

This project is being developed as part of our B.Tech CSE academic project.

Current Features

Book Management

- Add new books.
- Display all available book records.
- Search for books using Book ID.
- Issue books.
- Return books.
- Prevent duplicate Book IDs.
- Prevent issuing an already-issued book.
- Prevent returning an already-available book.
- Store book records in a text file.
- Automatically load saved records when the program starts.

OOP Concepts Used

- Classes and Objects
- Encapsulation
- Constructors
- Data Members and Member Functions

Technologies Used

- Programming Language: C++
- IDE: Visual Studio Code
- Compiler: GCC (g++)
- Version Control: Git and GitHub

How to Run

1. Clone this repository.

2. Open the project folder in Visual Studio Code.

3. Open the terminal.

4. Compile the program:
   
   g++ main.cpp -o library

5. Run the program:
   
   ./library
   
   On Windows, you can also run:
   
   ./library.exe

Data Storage

Book records are stored in "books.txt". The program loads saved records when it starts and updates the file when book information changes.

Project Status

The Book Management module has been implemented and tested. Member Management and integration with the complete Library Management System are in progress.