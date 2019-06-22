#include <algorithm>
#include <iostream>
#include <math.h>
#include <thread>
#include <chrono>
#include <iterator>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include "constants.h"

using namespace std;

class ProcessParser
{
    private:
        std::ifstream stream;
    public:
        static string getCmd(string pid);
        static vector<string> getPidList();
        static std::string getVmSize(string pid);
        static std::string getCpuPercent(string pid);
        static long int getSysUpTime();
        static std::string getProcUpTime(string pid);
        static string getProcUser(string pid);
        static vector<string> getSysCpuPercent(string coreNumber = "");
        static float getSysRamPercent();
        static string getSysKernelVersion();
        static int getNumberOfCores();
        static int getTotalThreads();
        static int getTotalNumberOfProcesses();
        static int getNumberOfRunningProcesses();
        static string getOSName();
        static std::string PrintCpuStats(std::vector<std::string> values1,
                                         std::vector<std::string> values2);
        static bool isPidExisting(string pid);

        static float getSysActiveCpuTime(std::vector<std::string> values);
        static float getSysIdleCpuTime(std::vector<std::string> values);
};

// TODO: Define all of the above functions below:
string ProcessParser::getCmd(string pid)
{
    string cmdline;
    std::ifstream stream = Util::getStream(
            Path::basePath() + pid + Path::cmdPath());
    std::getline(stream, cmdline);

    return cmdline;
}

vector<string> ProcessParser::getPidList()
{
    DIR *d;
    struct dirent *ent;
    if (!(d = opendir("/proc")))
    {
        throw std::runtime_error("Not able to open /proc");
    }
    std::vector<std::string> list;
    while ((ent = readdir(d)))
    {
        if (Util::isDigits(ent->d_name))
        {
            list.push_back(ent->d_name);
        }
    }
    closedir(d);
    return list;
}

std::string ProcessParser::getVmSize(string pid)
{
    std::string line;
    std::string VmData = "VmData:";
    float vmSize = 0;
    std::ifstream stream = Util::getStream(
            Path::basePath() + pid + Path::statusPath());

    while (std::getline(stream, line))
    {
        if (line.compare(0, VmData.size(), VmData) == 0)
        {
            std::istringstream iss(line);
            std::vector<std::string> v(
                    std::istream_iterator<std::string> { iss },
                    std::istream_iterator<std::string>());
            vmSize = stof(v[1]) / 1024 / 1024;
        }
    }

    return to_string(vmSize);
}

std::string ProcessParser::getCpuPercent(string pid)
{
    int uptime = ProcessParser::getSysUpTime();

    std::string stat;
    std::ifstream stream = Util::getStream(
            Path::basePath() + pid + "/" + Path::statPath());
    std::getline(stream, stat);
    std::istringstream iss(stat);
    std::vector<std::string> vStats(std::istream_iterator<std::string> { iss },
                                    std::istream_iterator<std::string>());

    float frequency = sysconf(_SC_CLK_TCK);
    float seconds = uptime - (stof(vStats[21]) / frequency);
    float cpuUsage = 100
            * (((stof(vStats[14]) + stof(vStats[15]) + stof(vStats[16])
                    + stof(vStats[17])) / frequency) / seconds);

    return to_string(cpuUsage);
}

long int ProcessParser::getSysUpTime()
{
    std::string uptime;
    std::ifstream stream = Util::getStream(
            Path::basePath() + Path::upTimePath());
    std::getline(stream, uptime);
    std::istringstream iss(uptime);
    std::vector<std::string> v(std::istream_iterator<std::string> { iss },
                               std::istream_iterator<std::string>());
    return stoi(v[0]);
}

std::string ProcessParser::getProcUpTime(string pid)
{
    std::string stat;
    std::ifstream stream = Util::getStream(
            Path::basePath() + pid + "/" + Path::statPath());
    std::getline(stream, stat);
    std::istringstream iss(stat);
    std::vector<std::string> vStats(std::istream_iterator<std::string> { iss },
                                    std::istream_iterator<std::string>());

    return to_string(stof(vStats[14]) / sysconf(_SC_CLK_TCK));
}

