/// ONNX Runtime inference engine for YOLOv8n models.
/// Handles preprocessing (resize/normalize/CHW), inference, and
/// YOLOv8 post-processing (transpose, threshold, NMS).

#include "yolo_dashboard/inference/onnx_engine.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <chrono>

// ONNX Runtime headers
#ifdef ONNXRUNTIME_ENABLED
#include <onnxruntime_cxx_api.h>
#endif

namespace yolo_dashboard {

// YOLOv8 COCO class names
const std::vector<std::string> OnnxEngine::COCO_CLASSES = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck",
    "boat", "traffic light", "fire hydrant", "stop sign", "parking meter", "bench",
    "bird", "cat", "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra",
    "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove",
    "skateboard", "surfboard", "tennis racket", "bottle", "wine glass", "cup",
    "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange",
    "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
    "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse",
    "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink",
    "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
    "hair drier", "toothbrush"
};

struct OnnxEngine::Impl {
#ifdef ONNXRUNTIME_ENABLED
    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<Ort::Session> session;
    std::unique_ptr<Ort::SessionOptions> session_options;
    Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    std::vector<std::string> input_names;
    std::vector<std::string> output_names;
    std::vector<const char*> input_names_ptrs;
    std::vector<const char*> output_names_ptrs;
    std::vector<int64_t> input_shape;
    std::vector<int64_t> output_shape;
#endif
};

OnnxEngine::OnnxEngine() : impl_(std::make_unique<Impl>()) {
#ifdef ONNXRUNTIME_ENABLED
    impl_->env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "YoloDashboard");
#endif
}

OnnxEngine::~OnnxEngine() {
    unloadModel();
}

bool OnnxEngine::loadModel(const std::string& model_path) {
#ifdef ONNXRUNTIME_ENABLED
    try {
        unloadModel();

        // Configure session options
        impl_->session_options = std::make_unique<Ort::SessionOptions>();
        impl_->session_options->SetIntraOpNumThreads(thread_count_);
        impl_->session_options->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        // Create session
#ifdef _WIN32
        // Windows needs wide string path
        std::wstring wpath(model_path.begin(), model_path.end());
        impl_->session = std::make_unique<Ort::Session>(*impl_->env, wpath.c_str(),
                                                         *impl_->session_options);
#else
        impl_->session = std::make_unique<Ort::Session>(*impl_->env, model_path.c_str(),
                                                         *impl_->session_options);
#endif

        // Get input info
        Ort::AllocatorWithDefaultOptions allocator;
        size_t num_inputs = impl_->session->GetInputCount();
        for (size_t i = 0; i < num_inputs; i++) {
            auto name = impl_->session->GetInputNameAllocated(i, allocator);
            impl_->input_names.push_back(name.get());
            impl_->input_names_ptrs.push_back(impl_->input_names.back().c_str());
        }

        // Get input shape
        auto input_info = impl_->session->GetInputTypeInfo(0);
        auto tensor_info = input_info.GetTensorTypeAndShapeInfo();
        impl_->input_shape = tensor_info.GetShape();

        // Dynamic batch size: set to 1
        if (impl_->input_shape[0] == -1) impl_->input_shape[0] = 1;

        input_height_ = static_cast<int>(impl_->input_shape[2]);
        input_width_ = static_cast<int>(impl_->input_shape[3]);
        
        // Handle dynamic spatial dimensions (-1) by falling back to 640x640
        if (input_height_ <= 0) {
            input_height_ = 640;
            impl_->input_shape[2] = 640;
        }
        if (input_width_ <= 0) {
            input_width_ = 640;
            impl_->input_shape[3] = 640;
        }

        // Get output info
        size_t num_outputs = impl_->session->GetOutputCount();
        for (size_t i = 0; i < num_outputs; i++) {
            auto name = impl_->session->GetOutputNameAllocated(i, allocator);
            impl_->output_names.push_back(name.get());
            impl_->output_names_ptrs.push_back(impl_->output_names.back().c_str());
        }

        auto output_info = impl_->session->GetOutputTypeInfo(0);
        auto output_tensor_info = output_info.GetTensorTypeAndShapeInfo();
        impl_->output_shape = output_tensor_info.GetShape();

        // Update model info
        model_info_.path = model_path;
        model_info_.name = fs::path(model_path).filename().string();
        model_info_.format = "onnx";
        model_info_.input_shape = impl_->input_shape;
        model_info_.output_shape = impl_->output_shape;
        model_info_.is_loaded = true;

        if (fs::exists(model_path)) {
            model_info_.file_size_bytes = fs::file_size(model_path);
        }

        model_loaded_ = true;
        std::cout << "[ONNX] Model loaded: " << model_info_.name
                  << " (input: " << input_width_ << "x" << input_height_ << ")" << std::endl;
        return true;

    } catch (const Ort::Exception& e) {
        std::cerr << "[ONNX] Failed to load model: " << e.what() << std::endl;
        return false;
    }
#else
    std::cerr << "[ONNX] ONNX Runtime not available (not compiled with ONNXRUNTIME_ENABLED)" << std::endl;
    // Store model info even if we can't load it
    model_info_.path = model_path;
    model_info_.name = fs::path(model_path).filename().string();
    model_info_.format = "onnx";
    model_info_.is_loaded = false;
    if (fs::exists(model_path)) {
        model_info_.file_size_bytes = fs::file_size(model_path);
    }
    return false;
#endif
}

