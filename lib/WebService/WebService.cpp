#include "WebService.hpp"
#include "SchedulingService.hpp"
#include "generated/web_files.h"
#include <WiFi.h>

WebService::WebService(ConfigService &config, ClockService &clock, FeedingService &feeding, SchedulingService &scheduling)
    : server(80), configService(config), clockService(clock), feedingService(feeding), schedulingService(scheduling),
      apActive(false), apStartTime(0), lastClientActivity(0) {}

bool WebService::begin(uint16_t port) {
    setupRoutes();
    // Don't start server yet - will start when AP mode is activated
    Serial.printf("[WEB] WebService initialized (server will start with AP mode)\n");
    return true;
}

void WebService::update() {
    if (apActive) {
        // Process DNS requests for captive portal
        dnsServer.processNextRequest();

        uint32_t now = millis();
        uint32_t timeSinceStart = now - apStartTime;

        // Check how many stations (devices) are connected
        uint8_t stationCount = WiFi.softAPgetStationNum();

        // Timeout logic: Only stop AP if no device is connected
        if (stationCount == 0) {
            // No device connected - timeout after 60 seconds from start
            if (timeSinceStart > AP_TIMEOUT_NO_CLIENT_MS) {
                Serial.println("[WEB] AP timeout (no client connected for 60s) - stopping AP mode");
                stopAP();
            }
        }
        // If device(s) are connected, keep AP running indefinitely
        // User must manually stop by disconnecting or the device will auto-disconnect eventually
    }

    // Handle deferred sleep request after responses have been sent
    if (sleepRequested && sleepCallback) {
        if (millis() - sleepRequestMillis > 200) { // small grace period to flush response
            sleepRequested = false;
            // Stop AP before sleeping
            if (apActive) {
                stopAP();
            }
            sleepCallback();
        }
    }
}

void WebService::startAP(const char* ssid, const char* password) {
    if (apActive) {
        Serial.println("[WEB] AP mode already active");
        lastClientActivity = millis(); // Reset timeout
        return;
    }

    Serial.printf("[WEB] Starting AP mode: %s\n", ssid);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);

    IPAddress IP = WiFi.softAPIP();
    Serial.printf("[WEB] AP IP address: %s\n", IP.toString().c_str());

    // Start DNS server for captive portal
    // Redirect all DNS requests to our IP
    dnsServer.start(DNS_PORT, "*", IP);
    Serial.println("[WEB] DNS server started for captive portal");

    // Start web server
    server.begin();
    Serial.println("[WEB] Web server started");

    apActive = true;
    apStartTime = millis();
    lastClientActivity = millis();
}

void WebService::stopAP() {
    if (!apActive) return;

    Serial.println("[WEB] Stopping AP mode");

    // Stop DNS server
    dnsServer.stop();

    // Stop web server
    server.end();

    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    apActive = false;

    Serial.println("[WEB] AP mode stopped");
}

bool WebService::isAPActive() {
    return apActive;
}