std::string ProcessParser::getProcUser(string pid)
{
    std::string line;
    std::string uid = "Uid:";
    std::string userUid;
    std::ifstream stream = Util::getStream(
            Path::basePath() + pid + Path::statusPath());
    while (std::getline(stream, line))
    {
        if (line.compare(0, uid.size(), uid) == 0)
        {
            std::istringstream iss(line);
            std::vector<std::string> v(
                    std::istream_iterator<std::string> { iss },
                    std::istream_iterator<std::string>());

            userUid = "x:" + v[1];
            break;
        }
    }

    stream = Util::getStream("/etc/passwd");
    while (std::getline(stream, line))
    {
        if (line.find(userUid) != std::string::npos)
        {
            return line.substr(0, line.find(":"));
        }
    }

    return "";
}

vector<string> ProcessParser::getSysCpuPercent(string coreNumber)
{
    std::string cpu = "cpu" + coreNumber;
    std::string line;
    std::ifstream stream = Util::getStream(Path::basePath() + Path::statPath());
    while (std::getline(stream, line))
    {
        if (line.compare(0, cpu.size(), cpu) == 0)
        {
            std::istringstream iss(line);
            std::vector<std::string> v(
                    std::istream_iterator<std::string> { iss },
                    std::istream_iterator<std::string>());
            return v;
        }
    }

    return std::vector<string> { };
}

float ProcessParser::getSysRamPercent()
{
    std::string sMemAvailable = "MemAvailable:";
    std::string sMemFree = "MemFree:";
    std::string sBuffers = "Buffers:";
    std::string line;

    float availableMem = 0, freeMem = 0, buffers = 0;

    std::ifstream stream = Util::getStream(
            Path::basePath() + Path::memInfoPath());
    while (std::getline(stream, line))
    {
        if (availableMem != 0 && freeMem != 0 && buffers != 0)
        {
            break;
        }
        if (line.compare(0, sMemAvailable.size(), sMemAvailable) == 0)
        {
            std::istringstream iss(line);
            std::vector<std::string> v(
                    std::istream_iterator<std::string> { iss },
                    std::istream_iterator<std::string>());
            availableMem = stof(v[1]);
            continue;
        }
        if (line.compare(0, sMemFree.size(), sMemFree) == 0)
        {
            std::istringstream iss(line);
            std::vector<std::string> v(
                    std::istream_iterator<std::string> { iss },
                    std::istream_iterator<std::string>());
            freeMem = stof(v[1]);
            continue;
        }
        if (line.compare(0, sBuffers.size(), sBuffers) == 0)
        {
            std::istringstream iss(line);
            std::vector<std::string> v(
                    std::istream_iterator<std::string> { iss },
                    std::istream_iterator<std::string>());
            buffers = stof(v[1]);
        }
    }
    return 100 * (1 - freeMem / (availableMem - buffers));
}

std::string ProcessParser::getSysKernelVersion()
{
    std::string linuxVersion = "Linux version ";
    std::string line;
    std::ifstream stream = Util::getStream(
            Path::basePath() + Path::versionPath());
    while (std::getline(stream, line))
    {
        if (line.compare(0, linuxVersion.size(), linuxVersion) == 0)
        {
            std::istringstream iss(line);
            std::vector<std::string> v(
                    std::istream_iterator<std::string> { iss },
                    std::istream_iterator<std::string>());
            return v[2];
        }
    }
    return "";
}

int ProcessParser::getNumberOfCores()
{
    string line;
    string cpuCores = "cpu cores";
    std::ifstream stream = Util::getStream(Path::basePath() + "cpuinfo");

    while (std::getline(stream, line))
    {
        if (line.compare(0, cpuCores.size(), cpuCores) == 0)
        {
            std::istringstream iss(line);
            std::vector<std::string> cores(
                    std::istream_iterator<std::string> { iss },
                    std::istream_iterator<std::string>());
            return stoi(cores[3]);
        }
    }
    return 0;
}

