#include "yolo_dashboard/web/routes.h"
#include "yolo_dashboard/web/websocket_handler.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace yolo_dashboard {

// Forward declaration of WebSocketHandler that will be instantiated in registerRoutes
static std::unique_ptr<WebSocketHandler> ws_handler;

RouteHandler::RouteHandler(CameraManager& camera, VideoRecorder& recorder,
                           ConfigManager& config, AuthManager& auth,
                           MetricsCollector& metrics, LogManager& logger)
    : camera_(camera), recorder_(recorder), config_(config),
      auth_(auth), metrics_(metrics), logger_(logger) {
          
    // Initialize inference engine based on config
    inference_engine_ = InferenceEngine::create(
        config_.getConfig().inference_mode == "npu" ? InferenceMode::NPU : InferenceMode::CPU
    );
    
    ws_handler = std::make_unique<WebSocketHandler>(camera_, metrics_);
    ws_handler->setInferenceEngine(inference_engine_.get());
}

bool RouteHandler::checkAuth(const crow::request& req, crow::response& res) {
    if (!auth_.isConfigured()) return true;

    auto auth_header = req.get_header_value("Authorization");
    if (auth_header.find("Bearer ") == 0) {
        std::string token = auth_header.substr(7);
        if (auth_.validateToken(token)) return true;
    }

    res.code = 401;
    res.body = R"({"error": "Unauthorized"})";
    return false;
}

void RouteHandler::registerRoutes(crow::SimpleApp& app) {
    
    // SPA Fallback and Custom 404 Handler
    CROW_CATCHALL_ROUTE(app)
    ([&](const crow::request& req, crow::response& res) {
        std::string path = req.url;
        if (!path.empty() && path[0] == '/') path = path.substr(1);

        if (path.find("api/") == 0 || path.find("ws/") == 0) {
            res.code = 404;
            res.end();
            return;
        }
        
        std::string full_path = "web/" + path;
        if (!path.empty() && std::filesystem::exists(full_path) && std::filesystem::is_regular_file(full_path)) {
            res.set_static_file_info(full_path);
        } else {
            res.set_static_file_info("web/index.html");
        }
        res.end();
    });

    // Auth
    CROW_ROUTE(app, "/api/auth/login").methods(crow::HTTPMethod::POST)
    ([&](const crow::request& req) { return handleLogin(req); });
    
    CROW_ROUTE(app, "/api/auth/logout").methods(crow::HTTPMethod::POST)
    ([&](const crow::request& req) { return handleLogout(req); });

    // API Routes (Need Auth)
    #define AUTH_GUARD() crow::response res; if (!checkAuth(req, res)) return res;

    // Hardware
    CROW_ROUTE(app, "/api/hardware")
    ([&](const crow::request& req) { AUTH_GUARD(); return handleDetectHardware(req); });

    // Camera
    CROW_ROUTE(app, "/api/cameras")
    ([&](const crow::request& req) { AUTH_GUARD(); return handleListCameras(req); });
    
    CROW_ROUTE(app, "/api/cameras/select").methods(crow::HTTPMethod::POST)
    ([&](const crow::request& req) { AUTH_GUARD(); return handleSelectCamera(req); });

    // Models
    CROW_ROUTE(app, "/api/models/browse")
    ([&](const crow::request& req) { AUTH_GUARD(); return handleBrowseModels(req); });

    // Inference
    CROW_ROUTE(app, "/api/inference/start").methods(crow::HTTPMethod::POST)
    ([&](const crow::request& req) { AUTH_GUARD(); return handleStartInference(req); });
    
    CROW_ROUTE(app, "/api/inference/stop").methods(crow::HTTPMethod::POST)
    ([&](const crow::request& req) { AUTH_GUARD(); return handleStopInference(req); });
    
    CROW_ROUTE(app, "/api/inference/status")
    ([&](const crow::request& req) { AUTH_GUARD(); return handleInferenceStatus(req); });

    // Config
    CROW_ROUTE(app, "/api/config")
    ([&](const crow::request& req) { AUTH_GUARD(); return handleGetConfig(req); });
    
    CROW_ROUTE(app, "/api/config").methods(crow::HTTPMethod::POST)
    ([&](const crow::request& req) { AUTH_GUARD(); return handleUpdateConfig(req); });

    // MJPEG Stream over HTTP
    CROW_ROUTE(app, "/api/stream")
    ([&](const crow::request& req, crow::response& res) {
        if (!auth_.isConfigured() || checkAuth(req, res)) {
            handleMjpegStream(req, res);
        }
    });

    // Register WebSockets
    ws_handler->registerRoutes(app);
    
    // Background thread for pushing frames/metrics to websockets
    std::thread([&]() {
        while(true) {
            ws_handler->broadcastUpdate();
            std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30fps max
        }
    }).detach();
}

crow::response RouteHandler::handleLogin(const crow::request& req) {
    auto body = crow::json::load(req.body);
    if (!body || !body.has("username") || !body.has("password")) {
        return crow::response(400, R"({"error": "Missing username or password"})");
    }
    
    std::string token = auth_.login(body["username"].s(), body["password"].s());
    if (token.empty()) {
        return crow::response(401, R"({"error": "Invalid credentials"})");
    }
    
    crow::json::wvalue res_body;
    res_body["token"] = token;
    return crow::response(200, res_body);
}

crow::response RouteHandler::handleLogout(const crow::request& req) {
    auto auth_header = req.get_header_value("Authorization");
    if (auth_header.find("Bearer ") == 0) {
        auth_.logout(auth_header.substr(7));
    }
    return crow::response(200, R"({"status": "success"})");
}