void WebService::setupRoutes() {
    // Root serves index.html
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        updateClientActivity();
        handleStaticFile(request, "/index.html");
    });

    // API endpoints
    server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        updateClientActivity();
        handleGetStatus(request);
    });

    server.on("/api/status/history", HTTP_GET, [this](AsyncWebServerRequest *request) {
        updateClientActivity();
        handleGetFeedHistory(request);
    });

    server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest *request) {
        updateClientActivity();
        handleGetConfig(request);
    });

    server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *request) {},
              NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        updateClientActivity();
        handlePostConfig(request, data, len);
    });

    server.on("/api/feed", HTTP_POST, [this](AsyncWebServerRequest *request) {
        updateClientActivity();
        handlePostFeed(request);
    });

    server.on("/api/time", HTTP_POST, [](AsyncWebServerRequest *request) {},
              NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        updateClientActivity();
        handlePostTime(request, data, len);
    });

    server.on("/api/power/sleep", HTTP_POST, [this](AsyncWebServerRequest *request) {
        updateClientActivity();
        handleSleep(request);
    });

    server.on("/api/config/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
        updateClientActivity();
        handleResetConfig(request);
    });

    // OTA endpoints
    server.on("/api/ota/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        updateClientActivity();
        handleOtaStatus(request);
    });

    server.on("/api/ota/update", HTTP_POST,
              [this](AsyncWebServerRequest *request) {
                  // Request complete callback
                  updateClientActivity();
              },
              [this](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
                  // Upload handler
                  handleOtaUpdate(request, filename, index, data, len, final);
              });

    // Captive portal detection endpoints
    // Android
    server.on("/generate_204", HTTP_GET, [this](AsyncWebServerRequest *request) {
        updateClientActivity();
        request->redirect("/");
    });

    // Apple iOS
    server.on("/hotspot-detect.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
        updateClientActivity();
        request->redirect("/");
    });

    // Microsoft Windows
    server.on("/connecttest.txt", HTTP_GET, [this](AsyncWebServerRequest *request) {
        updateClientActivity();
        request->redirect("/");
    });

    // Captive portal handler - serve files from webFiles array or redirect to index
    server.onNotFound([this](AsyncWebServerRequest *request) {
        updateClientActivity();

        // If request is for API, return error
        if (request->url().startsWith("/api/")) {
            sendError(request, "Not found", 404);
            return;
        }

        // Try to serve the requested file from webFiles array
        String url = request->url();
        handleStaticFile(request, url.c_str());
    });
}

void WebService::handleStaticFile(AsyncWebServerRequest *request, const char* path) {
    // Find file in webFiles array
    for (size_t i = 0; i < webFilesCount; i++) {
        if (strcmp(webFiles[i].path, path) == 0) {
            AsyncWebServerResponse *response = request->beginResponse(
                200,
                webFiles[i].mime_type,
                webFiles[i].data,
                webFiles[i].size
            );
            response->addHeader("Content-Encoding", "gzip");
            response->addHeader("Cache-Control", "max-age=86400");
            request->send(response);
            return;
        }
    }

    // File not found - serve index.html for captive portal
    // Search for index.html in webFiles
    for (size_t i = 0; i < webFilesCount; i++) {
        if (strcmp(webFiles[i].path, "/index.html") == 0) {
            AsyncWebServerResponse *response = request->beginResponse(
                200,
                webFiles[i].mime_type,
                webFiles[i].data,
                webFiles[i].size
            );
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
            return;
        }
    }

    // If even index.html is not found, send error
    sendError(request, "File not found", 404);
}

