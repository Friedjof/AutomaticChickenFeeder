# ConfigManager for Platformio and the ESP32
## Description
This is a simple config manager for the ESP32. It can manage Configuration based on a JSON file.

## Usage
```cpp
#include <Arduino.h>
#include <ConfigManager.h>

#define CONFIG_FILE "/config.json"

ConfigManager configManager;

void setup() {
  Serial.begin(115200);
  configManager = ConfigManager(CONFIG_FILE);

  // Load the config
  configManager.load();

  // Get the config
  Serial.println(configManager.get("ssid"));
  Serial.println(configManager.get("password"));
}

void loop() {
  // put your main code here, to run repeatedly:
}
```