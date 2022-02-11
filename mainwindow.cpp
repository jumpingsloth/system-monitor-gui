#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <fstream>
#include <QTimer>
#include <QString>
#include <sstream>
#include <iostream>
#include <QDateTime>
#include <QTime>
#include <QTimeZone>
#include <vector>
#include <filesystem>
#include <regex>

#include "linuxsystem.h"

#define KB_MB_FACTOR 9.5367431640625e-7
#define BYTES_KIB_FACTOR 0.0078125
#define BYTES_MIB_FACTOR 7.6294e-6

#define BYTES_GIB 134217728
#define BYTES_KIB 128

#define NETWORKING_UPDATE_TIME 3

#define BYTES_SECTOR_SIZE 512


using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    sysAPI = new LinuxSystem();

    update_cpu_usage();
    update_cpu_temp();
    update_disk();
    update_memory();
    update_networking();
    update_uptime();
    update_time();

    QTimer *timer1 = new QTimer(this);
    connect(timer1, SIGNAL(timeout()), this, SLOT(update_cpu_usage()));
    timer1->start(1000);

    QTimer *timer2 = new QTimer(this);
    connect(timer2, SIGNAL(timeout()), this, SLOT(update_cpu_temp()));
    timer2->start(1300);

    QTimer *timer3 = new QTimer(this);
    connect(timer3, SIGNAL(timeout()), this, SLOT(update_disk()));
    timer3->start(2000);

    QTimer *timer4 = new QTimer(this);
    connect(timer4, SIGNAL(timeout()), this, SLOT(update_memory()));
    timer4->start(1300);

    QTimer *timer5 = new QTimer(this);
    connect(timer5, SIGNAL(timeout()), this, SLOT(update_networking()));
    timer5->start(NETWORKING_UPDATE_TIME * 1000);

    QTimer *timer6 = new QTimer(this);
    connect(timer6, SIGNAL(timeout()), this, SLOT(update_uptime()));
    timer6->start(1300);

    QTimer *timer7 = new QTimer(this);
    connect(timer7, SIGNAL(timeout()), this, SLOT(update_time()));
    timer7->start(50);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::update_time() {
    ui->date_label->setText(QDateTime::currentDateTime().toString("dddd MMM dd.MM.yyyy"));
    ui->clock_label->setText(QTime::currentTime().toString("t hh:mm:ss"));
}

void MainWindow::update_memory() {
    vector<long> mem_data = sysAPI->sys_used_memory();
    long used_mem_kb = mem_data[0];
    long total_mem = mem_data[1];

    double used_mem_gb = used_mem_kb * KB_MB_FACTOR;

    ui->memory_lcd->display(QString::number(used_mem_gb, 'g', 2));
    ui->memory_bar->setMinimum(0.);
    ui->memory_bar->setMaximum(total_mem);
    ui->memory_bar->setValue(used_mem_kb);
}

void MainWindow::update_disk() {
    double disk_activity = sysAPI->sys_disk_activity();

    ui->disk_bar->setMinimum(0);
    ui->disk_bar->setMaximum(100);

    if (disk_activity > 100) {
        ui->disk_lcd->display(QString::number(100));
        ui->disk_bar->setValue(100);
    } else {
        ui->disk_lcd->display(QString::number(round(disk_activity)));
        ui->disk_bar->setValue(disk_activity);
    }
}

