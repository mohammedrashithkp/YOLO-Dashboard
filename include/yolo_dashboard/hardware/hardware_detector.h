#pragma once

#include "yolo_dashboard/common.h"

namespace yolo_dashboard {

/// Detects CPU, NPU (Hailo), and memory information.
class HardwareDetector {
public:
    /// Detect all available hardware and return a consolidated report.
    static HardwareInfo detectAll();

    /// Detect CPU information.
    static CpuInfo detectCpu();

    /// Detect available memory.
    static MemoryInfo detectMemory();

    /// Detect Hailo NPU devices.
    /// Returns empty vector if no Hailo hardware or SDK is available.
    static std::vector<NpuInfo> detectNpus();
};

} // namespace yolo_dashboard
