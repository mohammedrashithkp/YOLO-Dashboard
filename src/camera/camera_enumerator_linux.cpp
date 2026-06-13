/// Camera enumerator for Linux — uses V4L2 ioctls on /dev/video* devices.

#ifndef _WIN32

#include "yolo_dashboard/camera/camera_enumerator.h"

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <regex>
#include <opencv2/opencv.hpp>

namespace yolo_dashboard {

std::vector<CameraInfo> CameraEnumerator::enumerate() {
    std::vector<CameraInfo> cameras;

    // Scan /dev/ for video devices
    DIR* dir = opendir("/dev");
    if (!dir) {
        std::cerr << "[CameraEnum] Cannot open /dev" << std::endl;
        return cameras;
    }

    std::vector<std::string> video_devices;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name.find("video") == 0) {
            video_devices.push_back("/dev/" + name);
        }
    }
    closedir(dir);

    // Sort numerically
    std::sort(video_devices.begin(), video_devices.end());

    int index = 0;
    for (const auto& device_path : video_devices) {
        int fd = open(device_path.c_str(), O_RDWR | O_NONBLOCK);
        if (fd < 0) continue;

        struct v4l2_capability cap;
        if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
            close(fd);
            continue;
        }

        // Check if this is a video capture device (not metadata or other type)
        if (!(cap.device_caps & V4L2_CAP_VIDEO_CAPTURE)) {
            close(fd);
            continue;
        }

        CameraInfo info;
        info.id = index;
        info.name = reinterpret_cast<const char*>(cap.card);
        info.device_path = device_path;
        info.driver = reinterpret_cast<const char*>(cap.driver);
        info.is_available = true;

        cameras.push_back(info);
        close(fd);
        index++;
    }

    if (cameras.empty()) {
        std::cout << "[CameraEnum] No V4L2 cameras found, trying OpenCV probe..." << std::endl;
        for (int i = 0; i < 5; ++i) {
            cv::VideoCapture test(i, cv::CAP_V4L2);
            if (test.isOpened()) {
                CameraInfo info;
                info.id = i;
                info.name = "Camera " + std::to_string(i);
                info.device_path = "/dev/video" + std::to_string(i);
                info.driver = "v4l2";
                info.is_available = true;
                cameras.push_back(info);
                test.release();
            }
        }
    }

    return cameras;
}

} // namespace yolo_dashboard

#endif // !_WIN32
