#ifndef WEB_SERVICE_HPP
#define WEB_SERVICE_HPP

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include "ConfigService.hpp"
#include "ClockService.hpp"
#include "FeedingService.hpp"

class WebService {
public:
    WebService(ConfigService &config, ClockService &clock, FeedingService &feeding);

    bool begin(uint16_t port = 80);
    void update();

    // Access point mode
    void startAP(const char* ssid = "ChickenFeeder", const char* password = "");
    void stopAP();
    bool isAPActive();

private:
    AsyncWebServer server;
    DNSServer dnsServer;
    ConfigService &configService;
    ClockService &clockService;
    FeedingService &feedingService;

    bool apActive;
    uint32_t apStartTime;
    uint32_t lastClientActivity;
    static const uint32_t AP_TIMEOUT_NO_CLIENT_MS = 60000;  // 60s if no device connected
    static const uint32_t AP_TIMEOUT_WITH_CLIENT_MS = 300000; // 5min after last activity if device connected
    static const uint8_t DNS_PORT = 53;

    // Setup routes
    void setupRoutes();

    // API handlers
    void handleGetStatus(AsyncWebServerRequest *request);
    void handleGetConfig(AsyncWebServerRequest *request);
    void handlePostConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len);
    void handlePostFeed(AsyncWebServerRequest *request);
    void handlePostTime(AsyncWebServerRequest *request, uint8_t *data, size_t len);
    void handleResetConfig(AsyncWebServerRequest *request);

    // Static file handler
    void handleStaticFile(AsyncWebServerRequest *request, const char* path);

    // Helper methods
    void sendJsonResponse(AsyncWebServerRequest *request, JsonDocument &doc, int statusCode = 200);
    void sendError(AsyncWebServerRequest *request, const char* message, int statusCode = 400);
    void updateClientActivity();
};

#endif // WEB_SERVICE_HPP