DetectionResult OnnxEngine::infer(const cv::Mat& frame) {
    DetectionResult result;

#ifdef ONNXRUNTIME_ENABLED
    if (!model_loaded_ || !impl_->session) return result;

    auto total_start = Clock::now();

    // 1. Preprocess
    auto pre_start = Clock::now();
    auto input_tensor_values = preprocess(frame);
    result.preprocess_time_ms = std::chrono::duration<double, std::milli>(Clock::now() - pre_start).count();

    // 2. Run inference
    auto infer_start = Clock::now();

    try {
        auto input_tensor = Ort::Value::CreateTensor<float>(
            impl_->memory_info,
            input_tensor_values.data(), input_tensor_values.size(),
            impl_->input_shape.data(), impl_->input_shape.size()
        );

        auto output_tensors = impl_->session->Run(
            Ort::RunOptions{nullptr},
            impl_->input_names_ptrs.data(), &input_tensor, 1,
            impl_->output_names_ptrs.data(), impl_->output_names_ptrs.size()
        );

        result.inference_time_ms = std::chrono::duration<double, std::milli>(Clock::now() - infer_start).count();

        // 3. Post-process
        auto post_start = Clock::now();

        auto& output_tensor = output_tensors[0];
        auto output_shape = output_tensor.GetTensorTypeAndShapeInfo().GetShape();
        float* output_data = output_tensor.GetTensorMutableData<float>();

        // YOLOv8 output: [1, 84, 8400] → need to transpose to [8400, 84]
        int rows = static_cast<int>(output_shape[1]); // 84 (4 bbox + 80 classes)
        int cols = static_cast<int>(output_shape[2]); // 8400 detections

        std::vector<float> output_vec(output_data, output_data + rows * cols);
        result.detections = postprocess(output_vec, rows, cols,
                                         static_cast<float>(frame.cols),
                                         static_cast<float>(frame.rows));

        result.postprocess_time_ms = std::chrono::duration<double, std::milli>(Clock::now() - post_start).count();

    } catch (const Ort::Exception& e) {
        std::cerr << "[ONNX] Inference error: " << e.what() << std::endl;
    }
#endif

    return result;
}

void OnnxEngine::unloadModel() {
#ifdef ONNXRUNTIME_ENABLED
    impl_->session.reset();
    impl_->session_options.reset();
    impl_->input_names.clear();
    impl_->output_names.clear();
    impl_->input_names_ptrs.clear();
    impl_->output_names_ptrs.clear();
    impl_->input_shape.clear();
    impl_->output_shape.clear();
#endif
    model_loaded_ = false;
    model_info_ = ModelInfo{};
}

ModelInfo OnnxEngine::getModelInfo() const {
    return model_info_;
}

bool OnnxEngine::isModelLoaded() const {
    return model_loaded_;
}

void OnnxEngine::setConfidenceThreshold(float threshold) {
    confidence_threshold_ = threshold;
}

void OnnxEngine::setNmsThreshold(float threshold) {
    nms_threshold_ = threshold;
}

void OnnxEngine::setThreadCount(int threads) {
    thread_count_ = threads;
}

std::vector<float> OnnxEngine::preprocess(const cv::Mat& frame) {
    cv::Mat resized, blob;

    // Resize to input dimensions
    cv::resize(frame, resized, cv::Size(input_width_, input_height_));

    // Convert BGR to RGB and normalize to [0, 1]
    cv::cvtColor(resized, resized, cv::COLOR_BGR2RGB);
    resized.convertTo(blob, CV_32F, 1.0 / 255.0);

    // HWC to CHW layout
    std::vector<float> input(3 * input_height_ * input_width_);
    std::vector<cv::Mat> channels(3);
    cv::split(blob, channels);

    for (int c = 0; c < 3; c++) {
        memcpy(input.data() + c * input_height_ * input_width_,
               channels[c].data,
               input_height_ * input_width_ * sizeof(float));
    }

    return input;
}

