#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vector>
#include <thread>

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

    double cpu_usage();
    int cpu_temp();
    std::vector<long> sys_used_memory();
    std::vector<long long> sys_networking();
    long long sys_disk_activity();
    void initialize_gui();

    std::vector<std::unique_ptr<std::thread>> initialize_threads;

    const std::vector<std::string> temp_color_dark = {"rgb(38, 162, 105)", "rgb(229, 165, 10)", "rgb(198, 70, 0)", "rgb(165, 29, 45)", "rgb(97, 53, 131)"};
    const std::vector<std::string> temp_color_light = {"rgb(87, 227, 137)", "rgb(248, 228, 92)", "rgb(255, 163, 72)", "rgb(237, 51, 59)", "rgb(192, 97, 203)"};

    long long received_bytes = -1;
    long long sent_bytes = -1;
    long long highest_received = 0;
    long long highest_sent = 0;

    long long last_active_time = -1;
    long long last_total_time = -1;

    long long last_read_sectors = -1;
    long long last_written_sectors = -1;
};
#endif // MAINWINDOW_H
