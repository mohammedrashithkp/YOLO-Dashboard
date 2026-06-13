#include "yolo_dashboard/hardware/hardware_detector.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#endif

namespace yolo_dashboard {

HardwareInfo HardwareDetector::detectAll() {
    HardwareInfo info;
    info.cpu = detectCpu();
    info.memory = detectMemory();
    info.npus = detectNpus();
    return info;
}

#ifdef _WIN32
// ── Windows CPU Detection ──

CpuInfo HardwareDetector::detectCpu() {
    CpuInfo info;

    // Get CPU brand string via CPUID
    int cpuInfo[4] = {0};
    char brand[49] = {0};

    __cpuid(cpuInfo, 0x80000002);
    memcpy(brand, cpuInfo, sizeof(cpuInfo));
    __cpuid(cpuInfo, 0x80000003);
    memcpy(brand + 16, cpuInfo, sizeof(cpuInfo));
    __cpuid(cpuInfo, 0x80000004);
    memcpy(brand + 32, cpuInfo, sizeof(cpuInfo));

    info.model_name = brand;
    // Trim whitespace
    info.model_name.erase(0, info.model_name.find_first_not_of(" "));
    info.model_name.erase(info.model_name.find_last_not_of(" ") + 1);

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    info.threads = sysInfo.dwNumberOfProcessors;

    // Approximate cores (may differ from threads on hyperthreaded CPUs)
    DWORD len = 0;
    GetLogicalProcessorInformation(nullptr, &len);
    if (len > 0) {
        std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buffer(
            len / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
        if (GetLogicalProcessorInformation(buffer.data(), &len)) {
            int cores = 0;
            for (const auto& entry : buffer) {
                if (entry.Relationship == RelationProcessorCore) {
                    cores++;
                }
            }
            info.cores = cores;
        }
    }
    if (info.cores == 0) info.cores = info.threads;

    // Architecture
    switch (sysInfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: info.architecture = "x86_64"; break;
        case PROCESSOR_ARCHITECTURE_ARM64: info.architecture = "aarch64"; break;
        case PROCESSOR_ARCHITECTURE_INTEL: info.architecture = "x86"; break;
        default: info.architecture = "unknown"; break;
    }

    return info;
}

MemoryInfo HardwareDetector::detectMemory() {
    MemoryInfo info;
    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        info.total_mb = mem.ullTotalPhys / (1024 * 1024);
        info.available_mb = mem.ullAvailPhys / (1024 * 1024);
    }
    return info;
}

#else
// ── Linux CPU Detection ──

CpuInfo HardwareDetector::detectCpu() {
    CpuInfo info;

    std::ifstream cpuinfo("/proc/cpuinfo");
    std::string line;
    int processor_count = 0;

    while (std::getline(cpuinfo, line)) {
        if (line.find("model name") != std::string::npos) {
            auto pos = line.find(':');
            if (pos != std::string::npos) {
                info.model_name = line.substr(pos + 2);
            }
        }
        if (line.find("processor") == 0) {
            processor_count++;
        }
        if (line.find("cpu MHz") != std::string::npos) {
            auto pos = line.find(':');
            if (pos != std::string::npos) {
                try { info.frequency_mhz = std::stod(line.substr(pos + 2)); }
                catch (...) {}
            }
        }
    }

    info.threads = processor_count > 0 ? processor_count : 1;

    // Try to get physical cores from /sys
    std::ifstream core_file("/sys/devices/system/cpu/cpu0/topology/core_siblings_list");
    if (core_file.good()) {
        // Count unique core_id values
        info.cores = sysconf(_SC_NPROCESSORS_ONLN);
    } else {
        info.cores = info.threads;
    }

    // Architecture
    struct utsname uname_buf;
    if (uname(&uname_buf) == 0) {
        info.architecture = uname_buf.machine;
    }

    return info;
}

MemoryInfo HardwareDetector::detectMemory() {
    MemoryInfo info;

    std::ifstream meminfo("/proc/meminfo");
    std::string line;

    while (std::getline(meminfo, line)) {
        if (line.find("MemTotal:") == 0) {
            std::istringstream iss(line);
            std::string label;
            uint64_t value;
            iss >> label >> value;
            info.total_mb = value / 1024;
        }
        if (line.find("MemAvailable:") == 0) {
            std::istringstream iss(line);
            std::string label;
            uint64_t value;
            iss >> label >> value;
            info.available_mb = value / 1024;
        }
    }

    return info;
}

#endif // _WIN32

// ── NPU Detection (cross-platform) ──

std::vector<NpuInfo> HardwareDetector::detectNpus() {
    std::vector<NpuInfo> npus;

#ifdef HAILO_ENABLED
    // Use HailoRT to scan for devices
    try {
        auto scan_result = hailort::Device::scan();
        if (scan_result) {
            for (const auto& device_id : scan_result.value()) {
                NpuInfo npu;
                npu.device_id = device_id;
                npu.name = "Hailo-8";
                npu.tops = 26.0;
                npu.is_available = true;

                // Try to get firmware version
                auto device = hailort::Device::create(device_id);
                if (device) {
                    auto fw_version = device->get_extended_device_information();
                    if (fw_version) {
                        npu.firmware_version = fw_version->firmware_version;
                    }
                }

                npus.push_back(npu);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[Hardware] Hailo scan error: " << e.what() << std::endl;
    }
#else
    // No HailoRT linked — check if device node exists (Linux only)
#ifndef _WIN32
    struct stat st;
    if (stat("/dev/hailo0", &st) == 0) {
        NpuInfo npu;
        npu.device_id = "/dev/hailo0";
        npu.name = "Hailo (driver detected, HailoRT SDK not linked)";
        npu.is_available = false; // Available but not usable without SDK
        npus.push_back(npu);
    }
#endif
#endif

    if (npus.empty()) {
        std::cout << "[Hardware] No NPU accelerators detected" << std::endl;
    }

    return npus;
}

} // namespace yolo_dashboard
