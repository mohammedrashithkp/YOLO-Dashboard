#pragma once

#include "yolo_dashboard/common.h"

namespace yolo_dashboard {

/// Platform-agnostic camera enumeration interface.
/// Implementations: camera_enumerator_linux.cpp (V4L2), camera_enumerator_win.cpp (DirectShow)
class CameraEnumerator {
public:
    /// Discover all available video capture devices on the system.
    /// On Linux: scans /dev/video* using V4L2 VIDIOC_QUERYCAP
    /// On Windows: uses DirectShow ICreateDevEnum for VideoInputDeviceCategory
    static std::vector<CameraInfo> enumerate();
};

} // namespace yolo_dashboard