void MainWindow::update_networking() {
    vector<long long> rs_bytes_second = sysAPI->sys_networking();

    long long received = round(rs_bytes_second[0] / NETWORKING_UPDATE_TIME);
    long long sent = round(rs_bytes_second[1] / NETWORKING_UPDATE_TIME);


    ui->networkR_bar->setFormat("% of 1 Gbit/s");

    ui->networkR_bar->setMinimum(0);
    ui->networkR_bar->setMaximum(BYTES_GIB);

    ui->networkR_bar->setValue(received);

    ui->networkS_bar->setFormat("% of 1 Gbit/s");

    ui->networkS_bar->setMinimum(0);
    ui->networkS_bar->setMaximum(BYTES_GIB);

    ui->networkS_bar->setValue(sent);



    if (received > BYTES_KIB * 500) {
        ui->networkR_lcd->display(QString::number(received * BYTES_MIB_FACTOR, 'g', 2));
        ui->networkR_label->setText("Network Rec. 🠗 Mbit/s");
    } else if (received > 500) {
        ui->networkR_lcd->display(QString::number(received * BYTES_KIB_FACTOR, 'g', 2));
        ui->networkR_label->setText("Network Rec. 🠗 Kbit/s");
    } else {
        ui->networkR_lcd->display(QString::number(received));
        ui->networkR_label->setText("Network Rec. 🠗 Bytes/s");
    }


    if (sent > BYTES_KIB * 500) {
        ui->networkS_lcd->display(QString::number(sent * BYTES_MIB_FACTOR, 'g', 2));
        ui->networkS_label->setText("Network Send 🠕 Mbit/s");
    } else if (sent > 500) {
        ui->networkS_lcd->display(QString::number(sent * BYTES_KIB_FACTOR, 'g', 2));
        ui->networkS_label->setText("Network Send 🠕 Kbit/s");
    } else {
        ui->networkS_lcd->display(QString::number(sent));
        ui->networkS_label->setText("Network Send 🠕 Bytes/s");
    }

}

void MainWindow::update_cpu_usage() {

    // update cpu usage

    double cpu = sysAPI->cpu_usage();
    ui->processor_lcd->display(QString::number(cpu, 'g', 2));
    ui->processor_bar->setMinimum(0);
    ui->processor_bar->setMaximum(100);
    ui->processor_bar->setValue(cpu);
}

void MainWindow::update_uptime() {

    // update uptime
    chrono::milliseconds uptime(0u);
    double uptime_seconds;
    if (ifstream("/proc/uptime", ios::in) >> uptime_seconds) {
        uptime = chrono::milliseconds(static_cast<unsigned long long>(uptime_seconds*1000.0));
    }
    int uptime_minutes = uptime_seconds / 60;
    ui->uptime_lcd->display(QString::number((int)uptime_minutes));
    ui->uptime_bar->setMinimum(0);
    double day_minutes = 24 * 60;
    ui->uptime_bar->setMaximum(day_minutes);
    int bar_value = uptime_minutes - (day_minutes * (ceil(uptime_minutes / day_minutes) - 1));
    ui->uptime_bar->setFormat(QString::number((int)(100 * (bar_value / day_minutes))) + " % of day " + QString::number(ceil(uptime_minutes / day_minutes)));
    ui->uptime_bar->setValue(bar_value);

}

void MainWindow::update_cpu_temp() {

    // update cpu temp
    int cpu_temp_value = sysAPI->cpu_temp();
    ui->cpu_temp_bar->setFormat(QString::number(cpu_temp_value) + " °C");
    ui->cpu_temp_bar->setMinimum(0);
    ui->cpu_temp_bar->setMaximum(100);
    ui->cpu_temp_bar->setValue(cpu_temp_value);
    QString temp_style = "QProgressBar {"   \
                             "color: white;"    \
                             "border: 1px solid white;" \
                             "text-align: center;"  \
                         "}"    \
                         "QProgressBar::chunk {"    \
                             "background: qlineargradient(" \
                             "spread:pad, x1:0, y1:0, x2:1, y2:0,"  \
                             "stop:0 ";

    int temp_color_factor = round(cpu_temp_value / (100.0 / 5.0)) - 1;
    if (temp_color_factor > 4) {
        temp_color_factor = 4;
    }
    temp_style.append(QString::fromStdString(temp_color_dark[temp_color_factor] + ", stop:1 " + temp_color_light[temp_color_factor] + ")}"));

    ui->cpu_temp_bar->setStyleSheet(temp_style);
}
