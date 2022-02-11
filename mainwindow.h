#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vector>
#include <thread>

#include "linuxsystem.h"
#include "windowssystem.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void update_cpu_usage();
    void update_cpu_temp();
    void update_disk();
    void update_memory();
    void update_networking();
    void update_uptime();
    void update_time();

private:
    Ui::MainWindow *ui;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    WindowsSystem *sysAPI;
#elif __APPLE__
#   error "Programm can not be run on macos"
#elif __linux__
    LinuxSystem *sysAPI;
#else
#   error "Unknown compiler"
#endif

    const std::vector<std::string> temp_color_dark = {"rgb(38, 162, 105)", "rgb(229, 165, 10)", "rgb(198, 70, 0)", "rgb(165, 29, 45)", "rgb(97, 53, 131)"};
    const std::vector<std::string> temp_color_light = {"rgb(87, 227, 137)", "rgb(248, 228, 92)", "rgb(255, 163, 72)", "rgb(237, 51, 59)", "rgb(192, 97, 203)"};

};
#endif // MAINWINDOW_H
