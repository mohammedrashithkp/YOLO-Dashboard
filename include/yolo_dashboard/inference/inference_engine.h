#pragma once

#include "yolo_dashboard/common.h"
#include <opencv2/core.hpp>

namespace yolo_dashboard {

/// Abstract interface for YOLO inference engines.
/// Implementations: OnnxEngine (CPU), HailoEngine (NPU).
class InferenceEngine {
public:
    virtual ~InferenceEngine() = default;

    /// Load a model from the given file path.
    virtual bool loadModel(const std::string& model_path) = 0;

    /// Run inference on a single frame and return detection results.
    virtual DetectionResult infer(const cv::Mat& frame) = 0;

    /// Unload the currently loaded model and free resources.
    virtual void unloadModel() = 0;

    /// Get the name of this inference backend.
    virtual std::string getBackendName() const = 0;

    /// Get information about the currently loaded model.
    virtual ModelInfo getModelInfo() const = 0;

    /// Check if a model is currently loaded.
    virtual bool isModelLoaded() const = 0;

    /// Set confidence threshold for detections.
    virtual void setConfidenceThreshold(float threshold) = 0;

    /// Set NMS (Non-Maximum Suppression) threshold.
    virtual void setNmsThreshold(float threshold) = 0;

    /// Factory method: create the appropriate engine based on mode.
    static std::unique_ptr<InferenceEngine> create(InferenceMode mode);
};

} // namespace yolo_dashboard
