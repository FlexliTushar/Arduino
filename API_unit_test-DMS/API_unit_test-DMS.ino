#include<EEPROM.h>
#include<WiFi.h>
#include<WiFiUDP.h>
#include<HTTPClient.h>

const char *SSID = "TP-Link_35E3";
const char *PASSWORD = "msort@flexli";
const String HTTP_DEBUG_SERVER_URL = "http://192.168.2.109:8080/data";
String BOT_ID = "";
String debug_logging_string = "";
int BOT_FORMAT = 0;
String TOKEN = "";
int THRESHOLD = 15;

TaskHandle_t http_debug_log;
HTTPClient http_debugger;
HTTPClient http;

void add_log(String log) {
  if (true) 
  {
    debug_logging_string = debug_logging_string + " " + log;
    debug_logging_string += " | ";
  } 
  else 
  {
    Serial.println(log);
  }
}

// Core 2 finction which is sending http post request to debugger with debug logs as payload
void HTTP_DEBUG_LOGGER(void *pvParameters) {
  while (true) {
    // if (publish_debug_log_flag) {
    // publish_debug_log_flag = false;
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    http_debugger.begin(HTTP_DEBUG_SERVER_URL);
    int httpCode = -1;
    httpCode = http_debugger.POST(debug_logging_string);
    // add_log(String(httpCode));

    if (httpCode == HTTP_CODE_OK) {
      debug_logging_string = BOT_ID + " ";
      //String payload = http_debugger.getString();
    }
  }
}

void call_login_API() {
  int loginhttpCode = 0;
  int count = 0; 
  while (loginhttpCode != HTTP_CODE_OK && count < THRESHOLD) 
  {
    String login_url = "https://192.168.2.109:8443/m-sort-distribution-server/Login";
    http.begin(login_url);
    String loginPw = BOT_ID + "@flexliblr6";
    http.addHeader("userName", BOT_ID);
    http.addHeader("password", loginPw);
    add_log("UserName: " + BOT_ID);
    add_log("Password: " + loginPw);
    loginhttpCode = http.GET();
    if (loginhttpCode == HTTP_CODE_OK) 
    {
      TOKEN = http.getString();
      add_log("BOT token: " + TOKEN);
    } 
    else 
    {
      add_log(String(loginhttpCode) + " : login failed ");
      count++;
      delay(1000);
    }
  }
}

void call_flush_API() {
  // flushing all containers
  String flushurl = "https://192.168.2.109:8443/m-sort-distribution-server/FlushTheBot?botId=" + BOT_ID;  // confirm if the api takes 0-3 or 1-4 as compartmen id
  add_log(flushurl);
  bool flush_flag = false;
  int count = 0;
  while (!flush_flag && count < THRESHOLD) 
  {
    http.begin(flushurl);
    http.addHeader("authenticationToken", TOKEN);
    int flushhttpCode = http.PUT(flushurl);
    add_log("Flush API Response : " + String(flushhttpCode));
    if (flushhttpCode == HTTP_CODE_OK) 
    {
      flush_flag = true;
    } 
    else 
    {
      count++;
      delay(2000);
    }
  }
  add_log("Tries for Flush API: " + String(count));
}

void call_update_parcel_dropped_API(String sections) {
  // flushing all containers
  String updateurl = "https://192.168.2.109:8443/m-sort-distribution-server/UpdateParcelDroppedForBot?botId=" + BOT_ID + "&sectionIds=" + sections; // confirm if the api takes 0-3 or 1-4 as compartmen id
  add_log(updateurl);
  bool update_flag = false;
  int count = 0;
  while (!update_flag && count < THRESHOLD) 
  {
    http.begin(updateurl);
    http.addHeader("authenticationToken", TOKEN);
    int updatehttpCode = http.PUT(updateurl);
    add_log("Update Parcel Dropped API response : " + String(updatehttpCode));
    if (updatehttpCode == HTTP_CODE_OK) 
    {
      update_flag = true;
    } 
    else 
    {
      count++;
      delay(2000);
    }
  }
  add_log("Tries for Update Parcel Dropped API: " + String(count));
}

void call_localize_API() {
  // flushing all containers
  String localizeurl = "https://192.168.2.109/m-sort-distribution-server/GetPlannedPath?currentStationId=D7"; // confirm if the api takes 0-3 or 1-4 as compartmen id
  add_log(localizeurl);
  bool localize_flag = false;
  int count = 0;
  while (!localize_flag && count < THRESHOLD) 
  {
    http.begin(localizeurl);
    http.addHeader("authenticationToken", TOKEN);
    int localizehttpCode = http.GET();
    add_log("Localize API Response : " + String(localizehttpCode));
    if (localizehttpCode == HTTP_CODE_OK) 
    {
      localize_flag = true;
      String payload = http.getString();
      add_log("Localized: " + payload);
    } 
    else 
    {
      count++;
      delay(2000);
    }
  }
  add_log("Tries for Localize API: " + String(count));
}

