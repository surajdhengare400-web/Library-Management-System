#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "library.h"
#include "adminmanager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(
        const AdminAccount &currentAdmin,
        QWidget *parent = nullptr
        );

    ~MainWindow() override;

private:
    Ui::MainWindow *ui;
    Library library;
    AdminAccount currentAdmin;
    AdminManager adminManager;
};

#endif