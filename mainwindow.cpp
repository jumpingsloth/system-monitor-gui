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

#define BYTES_FACTOR 9.5367431640625e-7

using namespace std;

//  rgb(38, 162, 105) rgb(229, 165, 10) rgb(198, 70, 0) rgb(165, 29, 45) rgb(97, 53, 131)

//  rgb(87, 227, 137) rgb(248, 228, 92) rgb(255, 163, 72) rgb(237, 51, 59) rgb(192, 97, 203)

const vector<string> temp_color_dark = {"rgb(38, 162, 105)", "rgb(229, 165, 10)", "rgb(198, 70, 0)", "rgb(165, 29, 45)", "rgb(97, 53, 131)"};
const vector<string> temp_color_light = {"rgb(87, 227, 137)", "rgb(248, 228, 92)", "rgb(255, 163, 72)", "rgb(237, 51, 59)", "rgb(192, 97, 203)"};

long long received_bytes = -1;
long long sent_bytes = -1;
long long highest_received = 0;
long long highest_sent = 0;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

//    void update_cpu_usage();
//    void update_cpu_temp();
//    void update_disk();
//    void update_memory();
//    void update_networking();
//    void update_uptime();
//    void update_time();



//    QTimer *timer1 = new QTimer(this);
//    connect(timer1, SIGNAL(timeout()), this, SLOT(update_cpu_usage()));
//    timer1->start(1300);

//    QTimer *timer2 = new QTimer(this);
//    connect(timer2, SIGNAL(timeout()), this, SLOT(update_cpu_temp()));
//    timer2->start(1300);

//    QTimer *timer3 = new QTimer(this);
//    connect(timer3, SIGNAL(timeout()), this, SLOT(update_disk()));
//    timer3->start(1300);

    QTimer *timer4 = new QTimer(this);
    connect(timer4, SIGNAL(timeout()), this, SLOT(update_memory()));
    timer4->start(1300);

    QTimer *timer5 = new QTimer(this);
    connect(timer5, SIGNAL(timeout()), this, SLOT(update_networking()));
    timer5->start(1000);

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
    vector<long> mem_data = sys_used_memory();
    long used_mem_kb = mem_data[0];
    long total_mem = mem_data[1];

    double used_mem_gb = used_mem_kb * BYTES_FACTOR;

    ui->memory_lcd->display(QString::number(used_mem_gb, 'g', 2));
    ui->memory_bar->setMinimum(0.);
    ui->memory_bar->setMaximum(total_mem);
    ui->memory_bar->setValue(used_mem_kb);
}

void MainWindow::update_disk() {

}

void MainWindow::update_networking() {
    vector<long long> rs_bytes_second = sys_networking();
    double received_mb = rs_bytes_second[0] * BYTES_FACTOR;
    double sent_mb = rs_bytes_second[1] * BYTES_FACTOR;



    ui->networkR_lcd->display(QString::number(received_mb, 'g', 2));
    ui->networkS_lcd->display(QString::number(sent_mb, 'g', 2));

    ui->networkR_bar->setMinimum(0);
    if (highest_received > 0) {
        ui->networkR_bar->setMaximum(highest_received * BYTES_FACTOR);
    } else {
        ui->networkR_bar->setMaximum(0);
    }

    ui->networkR_bar->setValue(received_mb);

    ui->networkS_bar->setMinimum(0);
    if (highest_sent > 0) {
        ui->networkS_bar->setMaximum(highest_sent * BYTES_FACTOR);
    } else {
        ui->networkS_bar->setMaximum(0);
    }

    ui->networkS_bar->setValue(sent_mb);


}

void MainWindow::update_cpu_usage() {

    // update cpu usage

//    double cpu = cpu_usage();
//    ui->processor_lcd->display(QString::number(cpu));
//    cout << "cpu updated: " << cpu << endl;
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
    int cpu_temp_value = cpu_temp();
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

    int temp_color_factor = round(cpu_temp_value / (100.0 / 5.0));
    temp_style.append(QString::fromStdString(temp_color_dark[temp_color_factor] + ", stop:1 " + temp_color_light[temp_color_factor] + ")}"));

    ui->cpu_temp_bar->setStyleSheet(temp_style);
}

vector<long long> MainWindow::sys_networking() {
    ifstream network_file("/proc/net/dev");

    vector<string> network_received;
    vector<string> network_sent;
    string line;

    while (getline(network_file, line)) {
        if (!((line.find("Inter-") != string::npos) || (line.find("face") != string::npos) || (line.find("lo:") != string::npos))) {
            istringstream ss(line);
            string entry;
            vector<string> entries;
            while (ss >> entry) {
                entries.push_back(entry);
            }
            network_received.push_back(entries[1]);
            network_sent.push_back(entries[9]);
        }
    }

    long long total_received = 0;
    for (string element : network_received) {
        total_received += stoll(element);
    }

    long long total_sent = 0;
    for (string element : network_sent) {
        total_sent += stoll(element);
    }

    vector<long long> output;

    if (received_bytes != -1) {
        long long received_second = total_received - received_bytes;
        output.push_back(received_second);
        if (received_second > highest_received) {
            highest_received = received_second;
        }
        received_bytes = total_received;
    } else {
        received_bytes = total_received;
        output.push_back(0);
    }

    if (sent_bytes != -1) {
        long long sent_second = total_sent - sent_bytes;
        output.push_back(sent_second);
        if (sent_second > highest_sent) {
            highest_sent = sent_second;
        }
        sent_bytes = total_sent;
    } else {
        sent_bytes = total_sent;
        output.push_back(0);
    }

    return output;
}

vector<long> MainWindow::sys_used_memory() {
    ifstream meminfo_file("/proc/meminfo");

    string mem_total;
    string mem_free;

    const string STR_MEM_TOTAL = "MemTotal";
    const size_t STR_MEM_TOTAL_SIZE = STR_MEM_TOTAL.size();
    const string STR_MEM_FREE = "MemFree";
    const size_t STR_MEM_FREE_SIZE = STR_MEM_FREE.size();
    string line;

    while (getline(meminfo_file, line)) {
        if (line.find(STR_MEM_TOTAL) != string::npos) {
            istringstream ss(line);
            string mem_entry;
            ss >> mem_entry;

            if (mem_entry.size() > STR_MEM_TOTAL_SIZE) {
                mem_entry.erase(0, STR_MEM_TOTAL_SIZE);
                ss >> mem_total;
            }
        } else if (line.find(STR_MEM_FREE) != string::npos) {
            istringstream ss(line);
            string mem_entry;
            ss >> mem_entry;

            if (mem_entry.size() > STR_MEM_FREE_SIZE) {
                mem_entry.erase(0, STR_MEM_FREE_SIZE);
                ss >> mem_free;
            }
        } else {
            break;
        }
    }

    long mem_total_num = stoi(mem_total);
    long mem_free_num = stoi(mem_free);
    long used_memory_kb = mem_total_num - mem_free_num;

    return {used_memory_kb, mem_total_num};

}

int MainWindow::cpu_temp() {
    string path = "/sys/class/hwmon";
    vector<long> temp_vector;

    for (const auto & file : filesystem::directory_iterator(path)) {
        long temp;
        ifstream temp_file(file.path().string() + "/device/temp");
        temp_file >> temp;
        temp_vector.push_back(temp);
    }

    long long temp_sum = 0;
    for (auto x : temp_vector) {
        temp_sum += x;
    }

    return round((temp_sum / 1000.0) / (double)temp_vector.size());
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


