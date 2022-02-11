#include "windowssystem.h"

WindowsSystem::WindowsSystem()
{

}

double WindowsSystem::cpu_usage()
{
    return 0;
}

int WindowsSystem::cpu_temp()
{
    return 0;
}

std::vector<long> WindowsSystem::sys_used_memory()
{
    return {0};
}

std::vector<long long> WindowsSystem::sys_networking()
{
    return {0};
}

double WindowsSystem::sys_disk_activity()
{
    return 0;
}
