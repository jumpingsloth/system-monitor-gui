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

#define KB_MB_FACTOR 9.5367431640625e-7
#define BYTES_KIB_FACTOR 0.0078125
#define BYTES_MIB_FACTOR 7.6294e-6

#define BYTES_GIB 134217728
#define BYTES_KIB 128

#define NETWORKING_UPDATE_TIME 3

#define BYTES_SECTOR_SIZE 512

// 1  major number
// 2  minor mumber
// 3  device name
// 4  reads completed successfully
// 5  reads merged
// 6  sectors read
// 7  time spent reading (ms)
// 8  writes completed
// 9  writes merged
//10  sectors written
//11  time spent writing (ms)
//12  I/Os currently in progress
//13  time spent doing I/Os (ms)
//14  weighted time spent doing I/Os (ms)
//15  discards completed successfully
//16  discards merged
//17  sectors discarded
//18  time spent discarding


using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

//    initialize_gui();

    QTimer *timer1 = new QTimer(this);
    connect(timer1, SIGNAL(timeout()), this, SLOT(update_cpu_usage()));
    timer1->start(1000);

    QTimer *timer2 = new QTimer(this);
    connect(timer2, SIGNAL(timeout()), this, SLOT(update_cpu_temp()));
    timer2->start(1300);

    QTimer *timer3 = new QTimer(this);
    connect(timer3, SIGNAL(timeout()), this, SLOT(update_disk()));
    timer3->start(1000);

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

void MainWindow::initialize_gui() {
    vector<QProgressBar *> ui_bars = {ui->processor_bar, ui->cpu_temp_bar, ui->disk_bar, ui->memory_bar, ui->networkR_bar, ui->networkS_bar, ui->uptime_bar};

    ui_bars[0]->setMinimum(0);
    ui_bars[0]->setMaximum(0);
    ui_bars[0]->setValue(80);

}

void MainWindow::update_time() {
    ui->date_label->setText(QDateTime::currentDateTime().toString("dddd MMM dd.MM.yyyy"));
    ui->clock_label->setText(QTime::currentTime().toString("t hh:mm:ss"));
}

void MainWindow::update_memory() {
    vector<long> mem_data = sys_used_memory();
    long used_mem_kb = mem_data[0];
    long total_mem = mem_data[1];

    double used_mem_gb = used_mem_kb * KB_MB_FACTOR;

    ui->memory_lcd->display(QString::number(used_mem_gb, 'g', 2));
    ui->memory_bar->setMinimum(0.);
    ui->memory_bar->setMaximum(total_mem);
    ui->memory_bar->setValue(used_mem_kb);
}

void MainWindow::update_disk() {
    double disk_activity = sys_disk_activity();

    ui->disk_lcd->display(QString::number(disk_activity, 'g', 2));
    ui->disk_bar->setMinimum(0);
    ui->disk_bar->setMaximum(100);
    ui->disk_bar->setValue(disk_activity);
}

void MainWindow::update_networking() {
    vector<long long> rs_bytes_second = sys_networking();

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

    double cpu = cpu_usage();
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

    int temp_color_factor = round(cpu_temp_value / (100.0 / 5.0)) - 1;
    if (temp_color_factor > 4) {
        temp_color_factor = 4;
    }
    temp_style.append(QString::fromStdString(temp_color_dark[temp_color_factor] + ", stop:1 " + temp_color_light[temp_color_factor] + ")}"));

    ui->cpu_temp_bar->setStyleSheet(temp_style);
}

