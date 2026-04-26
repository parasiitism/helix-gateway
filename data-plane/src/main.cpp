// ═══════════════════════════════════════════════════════════════
//  Helix Data Plane — main entry point
//
//  Day 1 scope:
//    - Drogon HTTP server on :8080
//    - GET  /healthz          → liveness probe
//    - GET  /readyz           → readiness probe
//    - GET  /version          → build info
//    - POST /v1/chat/completions → returns 501 Not Implemented stub
//
//  This file boots cleanly, responds to curl, and is the foundation
//  every later module plugs into.
// ═══════════════════════════════════════════════════════════════

#include <drogon/drogon.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <string>
#include <thread>

#ifndef HELIX_VERSION
#define HELIX_VERSION "0.1.0-dev"
#endif

namespace {

// Global readiness flag — flipped to true once startup is complete.
// Atomic so any thread can read it lock-free.
std::atomic<bool> g_ready{false};

// Captures process start time for /version uptime reporting
const auto g_start_time = std::chrono::steady_clock::now();

/**
 * Read a string from environment, fall back to default.
 * Used so users can override host/port via env vars without recompiling.
 */
std::string env_or(const char* key, const std::string& fallback) {
    const char* v = std::getenv(key);
    return v ? std::string(v) : fallback;
}

/**
 * Read an integer from environment, fall back to default.
 */
int env_or_int(const char* key, int fallback) {
    const char* v = std::getenv(key);
    if (!v) return fallback;
    try {
        return std::stoi(v);
    } catch (...) {
        return fallback;
    }
}

/**
 * Configure spdlog with colored stdout, JSON-style structured fields.
 * In production you'd swap this for an async sink + JSON formatter.
 */
void setup_logging() {
    auto logger = spdlog::stdout_color_mt("helix");
    spdlog::set_default_logger(logger);

    const std::string level = env_or("HELIX_LOG_LEVEL", "info");
    if      (level == "trace") spdlog::set_level(spdlog::level::trace);
    else if (level == "debug") spdlog::set_level(spdlog::level::debug);
    else if (level == "info")  spdlog::set_level(spdlog::level::info);
    else if (level == "warn")  spdlog::set_level(spdlog::level::warn);
    else if (level == "error") spdlog::set_level(spdlog::level::err);

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
}

/**
 * Helper: build a JSON HttpResponse with a given status code.
 */
drogon::HttpResponsePtr json_response(drogon::HttpStatusCode code,
                                      const std::string& body) {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(code);
    resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    resp->setBody(body);
    return resp;
}

/**
 * Register all HTTP route handlers.
 *
 * Drogon handler signature:
 *   (HttpRequestPtr req, std::function<void(const HttpResponsePtr&)>&& cb)
 *
 * The callback model is asynchronous-friendly — perfect for upcoming
 * provider proxy code that will await upstream responses.
 */
void register_routes() {
    using drogon::Get;
    using drogon::Post;

    // ── /healthz — liveness probe ───────────────────────────────
    drogon::app().registerHandler(
        "/healthz",
        [](const drogon::HttpRequestPtr&,
           std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            cb(json_response(drogon::k200OK, R"({"status":"ok"})"));
        },
        {Get});

    // ── /readyz — readiness probe ───────────────────────────────
    // Kubernetes uses this to decide whether to send traffic.
    drogon::app().registerHandler(
        "/readyz",
        [](const drogon::HttpRequestPtr&,
           std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            if (g_ready.load(std::memory_order_acquire)) {
                cb(json_response(drogon::k200OK, R"({"status":"ready"})"));
            } else {
                cb(json_response(drogon::k503ServiceUnavailable,
                                 R"({"status":"starting"})"));
            }
        },
        {Get});

    // ── /version — build / runtime info ─────────────────────────
    drogon::app().registerHandler(
        "/version",
        [](const drogon::HttpRequestPtr&,
           std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            const auto uptime_s = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - g_start_time).count();

            std::string body = R"({"version":")" + std::string(HELIX_VERSION)
                + R"(","uptime_seconds":)" + std::to_string(uptime_s) + "}";

            cb(json_response(drogon::k200OK, body));
        },
        {Get});

    // ── /v1/chat/completions — OpenAI-compatible stub ──────────
    // This will become the gateway hot path in Week 2.
    drogon::app().registerHandler(
        "/v1/chat/completions",
        [](const drogon::HttpRequestPtr& req,
           std::function<void(const drogon::HttpResponsePtr&)>&& cb) {
            spdlog::info("chat-completions request received (stub)");
            cb(json_response(drogon::k501NotImplemented,
                R"({"error":{"message":"Not yet implemented","type":"stub"}})"));
        },
        {Post});

    spdlog::info("Routes registered: /healthz /readyz /version /v1/chat/completions");
}

/**
 * Graceful shutdown handler — fires on SIGINT / SIGTERM.
 * Flips readiness to false so Kubernetes pulls us from rotation,
 * then asks Drogon's event loop to stop.
 */
void install_signal_handlers() {
    std::signal(SIGINT,  [](int) {
        spdlog::warn("SIGINT received — shutting down");
        g_ready.store(false, std::memory_order_release);
        drogon::app().getLoop()->runAfter(0.5, []() {
            drogon::app().quit();
        });
    });

    std::signal(SIGTERM, [](int) {
        spdlog::warn("SIGTERM received — shutting down");
        g_ready.store(false, std::memory_order_release);
        drogon::app().getLoop()->runAfter(0.5, []() {
            drogon::app().quit();
        });
    });
}

}  // anonymous namespace

// ═══════════════════════════════════════════════════════════════
//  main
// ═══════════════════════════════════════════════════════════════

int main(int argc, char** argv) {
    setup_logging();

    const auto host    = env_or("HELIX_HOST", "0.0.0.0");
    const auto port    = env_or_int("HELIX_PORT", 8080);
    const auto threads = env_or_int("HELIX_THREADS", 0);  // 0 == hardware_concurrency

    spdlog::info("═══════════════════════════════════════════════");
    spdlog::info("  Helix Gateway v{}", HELIX_VERSION);
    spdlog::info("  Listening on:  http://{}:{}", host, port);
    spdlog::info("  Threads:       {}", threads == 0 ? "auto" : std::to_string(threads));
    spdlog::info("═══════════════════════════════════════════════");

    register_routes();
    install_signal_handlers();

    // Mark ready right before starting the loop. In later weeks this
    // will be deferred until Redis, gRPC config, and providers are wired.
    g_ready.store(true, std::memory_order_release);
    spdlog::info("Helix is READY");

    // .run() blocks until quit() is called.
    drogon::app()
        .addListener(host, port)
        .setThreadNum(threads)
        .setIdleConnectionTimeout(60)
        .run();

    spdlog::info("Helix exited cleanly");
    return 0;
}