void call_destination_API() {
  for(int i = 0 ; i < BOT_FORMAT ; i++)
  {
    String infeedurl = "https://192.168.2.109:8443/m-sort-distribution-server/GetDestinationStationIdForBot?botId=" + BOT_ID + "&infeedStationId=I1&sectionId=S" + String(i + 1);; // confirm if the api takes 0-3 or 1-4 as compartmen id
    add_log(infeedurl);
    bool infeed_flag = false;
    int count = 0;
    while (!infeed_flag && count < THRESHOLD) 
    {
      http.begin(infeedurl);
      http.addHeader("authenticationToken", TOKEN);
      int infeedhttpCode = http.GET();
      add_log("Get Destination API Response : " + String(infeedhttpCode));
      if (infeedhttpCode == HTTP_CODE_OK) 
      {
        infeed_flag = true;
        String payload = http.getString();
        add_log("Path Recieved: " + payload);
      } 
      else 
      {
        count++;
        delay(2000);
      }
    }
    add_log("Tries for Get Destination API: " + String(count));
  }
  
}

void call_redirection_API() {
  // flushing all containers
  String redirectionURL = "https://192.168.2.109:8443/m-sort-distribution-server/GetPlannedPathForRedirection?currentStationId=D11&droppingStationIds=-D7-D3"; // confirm if the api takes 0-3 or 1-4 as compartmen id
  add_log(redirectionURL);
  bool redirection_flag = false;
  int count = 0;
  while (!redirection_flag && count < THRESHOLD) 
  {
    http.begin(redirectionURL);
    http.addHeader("authenticationToken", TOKEN);
    int redirectionhttpCode = http.GET();
    add_log("Redirection API Response : " + String(redirectionhttpCode));
    if (redirectionhttpCode == HTTP_CODE_OK) 
    {
      redirection_flag = true;
      String payload = http.getString();
      add_log("Redirected: " + payload);
    } 
    else 
    {
      count++;
      delay(2000);
    }
  }
  add_log("Tries for Redirection API: " + String(count));
}

void call_check_station_API() {
  // flushing all containers
  String check_motherbag_url = "https://192.168.2.109/m-sort-distribution-server/CheckIfStationIsAvailableAsDropOff?stationId="; // confirm if the api takes 0-3 or 1-4 as compartmen id
  add_log(check_motherbag_url);
  bool check_motherbag__flag = false;
  int count = 0;
  while (!check_motherbag__flag && count < THRESHOLD) 
  {
    http.begin(check_motherbag_url);
    http.addHeader("authenticationToken", TOKEN);
    int check_motherbag_httpCode = http.GET();
    add_log("CHeck Station API Response : " + String(check_motherbag_httpCode));
    if (check_motherbag_httpCode == HTTP_CODE_OK) 
    {
      check_motherbag__flag = true;
      String payload = http.getString();
      add_log("Available for dropping: " + payload);
    } 
    else 
    {
      count++;
      delay(2000);
    }
  }
  add_log("Tries for Check Station API: " + String(count));
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial);
  // initalizing EEPROM and reseting ESP if it fails
  if (!EEPROM.begin(1000)) 
  {
    add_log("Failed to initialise EEPROM");
    add_log("ESP32 Restarting...");
    delay(1000);
    ESP.restart();
  } 
  else 
  {
    add_log("EEPROM working fine");
    BOT_ID = EEPROM.readString(20);
    BOT_FORMAT = EEPROM.readInt(0);
  }
  add_log("BOT ID = " + BOT_ID + " BOT FORMAT= " + BOT_FORMAT + " Firmware: " + String(__FILE__));

  add_log("Connecting... " + String(SSID) + " " + String(PASSWORD));
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) 
  {
    add_log(".");
    delay(500);
  }
  //ack_led.actuate(Relay::_ON);
  add_log(" Wifi Connected! : ");
  // creating a core 2 task to handle debugger http request
  xTaskCreatePinnedToCore(HTTP_DEBUG_LOGGER, "debug_logging", 10000, NULL, 1, &http_debug_log, 1);
  call_login_API();
}

void loop() {
  // put your main code here, to run repeatedly:
  call_flush_API();
  call_localize_API();
  call_destination_API();
  call_redirection_API();
  call_check_station_API();
  String sections = "";
  for(int i = 0 ; i < BOT_FORMAT ; i++)
  {
    sections += "-S" + String(i + 1);
  }
  call_update_parcel_dropped_API(sections);
}
