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

/*
0: user – time spent in user mode.
1: nice – time spent in user mode with low priority.
2: system – time spent in system mode.
3: idle – time spent in the idle task.
4: iowait –  time waiting for I/O to complete.
5: irq – time servicing hardware interrupts.
6: softirq – time servicing software interrupts.
7: steal – time spent in other operating systems when running in a virtualized environment.
8: guest – time spent running a virtual CPU for guest operating systems.
9: guest_nice – time spent running a low priority virtual CPU for guest operating systems.
*/

#define S_USER          0
#define S_NICE          1
#define S_SYSTEM        2
#define S_IDLE          3
#define S_IOWAIT        4
#define S_IRQ           5
#define S_SOFTIRQ       6
#define S_STEAL         7
#define S_GUEST         8
#define S_GUEST_NICE    9

using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QTimer *timer1 = new QTimer(this);
    connect(timer1, SIGNAL(timeout()), this, SLOT(update_monitor()));
    timer1->start(1500);

    QTimer *timer2 = new QTimer(this);
    connect(timer2, SIGNAL(timeout()), this, SLOT(update_time()));
    timer2->start(50);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::update_time() {
    ui->date_label->setText(QDateTime::currentDateTime().toString("dddd MMM dd.MM.yyyy"));
    ui->clock_label->setText(QTime::currentTime().toString("t hh:mm:ss"));
}

void MainWindow::update_monitor() {
    // cpu usage


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

//    double cpu = cpu_usage();
//    ui->processor_lcd->display(QString::number(cpu));
//    cout << "cpu updated: " << cpu << endl;
}

int MainWindow::cpu_usage() {
    ifstream stat_file("/proc/stat");

    string line;
    const string STR_CPU = "cpu";
    const size_t STR_CPU_SIZE = STR_CPU.size();
    const int NUM_CPU_STATES = 10;
    string cpu_entry;
    vector<string> cpu_states;

    while(getline(stat_file, line)) {
        if (line.find(STR_CPU) != string::npos) {
            istringstream ss(line);

            ss >> cpu_entry;

            if (cpu_entry.size() > STR_CPU_SIZE) {
                cpu_entry.erase(0, STR_CPU_SIZE);
                string temp_state;
                for (int i=0; i < NUM_CPU_STATES; ++i) {
                    ss >> temp_state;
                    cpu_states.push_back(temp_state);
                }
            }
            break;
        }

    }

    if (cpu_states.empty()) {
        return -1;
    }

//    cout << "cpu states:" << endl;
//    for (auto item : cpu_states) {
//        cout << item << endl;
//    }

    int idle_time = stoi(cpu_states[S_IDLE]) + stoi(cpu_states[S_IOWAIT]);
    int active_time = stoi(cpu_states[S_USER]) +
                      stoi(cpu_states[S_NICE]) +
                      stoi(cpu_states[S_SYSTEM]) +
                      stoi(cpu_states[S_IRQ]) +
                      stoi(cpu_states[S_SOFTIRQ]) +
                      stoi(cpu_states[S_STEAL]) +
                      stoi(cpu_states[S_GUEST]) +
                      stoi(cpu_states[S_GUEST_NICE]);
    int total_time = idle_time + active_time;

//    cout << "idle time:" << idle_time << "\nactive time: " << active_time << endl;
//    double output = (total_time - idle_time) / total_time * 100.;
    double output = 100. * active_time / total_time;
    cout << output << endl;
    return output;
}
