/// Camera enumerator for Windows — uses DirectShow COM API.
/// Enumerates CLSID_VideoInputDeviceCategory devices.

#ifdef _WIN32

#include "yolo_dashboard/camera/camera_enumerator.h"

#include <windows.h>
#include <dshow.h>
#include <string>
#include <vector>
#include <iostream>

#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

namespace yolo_dashboard {

static std::string wcharToString(const wchar_t* wstr) {
    if (!wstr) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    std::string result(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &result[0], len, nullptr, nullptr);
    return result;
}

std::vector<CameraInfo> CameraEnumerator::enumerate() {
    std::vector<CameraInfo> cameras;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool com_initialized = SUCCEEDED(hr) || hr == S_FALSE;
    if (hr == RPC_E_CHANGED_MODE) {
        // Already initialized in a different mode, try anyway
        com_initialized = true;
    }

    if (!com_initialized) {
        std::cerr << "[CameraEnum] COM initialization failed" << std::endl;
        return cameras;
    }

    ICreateDevEnum* dev_enum = nullptr;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
                          IID_ICreateDevEnum, (void**)&dev_enum);

    if (FAILED(hr) || !dev_enum) {
        CoUninitialize();
        return cameras;
    }

    IEnumMoniker* enum_moniker = nullptr;
    hr = dev_enum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &enum_moniker, 0);

    if (hr == S_OK && enum_moniker) {
        IMoniker* moniker = nullptr;
        int index = 0;

        while (enum_moniker->Next(1, &moniker, nullptr) == S_OK) {
            IPropertyBag* prop_bag = nullptr;
            hr = moniker->BindToStorage(nullptr, nullptr, IID_IPropertyBag, (void**)&prop_bag);

            if (SUCCEEDED(hr) && prop_bag) {
                CameraInfo info;
                info.id = index;
                info.is_available = true;

                // Get friendly name
                VARIANT var;
                VariantInit(&var);
                hr = prop_bag->Read(L"FriendlyName", &var, nullptr);
                if (SUCCEEDED(hr)) {
                    info.name = wcharToString(var.bstrVal);
                    VariantClear(&var);
                } else {
                    info.name = "Camera " + std::to_string(index);
                }

                // Get device path
                VariantInit(&var);
                hr = prop_bag->Read(L"DevicePath", &var, nullptr);
                if (SUCCEEDED(hr)) {
                    info.device_path = wcharToString(var.bstrVal);
                    VariantClear(&var);
                }

                info.driver = "DirectShow";
                cameras.push_back(info);

                prop_bag->Release();
            }

            moniker->Release();
            index++;
        }

        enum_moniker->Release();
    }

    dev_enum->Release();
    CoUninitialize();

    if (cameras.empty()) {
        std::cout << "[CameraEnum] No cameras found via DirectShow, trying OpenCV probe..." << std::endl;
        // Fallback: probe indices
        for (int i = 0; i < 5; ++i) {
            cv::VideoCapture test(i, cv::CAP_MSMF);
            if (test.isOpened()) {
                CameraInfo info;
                info.id = i;
                info.name = "Camera " + std::to_string(i);
                info.device_path = "index:" + std::to_string(i);
                info.driver = "MSMF";
                info.is_available = true;
                cameras.push_back(info);
                test.release();
            }
        }
    }

    return cameras;
}

} // namespace yolo_dashboard

#endif // _WIN32
