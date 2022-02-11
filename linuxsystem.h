#ifndef LINUXSYSTEM_H
#define LINUXSYSTEM_H

#include <vector>

class LinuxSystem
{
public:
    LinuxSystem();
    ~LinuxSystem();

public:
    double cpu_usage();
    int cpu_temp();
    std::vector<long> sys_used_memory();
    std::vector<long long> sys_networking();
    double sys_disk_activity();

private:
    long long received_bytes = -1;
    long long sent_bytes = -1;
    long long highest_received = 0;
    long long highest_sent = 0;

    long long last_active_time = -1;
    long long last_total_time = -1;

    long long last_disk_total_time = -1;
    long long last_disk_active_time = -1;
};

#endif // LINUXSYSTEM_H