double MainWindow::sys_disk_activity() {
    ifstream disk_file("/proc/diskstats");

    string line;
    vector<vector<long long>> disk_info;

    while (getline(disk_file, line)) {
        istringstream ss(line);

        vector<string> disk_stats;
        string entry;
        while (ss >> entry) {
            disk_stats.push_back(entry);
        }

        if (!(regex_match(disk_stats[2], regex(".*[0-9]+")))) { // device name
            vector<long long> disk_stats_num;
            for (vector<string>::iterator it = disk_stats.begin(); it != disk_stats.end(); ++it) {
                if (it - disk_stats.begin() > 2) { // all entries after device name
                    disk_stats_num.push_back(stoll(*it));
                }
            }
            disk_info.push_back(disk_stats_num);
        }
    }

    // get uptime
    chrono::milliseconds uptime(0u);
    double uptime_seconds;
    if (ifstream("/proc/uptime", ios::in) >> uptime_seconds) {
        uptime = chrono::milliseconds(static_cast<unsigned long long>(uptime_seconds*1000.0));
    }

    long long total_time = uptime.count();
    long long active_time;
    for (const auto & disk : disk_info) {
        active_time = disk[3] + disk[7]; // time spent reading / writing (ms)
    }

    double output;
    if ((last_disk_total_time != -1) && (last_disk_active_time != -1)) {
        long long total_time_diff = total_time - last_disk_total_time;
        long long active_time_diff = active_time - last_disk_active_time;

        output = 100. * (double)active_time_diff / (double)total_time_diff;

        last_disk_total_time = total_time;
        last_disk_active_time = active_time;
    } else {
        output = 0;

        last_disk_total_time = total_time;
        last_disk_active_time = active_time;
    }

    return output;
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
    for (const string & element : network_received) {
        total_received += stoll(element);
    }

    long long total_sent = 0;
    for (const string & element : network_sent) {
        total_sent += stoll(element);
    }

    vector<long long> output;

    if (received_bytes != -1) {
        long long received_diff = total_received - received_bytes;
        output.push_back(received_diff);
        if (received_diff > highest_received) {
            highest_received = received_diff;
        }
        received_bytes = total_received;
    } else {
        received_bytes = total_received;
        output.push_back(0);
    }

    if (sent_bytes != -1) {
        long long sent_diff = total_sent - sent_bytes;
        output.push_back(sent_diff);
        if (sent_diff > highest_sent) {
            highest_sent = sent_diff;
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

    long mem_total_num = stol(mem_total);
    long mem_free_num = stol(mem_free);
    long used_memory_kb = mem_total_num - mem_free_num;

    return {used_memory_kb, mem_total_num};

}

int MainWindow::cpu_temp() {
    string path = "/sys/class/hwmon/hwmon2";
    vector<long> temp_vector;

    for (const auto & file : filesystem::directory_iterator(path)) {
        if ((file.path().string().find("temp") != string::npos) && file.path().string().find("input") != string::npos) {
            ifstream temp_file(file.path().string());
            string temp_str;
            temp_file >> temp_str;
            temp_vector.push_back(stol(temp_str));
        }
    }

    long long temp_sum = 0;
    for (auto x : temp_vector) {
        temp_sum += x;
    }

    return round((temp_sum / 1000.0) / (double)temp_vector.size());
}

double MainWindow::cpu_usage() {
    ifstream stat_file("/proc/stat");

    string line;
    const string STR_CPU = "cpu";
    const size_t STR_CPU_SIZE = STR_CPU.size();
    const int NUM_CPU_STATES = 10;
    string cpu_entry;
    vector<long long> cpu_states;

    while(getline(stat_file, line)) {
        if (line.find(STR_CPU) != string::npos) {
            istringstream ss(line);

            ss >> cpu_entry;

            if (cpu_entry == STR_CPU) {
                string temp_state;
                for (int i=0; i < NUM_CPU_STATES; ++i) {
                    ss >> temp_state;
                    cpu_states.push_back(stoll(temp_state));
                }
                break;
            }
        }
    }

    if (cpu_states.empty()) {
        return -1;
    }

//    cout << "cpu states:" << endl;
//    for (auto item : cpu_states) {
//        cout << item << endl;
//    }

    long long idle_time = cpu_states[S_IDLE] + cpu_states[S_IOWAIT];
    long long active_time = cpu_states[S_USER] +
                      cpu_states[S_NICE] +
                      cpu_states[S_SYSTEM] +
                      cpu_states[S_IRQ] +
                      cpu_states[S_SOFTIRQ] +
                      cpu_states[S_STEAL] +
                      cpu_states[S_GUEST] +
                      cpu_states[S_GUEST_NICE];
    long long total_time = idle_time + active_time;

//    cout << "idle time:" << idle_time << "\nactive time: " << active_time << endl;
//    double output = (total_time - idle_time) / total_time * 100.;

//    double output = 100. * (double)active_time / (double)total_time;
    double output;
    if ((last_active_time != -1) && last_total_time != -1) {
        long long active_diff = active_time - last_active_time;
        long long total_diff = total_time - last_total_time;
        output = 100. * (double)active_diff / (double)total_diff;

        last_active_time = active_time;
        last_total_time = total_time;
    } else {
        output = 0;
        last_active_time = active_time;
        last_total_time = total_time;
    }

    return output;
}


