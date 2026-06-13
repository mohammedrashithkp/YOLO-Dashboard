# Hardware & NPU Support

YOLO Dashboard is uniquely tailored for edge deployment, specifically on devices like the **Raspberry Pi 5** paired with a **Hailo-8 26 TOPS AI Hat+**.

## CPU Inference (ONNX)
By default, the application runs on the CPU via ONNX Runtime. This requires the model to be in standard `.onnx` format (e.g., `yolov8n.onnx`).

## Hailo NPU Inference
To achieve 30+ FPS object detection on edge devices, the dashboard supports the Hailo Dataflow Compiler and HailoRT C++ SDK.

### Compilation
When building on a Raspberry Pi 5, use the `Dockerfile.arm64`. This Dockerfile invokes CMake with the `-DENABLE_HAILO=ON` flag, which automatically discovers the HailoRT libraries and links them into the application.

### Device Passthrough
To ensure the Docker container can talk to the NPU, you MUST run the container with device passthrough:
```yaml
    devices:
      - /dev/hailo0:/dev/hailo0
```
This is provided in the `docker-compose.rpi5.yml` override file.

### Model Format
Hailo NPUs require the model to be compiled ahead of time into a `.hef` (Hailo Executable Format) file. Place your compiled `.hef` file in the `/data/models` directory. You can select it in the Model tab of the UI.
