#pragma once

#include "yolo_dashboard/common.h"
#include <unordered_map>
#include <string>
#include <mutex>

namespace yolo_dashboard {

/// Simple token-based authentication manager.
/// Uses username/password login with session tokens.
class AuthManager {
public:
    AuthManager();
    ~AuthManager() = default;

    /// Configure the auth system with username and password.
    /// Password is stored as SHA-256 hash.
    void configure(const std::string& username, const std::string& password,
                   int token_ttl_hours = 8);

    /// Attempt login with username and password.
    /// Returns a session token on success, empty string on failure.
    std::string login(const std::string& username, const std::string& password);

    /// Validate a session token.
    bool validateToken(const std::string& token) const;

    /// Invalidate (logout) a session token.
    void logout(const std::string& token);

    /// Clean up expired tokens.
    void cleanupExpiredTokens();

    /// Check if authentication is configured.
    bool isConfigured() const { return !username_.empty(); }

    /// Set the password hash directly (for loading from config).
    void setPasswordHash(const std::string& hash) { password_hash_ = hash; }

    /// Get the current password hash (for saving to config).
    std::string getPasswordHash() const { return password_hash_; }

private:
    /// Generate a cryptographic-quality random token.
    static std::string generateToken();

    /// Hash a password with SHA-256.
    static std::string hashPassword(const std::string& password);

    std::string username_;
    std::string password_hash_;
    int token_ttl_hours_ = 8;

    mutable std::mutex tokens_mutex_;
    std::unordered_map<std::string, AuthToken> active_tokens_;
};

} // namespace yolo_dashboard