std::vector<BoundingBox> OnnxEngine::postprocess(const std::vector<float>& output,
                                                   int rows, int cols,
                                                   float img_width, float img_height) {
    // YOLOv8 output format: [1, 84, 8400]
    // rows = 84 (4 bbox coords + 80 class scores)
    // cols = 8400 (number of detections)
    // Data is in column-major order: need to transpose

    int num_classes = rows - 4;
    std::vector<BoundingBox> candidates;

    float x_scale = img_width / input_width_;
    float y_scale = img_height / input_height_;

    for (int i = 0; i < cols; i++) {
        // Extract class scores for this detection
        float max_score = 0;
        int max_class = -1;

        for (int c = 4; c < rows; c++) {
            float score = output[c * cols + i]; // Transposed access
            if (score > max_score) {
                max_score = score;
                max_class = c - 4;
            }
        }

        if (max_score < confidence_threshold_) continue;

        // Extract bounding box (cx, cy, w, h) → (x1, y1, x2, y2)
        float cx = output[0 * cols + i];
        float cy = output[1 * cols + i];
        float w  = output[2 * cols + i];
        float h  = output[3 * cols + i];

        BoundingBox box;
        box.x1 = (cx - w / 2.0f) * x_scale;
        box.y1 = (cy - h / 2.0f) * y_scale;
        box.x2 = (cx + w / 2.0f) * x_scale;
        box.y2 = (cy + h / 2.0f) * y_scale;
        box.confidence = max_score;
        box.class_id = max_class;

        if (max_class >= 0 && max_class < static_cast<int>(COCO_CLASSES.size())) {
            box.class_name = COCO_CLASSES[max_class];
        } else {
            box.class_name = "class_" + std::to_string(max_class);
        }

        // Clamp to image bounds
        box.x1 = std::max(0.0f, std::min(box.x1, img_width));
        box.y1 = std::max(0.0f, std::min(box.y1, img_height));
        box.x2 = std::max(0.0f, std::min(box.x2, img_width));
        box.y2 = std::max(0.0f, std::min(box.y2, img_height));

        candidates.push_back(box);
    }

    // Apply NMS
    auto keep_indices = nms(candidates, nms_threshold_);

    std::vector<BoundingBox> result;
    result.reserve(keep_indices.size());
    for (int idx : keep_indices) {
        result.push_back(candidates[idx]);
    }

    return result;
}

std::vector<int> OnnxEngine::nms(const std::vector<BoundingBox>& boxes, float threshold) {
    if (boxes.empty()) return {};

    // Sort by confidence (descending)
    std::vector<int> indices(boxes.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(),
              [&boxes](int a, int b) { return boxes[a].confidence > boxes[b].confidence; });

    std::vector<bool> suppressed(boxes.size(), false);
    std::vector<int> keep;

    for (int i : indices) {
        if (suppressed[i]) continue;
        keep.push_back(i);

        for (int j : indices) {
            if (suppressed[j] || i == j) continue;

            // Calculate IoU
            float xx1 = std::max(boxes[i].x1, boxes[j].x1);
            float yy1 = std::max(boxes[i].y1, boxes[j].y1);
            float xx2 = std::min(boxes[i].x2, boxes[j].x2);
            float yy2 = std::min(boxes[i].y2, boxes[j].y2);

            float w = std::max(0.0f, xx2 - xx1);
            float h = std::max(0.0f, yy2 - yy1);
            float intersection = w * h;

            float area_i = (boxes[i].x2 - boxes[i].x1) * (boxes[i].y2 - boxes[i].y1);
            float area_j = (boxes[j].x2 - boxes[j].x1) * (boxes[j].y2 - boxes[j].y1);
            float union_area = area_i + area_j - intersection;

            float iou = union_area > 0 ? intersection / union_area : 0;
            if (iou > threshold) {
                suppressed[j] = true;
            }
        }
    }

    return keep;
}

// Factory method
std::unique_ptr<InferenceEngine> InferenceEngine::create(InferenceMode mode) {
    switch (mode) {
        case InferenceMode::CPU:
            return std::make_unique<OnnxEngine>();
        case InferenceMode::NPU:
#ifdef HAILO_ENABLED
            // TODO: Return HailoEngine when implemented
            std::cerr << "[Inference] Hailo engine not yet implemented, falling back to CPU" << std::endl;
#else
            std::cerr << "[Inference] NPU mode requested but HAILO_ENABLED is not defined, using CPU" << std::endl;
#endif
            return std::make_unique<OnnxEngine>();
        default:
            return std::make_unique<OnnxEngine>();
    }
}

} // namespace yolo_dashboard