void WebService::handleGetStatus(AsyncWebServerRequest *request) {
    JsonDocument doc;

    doc["success"] = true;

    JsonObject data = doc["data"].to<JsonObject>();
    data["isOnline"] = true;
    data["isFeeding"] = feedingService.isFeeding();

    // Servo position
    const char* position = "Closed";
    if (feedingService.isFeeding()) {
        position = "Feeding";
    }
    data["servoPosition"] = position;

    // Last feed time (placeholder - would need to track this)
    uint32_t lastFeedTs = feedingService.getLastFeedTimestamp();
    if (lastFeedTs > 0) {
        DateTime dt(lastFeedTs);
        char buf[25];
        snprintf(buf, sizeof(buf), "%04u-%02u-%02uT%02u:%02u:%02uZ",
                 dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
        data["lastFeedTime"] = buf;
    } else {
        data["lastFeedTime"] = nullptr;
    }

    // Total fed today (placeholder - would need to track this)
    data["totalFedToday"] = 0;

    sendJsonResponse(request, doc);
}

void WebService::handleGetFeedHistory(AsyncWebServerRequest *request) {
    JsonDocument doc;

    doc["success"] = true;

    // Get limit parameter from query string (default 10)
    int limit = 10;
    if (request->hasParam("limit")) {
        limit = request->getParam("limit")->value().toInt();
        if (limit < 1) limit = 1;
        if (limit > 100) limit = 100;
    }

    JsonObject data = doc["data"].to<JsonObject>();
    JsonArray feeds = data["feeds"].to<JsonArray>();

    // Get feed history from FeedingService
    uint8_t count = feedingService.getFeedHistoryCount();
    const FeedHistoryEntry* history = feedingService.getFeedHistory();

    Serial.printf("[WEB] Feed history request: count=%d, limit=%d\n", count, limit);

    // Get portion unit grams for calculating total grams
    uint8_t portionGrams = configService.getPortionUnitGrams();

    // Limit the number of entries returned
    if (count > limit) {
        count = limit;
    }

    // Add entries to JSON array (newest first)
    // Ring buffer: entries are added at feedHistoryIndex, wrapping around
    // We need to read them in reverse chronological order
    for (uint8_t i = 0; i < count; i++) {
        const FeedHistoryEntry& entry = history[i];

        Serial.printf("[WEB] Entry %d: timestamp=%lu, portion_units=%d\n", i, entry.timestamp, entry.portion_units);

        if (entry.timestamp == 0) {
            Serial.printf("[WEB] Skipping empty entry at index %d\n", i);
            continue; // Skip empty entries
        }

        JsonObject feed = feeds.add<JsonObject>();

        // Format timestamp as ISO 8601
        DateTime dt(entry.timestamp);
        char buf[25];
        snprintf(buf, sizeof(buf), "%04u-%02u-%02uT%02u:%02u:%02uZ",
                 dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
        feed["timestamp"] = buf;

        // Calculate portion in grams
        uint16_t portionTotal = entry.portion_units * portionGrams;
        feed["portion"] = portionTotal;

        Serial.printf("[WEB] Added feed to response: %s, %dg\n", buf, portionTotal);
    }

    Serial.printf("[WEB] Sending %d feed entries\n", feeds.size());

    sendJsonResponse(request, doc);
}

void WebService::handleGetConfig(AsyncWebServerRequest *request) {
    JsonDocument doc;

    doc["success"] = true;

    JsonObject data = doc["data"].to<JsonObject>();
    data["version"] = 1;
    data["portion_unit_grams"] = configService.getPortionUnitGrams();

    JsonArray schedules = data["schedules"].to<JsonArray>();

    Schedule schedule;
    for (uint8_t i = 0; i < MAX_SCHEDULES; i++) {
        if (configService.loadSchedule(i, schedule)) {
            JsonObject s = schedules.add<JsonObject>();
            s["id"] = schedule.id;
            s["enabled"] = schedule.enabled;
            s["time"] = schedule.time;
            s["weekday_mask"] = schedule.weekday_mask;
            s["portion_units"] = schedule.portion_units;
        }
    }

    sendJsonResponse(request, doc);
}

void WebService::handlePostConfig(AsyncWebServerRequest *request, uint8_t *data, size_t len) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        sendError(request, "Invalid JSON", 400);
        return;
    }

    // Save schedules if provided
    if (!doc["schedules"].isNull()) {
        JsonArray schedules = doc["schedules"];

        for (uint8_t i = 0; i < schedules.size() && i < MAX_SCHEDULES; i++) {
            JsonObject s = schedules[i];

            Schedule schedule;
            schedule.id = s["id"] | (i + 1);
            schedule.enabled = s["enabled"] | false;
            strncpy(schedule.time, s["time"] | "00:00", 6);
            schedule.weekday_mask = s["weekday_mask"] | 0;
            schedule.portion_units = s["portion_units"] | 1;

            // Validate portion_units
            if (schedule.portion_units < 1 || schedule.portion_units > 5) {
                sendError(request, "Invalid portion size. Must be between 1-5 units (12-60g).", 400);
                return;
            }

            configService.saveSchedule(i, schedule);
        }
    }

    // Update portion unit grams if provided
    if (!doc["portion_unit_grams"].isNull()) {
        uint8_t grams = doc["portion_unit_grams"];
        configService.setPortionUnitGrams(grams);
    }

    JsonDocument response;
    response["success"] = true;
    response["message"] = "Configuration saved successfully";

    // Notify scheduling service of config change
    schedulingService.onConfigChanged();

    sendJsonResponse(request, response);
}

