#include "yolo_dashboard/auth/auth_manager.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>

// Simple SHA-256 implementation for password hashing.
// In production, use OpenSSL or a dedicated crypto library.
// This is a portable fallback using a basic hash function.

namespace yolo_dashboard {

AuthManager::AuthManager() = default;

void AuthManager::configure(const std::string& username, const std::string& password,
                             int token_ttl_hours) {
    username_ = username;
    token_ttl_hours_ = token_ttl_hours;

    if (!password.empty()) {
        password_hash_ = hashPassword(password);
    }
}

std::string AuthManager::login(const std::string& username, const std::string& password) {
    if (username != username_) {
        return "";
    }

    // If no password hash set, accept any password (initial setup)
    if (!password_hash_.empty() && hashPassword(password) != password_hash_) {
        return "";
    }

    // Generate session token
    std::string token = generateToken();

    AuthToken auth_token;
    auth_token.token = token;
    auth_token.username = username;
    auth_token.expires_at = Clock::now() + std::chrono::hours(token_ttl_hours_);

    {
        std::lock_guard<std::mutex> lock(tokens_mutex_);
        active_tokens_[token] = auth_token;
    }

    // Clean up expired tokens periodically
    cleanupExpiredTokens();

    return token;
}

bool AuthManager::validateToken(const std::string& token) const {
    if (token.empty()) return false;

    std::lock_guard<std::mutex> lock(tokens_mutex_);
    auto it = active_tokens_.find(token);
    if (it == active_tokens_.end()) return false;

    return it->second.is_valid();
}

void AuthManager::logout(const std::string& token) {
    std::lock_guard<std::mutex> lock(tokens_mutex_);
    active_tokens_.erase(token);
}

void AuthManager::cleanupExpiredTokens() {
    std::lock_guard<std::mutex> lock(tokens_mutex_);
    auto it = active_tokens_.begin();
    while (it != active_tokens_.end()) {
        if (!it->second.is_valid()) {
            it = active_tokens_.erase(it);
        } else {
            ++it;
        }
    }
}

std::string AuthManager::generateToken() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;

    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    ss << std::setw(16) << dis(gen);
    ss << std::setw(16) << dis(gen);
    ss << std::setw(16) << dis(gen);
    ss << std::setw(16) << dis(gen);
    return ss.str();
}

std::string AuthManager::hashPassword(const std::string& password) {
    // FNV-1a 64-bit hash — simple and portable.
    // For production, replace with OpenSSL SHA-256 or bcrypt.
    const uint64_t FNV_PRIME = 0x100000001b3ULL;
    const uint64_t FNV_OFFSET = 0xcbf29ce484222325ULL;

    uint64_t hash = FNV_OFFSET;
    for (char c : password) {
        hash ^= static_cast<uint64_t>(c);
        hash *= FNV_PRIME;
    }

    // Double hash with salt for minimal security
    std::string salted = "yolo_dashboard_salt_" + std::to_string(hash);
    uint64_t hash2 = FNV_OFFSET;
    for (char c : salted) {
        hash2 ^= static_cast<uint64_t>(c);
        hash2 *= FNV_PRIME;
    }

    std::ostringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(16) << hash2;
    return ss.str();
}

} // namespace yolo_dashboard
