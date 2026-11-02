#include <iostream>
#include <string>
#include <fstream>
using namespace std;

class Book
{
private:
    int bookID;
    string title;
    string author;
    bool available;

public:
    // Default Constructor
    Book()
    {
        bookID = 0;
        title = "";
        author = "";
        available = true;
    }
    // Parameterized Constructor
    Book(int id, string t, string a)
    {
        bookID = id;
        title = t;
        author = a;
        available = true;
    }

    // Display Book details
    void displayDetails()
    {
        cout << "Book ID: " << bookID << endl;
        cout << "Title: " << title << endl;
        cout << "Author: " << author << endl;

        if (available)
        {
            cout << "Status: Available" << endl;
        }
        else
        {
            cout << "Status: Issued" << endl;
        }
        cout << "-----------------------" << endl;
    }

    // Return book ID
    int getBookID()
    {
        return bookID;
    }
    string getTitle()
    {
        return title;
    }
    string getAuthor()
    {
        return author;
    }

    // Check availability
    bool isAvailable()
    {
        return available;
    }

    // Change availability
    void setAvailability(bool status)
    {
        available = status;
    }

    void setBookDetails(int id, string t, string a, bool status)
    {
        bookID = id;
        title = t;
        author = a;
        available = status;
    }
};

void saveBooks(Book books[], int bookCount)
{
    ofstream file("books.txt");

    for (int i = 0; i < bookCount; i++)
    {
        file << books[i].getBookID() << endl;
        file << books[i].getTitle() << endl;
        file << books[i].getAuthor() << endl;
        file << books[i].isAvailable() << endl;
    }
    file.close();

    cout << "Books saved successfully!" << endl;
}

int loadBooks(Book books[])
{
    ifstream file("books.txt");

    if (!file)
    {
        return 0;
    }
    int count = 0;
    int id, status;
    string title, author;

    while (count < 100 && file >> id)
    {
        file.ignore();

        getline(file, title);
        getline(file, author);

        file >> status;
        file.ignore();

        books[count].setBookDetails(id, title, author, status);
        count++;
    }
    file.close();

    return count;
}

int main()
{

    Book books[100];
    int bookCount = 0;
    int choice;

    bookCount = loadBooks(books);

    do
    {
        cout << "=============================" << endl;
        cout << "LIBRARY MANAGEMENT SYSTEM" << endl;
        cout << "=============================" << endl;
        cout << "1. Add New Books" << endl;
        cout << "2. Display All Books" << endl;
        cout << "3. Search Book by ID" << endl;
        cout << "4. Issue Book" << endl;
        cout << "5. Return Book" << endl;
        cout << "6. Exit" << endl;

        cout << "\nEnter your choice: ";
        cin >> choice;

        if (choice == 1)
        {
            if (bookCount >= 100)
            {
                cout << "Library is full!" << endl;
            }
            else
            {
                int id;
                string bookTitle, bookAuthor;

                cout << "\nEnter Book ID: ";
                cin >> id;

                bool exists = false;

                for (int i = 0; i < bookCount; i++)
                {
                    if (books[i].getBookID() == id)
                    {
                        cout << "\nBook Found:\n";
                        exists = true;
                        break;
                    }
                }
                if (exists)
                {
                    cout << "Book ID already exists!" << endl;
                }
                else
                {
                    cin.ignore();

                    cout << "Enter Book Title: ";
                    getline(cin, bookTitle);
                    cout << "Enter Author Name: ";
                    getline(cin, bookAuthor);

                    books[bookCount] = Book(id, bookTitle, bookAuthor);
                    bookCount++;
                    saveBooks(books, bookCount);

                    cout << "Book added successfully!" << endl;
                }
            }
        }

        else if (choice == 2)
        {
            cout << "\n=======All Books=======" << endl;
            for (int i = 0; i < bookCount; i++)
            {
                books[i].displayDetails();
            }
        }
        else if (choice == 3)
        {
            int searchID;
            bool found = false;

            cout << "\nEnter Book ID to search: ";
            cin >> searchID;

            for (int i = 0; i < bookCount; i++)
            {
                if (books[i].getBookID() == searchID)
                {
                    cout << "\nBook Found:\n";
                    books[i].displayDetails();
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                cout << "\nBook not found!\n";
            }
        }
        else if (choice == 4)
        {
            int id;
            bool found = false;

            cout << "Enter Book ID to issue: ";
            cin >> id;

            for (int i = 0; i < bookCount; i++)
            {
                if (books[i].getBookID() == id)
                {
                    found = true;
                    if (books[i].isAvailable())
                    {
                        books[i].setAvailability(false);
                        saveBooks(books, bookCount);
                        cout << "Book issued successfully!" << endl;
                    }
                    else
                    {
                        cout << "Book is already issued!" << endl;
                    }
                    break;
                }
            }
            if (!found)
            {
                cout << "Book not found!" << endl;
            }
        }

        else if (choice == 5)
        {
            int id;
            bool found = false;

            cout << "Enter Book ID to return: ";
            cin >> id;

            for (int i = 0; i < bookCount; i++)
            {
                if (books[i].getBookID() == id)
                {
                    found = true;

                    if (!books[i].isAvailable())
                    {
                        books[i].setAvailability(true);
                        saveBooks(books, bookCount);
                        cout << "Book returned successfully!" << endl;
                    }
                    else
                    {
                        cout << "This book is already available!" << endl;
                    }
                    break;
                }
            }
            if (!found)
            {
                cout << "Book not found!" << endl;
            }
        }

        else if (choice == 6)
        {
            cout << "Exiting Library Management System..." << endl;
        }

        else
        {
            cout << "Invalid choice! Try again.\n";
        }

    } while (choice != 6);

    return 0;
}