crow::response RouteHandler::handleDetectHardware(const crow::request& /*req*/) {
    auto hw = HardwareDetector::detectAll();
    crow::json::wvalue res;
    
    res["cpu"]["model"] = hw.cpu.model_name;
    res["cpu"]["cores"] = hw.cpu.cores;
    res["cpu"]["arch"] = hw.cpu.architecture;
    
    res["memory"]["total_mb"] = hw.memory.total_mb;
    res["memory"]["available_mb"] = hw.memory.available_mb;
    
    int i = 0;
    for (const auto& npu : hw.npus) {
        res["npus"][i]["name"] = npu.name;
        res["npus"][i]["tops"] = npu.tops;
        res["npus"][i]["available"] = npu.is_available;
        i++;
    }
    if (hw.npus.empty()) {
        res["npus"] = std::vector<std::string>(); // empty list
    }
    
    return crow::response(200, res);
}

crow::response RouteHandler::handleListCameras(const crow::request& /*req*/) {
    auto cameras = camera_.listCameras();
    crow::json::wvalue res;
    int i = 0;
    for (const auto& cam : cameras) {
        res[i]["id"] = cam.id;
        res[i]["name"] = cam.name;
        res[i]["device_path"] = cam.device_path;
        i++;
    }
    if (cameras.empty()) {
        res = std::vector<std::string>();
    }
    return crow::response(200, res);
}

crow::response RouteHandler::handleSelectCamera(const crow::request& req) {
    auto body = crow::json::load(req.body);
    if (!body || !body.has("id")) {
        return crow::response(400, "Missing camera ID");
    }
    int id = body["id"].i();
    
    Resolution res{640, 480};
    if (body.has("width") && body.has("height")) {
        res.width = body["width"].i();
        res.height = body["height"].i();
    }
    
    if (camera_.openCamera(id, res, 30)) {
        return crow::response(200, R"({"status": "success"})");
    }
    return crow::response(500, R"({"error": "Failed to open camera"})");
}

crow::response RouteHandler::handleBrowseModels(const crow::request& req) {
    std::string path = req.url_params.get("path") ? req.url_params.get("path") : "./data/models";
    
    if (!fs::exists(path)) {
        try { fs::create_directories(path); } catch(...) {}
    }
    
    crow::json::wvalue res;
    int i = 0;
    if (fs::exists(path) && fs::is_directory(path)) {
        for (const auto& entry : fs::directory_iterator(path)) {
            std::string ext = entry.path().extension().string();
            if (entry.is_directory() || ext == ".onnx" || ext == ".hef") {
                res[i]["name"] = entry.path().filename().string();
                res[i]["path"] = entry.path().string();
                res[i]["is_dir"] = entry.is_directory();
                if (!entry.is_directory()) {
                    res[i]["size"] = static_cast<uint64_t>(entry.file_size());
                }
                i++;
            }
        }
    }
    if (i == 0) res = std::vector<std::string>();
    return crow::response(200, res);
}

crow::response RouteHandler::handleStartInference(const crow::request& req) {
    auto body = crow::json::load(req.body);
    if (!body || !body.has("model_path")) {
        return crow::response(400, "Missing model_path");
    }
    
    std::lock_guard<std::mutex> lock(inference_mutex_);
    
    if (inference_engine_->loadModel(body["model_path"].s())) {
        inference_running_ = true;
        ws_handler->setInferenceRunning(true);
        return crow::response(200, R"({"status": "started"})");
    }
    
    return crow::response(500, R"({"error": "Failed to load model"})");
}

crow::response RouteHandler::handleStopInference(const crow::request& /*req*/) {
    std::lock_guard<std::mutex> lock(inference_mutex_);
    inference_running_ = false;
    ws_handler->setInferenceRunning(false);
    inference_engine_->unloadModel();
    return crow::response(200, R"({"status": "stopped"})");
}

crow::response RouteHandler::handleInferenceStatus(const crow::request& /*req*/) {
    crow::json::wvalue res;
    res["running"] = inference_running_.load();
    if (inference_engine_->isModelLoaded()) {
        auto info = inference_engine_->getModelInfo();
        res["model_name"] = info.name;
        res["model_format"] = info.format;
    }
    return crow::response(200, res);
}

crow::response RouteHandler::handleGetConfig(const crow::request& /*req*/) {
    auto parsed = crow::json::load(config_.toYamlString());
    if (!parsed) {
        crow::response r(200, config_.toYamlString());
        r.add_header("Content-Type", "application/x-yaml");
        return r;
    }
    
    crow::json::wvalue res(parsed);
    return crow::response(200, res);
}

crow::response RouteHandler::handleUpdateConfig(const crow::request& req) {
    if (config_.updateFromString(req.body)) {
        config_.saveToFile(config_.getCurrentFilePath());
        return crow::response(200, R"({"status": "success"})");
    }
    return crow::response(400, R"({"error": "Invalid config"})");
}

void RouteHandler::handleMjpegStream(const crow::request& /*req*/, crow::response& res) {
    res.set_header("Content-Type", "multipart/x-mixed-replace; boundary=frame");
    
    while (camera_.isOpen()) {
        std::vector<uint8_t> buf;
        if (camera_.getLatestJpeg(buf)) {
            std::string data(buf.begin(), buf.end());
            std::string part = "--frame\r\nContent-Type: image/jpeg\r\n\r\n" + data + "\r\n";
            res.write(part);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    res.end();
}

} // namespace yolo_dashboard