void WebService::handlePostFeed(AsyncWebServerRequest *request) {
    if (feedingService.isFeeding()) {
        sendError(request, "Feeder is already active", 400);
        return;
    }

    // Trigger manual feed with 1 portion
    feedingService.feed(1);

    JsonDocument doc;
    doc["success"] = true;
    doc["message"] = "Feed cycle started";

    sendJsonResponse(request, doc);
}

void WebService::handleSleep(AsyncWebServerRequest *request) {
    if (feedingService.isFeeding()) {
        sendError(request, "Cannot sleep while feeding", 400);
        return;
    }
    if (!sleepCallback) {
        sendError(request, "Sleep callback not set", 500);
        return;
    }

    JsonDocument doc;
    doc["success"] = true;
    doc["message"] = "Sleep requested";
    sendJsonResponse(request, doc);

    // Defer actual sleep to allow response to flush
    sleepRequested = true;
    sleepRequestMillis = millis();
}

void WebService::handlePostTime(AsyncWebServerRequest *request, uint8_t *data, size_t len) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        sendError(request, "Invalid JSON", 400);
        return;
    }

    if (doc["unixTime"].isNull()) {
        sendError(request, "Missing unixTime field", 400);
        return;
    }

    uint32_t unixTime = doc["unixTime"];

    if (!clockService.setTime(unixTime)) {
        sendError(request, "Failed to set time", 500);
        return;
    }

    JsonDocument response;
    response["success"] = true;
    response["message"] = "Time synchronized successfully";

    sendJsonResponse(request, response);
}

void WebService::handleResetConfig(AsyncWebServerRequest *request) {
    if (!configService.resetToDefaults()) {
        sendError(request, "Failed to reset configuration", 500);
        return;
    }

    JsonDocument doc;
    doc["success"] = true;
    doc["message"] = "Configuration reset to defaults";

    sendJsonResponse(request, doc);
}

void WebService::sendJsonResponse(AsyncWebServerRequest *request, JsonDocument &doc, int statusCode) {
    String output;
    serializeJson(doc, output);

    AsyncWebServerResponse *response = request->beginResponse(statusCode, "application/json", output);
    response->addHeader("Access-Control-Allow-Origin", "*");
    request->send(response);
}

void WebService::sendError(AsyncWebServerRequest *request, const char* message, int statusCode) {
    JsonDocument doc;
    doc["success"] = false;
    doc["error"] = message;

    sendJsonResponse(request, doc, statusCode);
}

void WebService::updateClientActivity() {
    lastClientActivity = millis();
}

void WebService::handleOtaStatus(AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["success"] = true;
    doc["maintenanceMode"] = maintenanceMode;

    sendJsonResponse(request, doc);
}

void WebService::handleOtaUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
    // Start update on first chunk
    if (index == 0) {
        Serial.printf("[OTA] Update Start: %s\n", filename.c_str());

        // Begin OTA update
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
            if (final) {
                JsonDocument doc;
                doc["success"] = false;
                doc["error"] = "OTA update failed to begin";
                sendJsonResponse(request, doc, 500);
            }
            return;
        }
    }

    // Write chunk
    if (len) {
        if (Update.write(data, len) != len) {
            if (final) {
                JsonDocument doc;
                doc["success"] = false;
                doc["error"] = "OTA write failed";
                sendJsonResponse(request, doc, 500);
            }
            return;
        }
    }

    // Finalize update on last chunk
    if (final) {
        if (Update.end(true)) {
            Serial.printf("[OTA] Update Success: %u bytes\n", index + len);

            JsonDocument doc;
            doc["success"] = true;
            doc["message"] = "Firmware updated successfully. Rebooting...";
            sendJsonResponse(request, doc);

            // Reboot after response is sent
            delay(100);
            ESP.restart();
        } else {
            Update.printError(Serial);

            JsonDocument doc;
            doc["success"] = false;
            doc["error"] = "OTA update failed to complete";
            sendJsonResponse(request, doc, 500);
        }
    }
}
