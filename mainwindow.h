#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vector>

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

    int cpu_usage();
    int cpu_temp();
    std::vector<long> sys_used_memory();
    std::vector<long long> sys_networking();
};
#endif // MAINWINDOW_H