int ProcessParser::getTotalThreads()
{
    std::string line;
    std::string threads = "Threads:";
    int threadCnt = 0;
    std::ifstream stream;

    vector<string> pidList = getPidList();
    for (uint i = 0; i < pidList.size(); i++)
    {
        stream = Util::getStream(
                Path::basePath() + pidList[i] + Path::statusPath());
        while (std::getline(stream, line))
        {
            if (line.compare(0, threads.size(), threads) == 0)
            {
                std::istringstream iss(line);
                std::vector<std::string> v(
                        std::istream_iterator<std::string> { iss },
                        std::istream_iterator<std::string>());
                threadCnt += stoi(v[1]);
                break;
            }

        }
    }
    return threadCnt;
}

int ProcessParser::getTotalNumberOfProcesses()
{
    std::string line;
    std::string processes = "processes";
    std::ifstream stream = Util::getStream(Path::basePath() + Path::statPath());
    while (std::getline(stream, line))
    {
        if (line.compare(0, processes.size(), processes) == 0)
        {
            std::istringstream iss(line);
            std::vector<std::string> v(
                    std::istream_iterator<std::string> { iss },
                    std::istream_iterator<std::string>());
            return stoi(v[1]);
        }
    }
    return 0;
}

int ProcessParser::getNumberOfRunningProcesses()
{
    std::string line;
    std::string procsRunning = "procs_running";
    std::ifstream stream = Util::getStream(Path::basePath() + Path::statPath());
    while (std::getline(stream, line))
    {
        if (line.compare(0, procsRunning.size(), procsRunning) == 0)
        {
            std::istringstream iss(line);
            std::vector<std::string> v(
                    std::istream_iterator<std::string> { iss },
                    std::istream_iterator<std::string>());
            return stoi(v[1]);
        }
    }
    return 0;
}

std::string ProcessParser::getOSName()
{

    std::string prettyName = "PRETTY_NAME=";
    std::string line;
    std::ifstream stream = Util::getStream("/etc/os-release");
    while (std::getline(stream, line))
    {
        if (line.compare(0, prettyName.size(), prettyName) == 0)
        {
            std::size_t found = line.find("=");
            std::string osName = line.substr(++found);
            osName.erase(std::remove(osName.begin(), osName.end(), '"'),
                         osName.end());
            return osName;
        }
    }
    return "";
}

std::string ProcessParser::PrintCpuStats(std::vector<std::string> values1,
                                         std::vector<std::string> values2)
{

    float activeCpuTime = getSysActiveCpuTime(values2)
            - getSysActiveCpuTime(values1);
    float idleCpuTime = getSysIdleCpuTime(values2) - getSysIdleCpuTime(values1);
    return to_string(100 * (activeCpuTime / (activeCpuTime + idleCpuTime)));
}

bool ProcessParser::isPidExisting(string pid)
{
    vector<string> pidList = getPidList();
    std::vector<string>::iterator it = std::find(pidList.begin(), pidList.end(),
                                                 pid);
    if (it != pidList.end())
    {
        return true;
    }
    return false;
}

float ProcessParser::getSysActiveCpuTime(std::vector<std::string> values)
{
    return stof(values[CPUStates::S_GUEST])
            + stof(values[CPUStates::S_GUEST_NICE])
            + stof(values[CPUStates::S_IRQ]) + stof(values[CPUStates::S_NICE])
            + stof(values[CPUStates::S_SOFTIRQ])
            + stof(values[CPUStates::S_STEAL])
            + stof(values[CPUStates::S_SYSTEM])
            + stof(values[CPUStates::S_USER]);
}

float ProcessParser::getSysIdleCpuTime(std::vector<std::string> values)
{
    return stof(values[CPUStates::S_IDLE]) + stof(values[CPUStates::S_IOWAIT]);
}