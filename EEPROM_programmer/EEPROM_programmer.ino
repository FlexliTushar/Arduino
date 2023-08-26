#include <EEPROM.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <OTA_firmware_updater.h>
#include <Pin_Config_Constants.h>
String ESP32_BOT_ID = "B101";
int BOT_FORMAT= 2;
HTTPClient http;
bool debugger_flag = true;
static String debug_logging_string = "";
TaskHandle_t http_debug_log;

OTA_firmware_updater ota(HTTP_OTA_SERVER_URL);

void add_log(String log) {
  if (debugger_flag == true) {
    debug_logging_string = debug_logging_string + " " + log;
    debug_logging_string += " | ";
  } else {
    Serial.println(log);
  }
}

void HTTP_DEBUG_LOGGER(void *pvParameters) {
  while (true) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    http.begin(HTTP_DEBUG_SERVER_URL);

    int httpCode = http.POST(debug_logging_string);
    add_log(String(httpCode));
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
        debug_logging_string = "";
        String payload = http.getString();
      }
    } else {
      add_log(http.errorToString(httpCode).c_str());
    }
  }
}


void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  // add_log("Attempting to connect to SSID : " + String(SSID) + " " + String(PASSWORD));
  // WiFi.begin(SSID, PASSWORD);

  // while (WiFi.status() != WL_CONNECTED) {
  //   add_log(".");
  //   delay(500);
  // }
  // add_log("  Connected! : " + String(SSID) + " " + String(PASSWORD));
  // xTaskCreatePinnedToCore(
  //   HTTP_DEBUG_LOGGER,
  //   "debug_logging",
  //   10000,
  //   NULL,
  //   1,
  //   &http_debug_log,
  //   1);

  if (!EEPROM.begin(1000)) {
    add_log("Failed to initialise EEPROM");
    add_log("ESP32 Restarting...");
    delay(1000);
    ESP.restart();
  } else {
    add_log("EEPROM working fine");
  }
  EEPROM.writeInt(0, BOT_FORMAT);
  EEPROM.writeInt(4, 192);
  EEPROM.writeInt(4, 168);
  EEPROM.writeInt(12, 2);
  EEPROM.writeInt(16, 251);
  EEPROM.writeString(20, ESP32_BOT_ID);

  EEPROM.commit();
  delay(1000);
  Serial.println("BOT ID = " + String(EEPROM.readString(20)) + " count = " + String(EEPROM.readInt(0)));

  // add_log("BOT ID updated, please update the firmware file on server in 10 sec");
  // for (int i = 10; i >= 10; i--) {
  //   add_log(String(i));
  //   delay(1000);
  // }
  // add_log("OTA updating");
  // ota.update_firmware();
}
void loop() {
}