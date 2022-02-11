#include "linuxsystem.h"

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

LinuxSystem::LinuxSystem()
{

}

double LinuxSystem::sys_disk_activity() {
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

            for (size_t i=3; i<disk_stats.size(); i++) {
                disk_stats_num.push_back(stoll(disk_stats[i]));
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
    long long active_time = 0;
    for (const auto & disk : disk_info) {
        active_time += disk[3] + disk[7]; // time spent reading / writing (ms)
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

vector<long long> LinuxSystem::sys_networking() {
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

vector<long> LinuxSystem::sys_used_memory() {
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

int LinuxSystem::cpu_temp() {
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

double LinuxSystem::cpu_usage() {
    ifstream stat_file("/proc/stat");

    string line;
    const string STR_CPU = "cpu";
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


