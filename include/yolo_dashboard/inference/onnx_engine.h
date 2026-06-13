#pragma once

#include "yolo_dashboard/inference/inference_engine.h"
#include <vector>
#include <string>

// Forward declarations for ONNX Runtime types
namespace Ort {
    struct Env;
    struct Session;
    struct SessionOptions;
    struct MemoryInfo;
}

namespace yolo_dashboard {

/// ONNX Runtime-based inference engine for YOLOv8 models.
/// Runs on CPU with configurable thread count.
class OnnxEngine : public InferenceEngine {
public:
    OnnxEngine();
    ~OnnxEngine() override;

    bool loadModel(const std::string& model_path) override;
    DetectionResult infer(const cv::Mat& frame) override;
    void unloadModel() override;
    std::string getBackendName() const override { return "ONNX Runtime (CPU)"; }
    ModelInfo getModelInfo() const override;
    bool isModelLoaded() const override;
    void setConfidenceThreshold(float threshold) override;
    void setNmsThreshold(float threshold) override;

    /// Set number of CPU threads for inference.
    void setThreadCount(int threads);

private:
    /// Preprocess frame: resize, normalize, CHW layout, create tensor.
    std::vector<float> preprocess(const cv::Mat& frame);

    /// YOLOv8 post-processing: transpose, threshold, NMS.
    std::vector<BoundingBox> postprocess(const std::vector<float>& output,
                                          int output_rows, int output_cols,
                                          float img_width, float img_height);

    /// Non-Maximum Suppression.
    std::vector<int> nms(const std::vector<BoundingBox>& boxes, float threshold);

    struct Impl;
    std::unique_ptr<Impl> impl_;

    ModelInfo model_info_;
    float confidence_threshold_ = 0.5f;
    float nms_threshold_ = 0.45f;
    int thread_count_ = 4;
    int input_width_ = 640;
    int input_height_ = 640;
    bool model_loaded_ = false;

    // YOLOv8 COCO class names (80 classes)
    static const std::vector<std::string> COCO_CLASSES;
};

} // namespace yolo_dashboard
