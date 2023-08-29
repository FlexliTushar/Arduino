#include<vector>
#include<string>
#include<WiFi.h>
#include<WiFiUdp.h>
#include<elapsedMillis.h>
#include<EEPROM.h>
#include<HTTPClient.h>
using namespace std;

int udp_request_id = 0;
const int UDP_PORT = 9999;
char *UDP_IP = "192.168.2.109";
WiFiUDP udp;
elapsedMillis pkt_response_time;
String BOT_ID = "";
int BOT_FORMAT = 0;
int occupy_index = 0;
int path_index = 0;
const vector<string> PLANNED_PATH{"I1", "D1", "D2", "D3", "D4", "D5", "D6", "S14", "S15", "S17", "C4", "S21", "S22", "S23", "S24", 
                                  "S25", "S26", "S27", "C1", "D7", "D8", "D9", "D10", "D11", "D12", "I11", "I12", "I13", "I1"};

const vector<string> REDIRECTED_PATH{"S14", "D37", "D38", "D39", "D40", "D41", "D42", "C15", "D43", "D44", "D45", "D46", "D47", 
                                    "D48", "S17", "C4", "S21", "S22", "S23", "S24", "S25", "S26", "S27", "C1", "D7", "D8", "D9", 
                                    "D10", "D11", "D12", "I11", "I12", "I13", "I1" };

const vector<string> DYNAMIC_INFEED_PATH{"D5", "D6", "S14", "D37", "D38", "D39", "D40", "D41", "D42", "I31", "I32", "I33", "I3"};

vector<string> planned_path;
string currentStation = "I1";
string expectedStation = "I1";
string assignedStation = "";
string reservedStation = "";
string currentInfeed = "I1";
vector<string> infeed_list{"I1", "I2", "I3", "I4"};
String debug_logging_string = "";

const char *SSID = "TP-Link_35E3";
const char *PASSWORD = "msort@flexli";
const String HTTP_DEBUG_SERVER_URL = "http://192.168.2.109:8080/data";

vector<string> dropOff(4, "X");
TaskHandle_t http_debug_log;
HTTPClient http_debugger;

void add_log(String log) {
  debug_logging_string = debug_logging_string + " " + log;
  debug_logging_string += " | ";
  Serial.println(log);
}

// Core 2 finction which is sending http post request to debugger with debug logs as payload
void HTTP_DEBUG_LOGGER(void *pvParameters) {
  while (true) 
  {
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    http_debugger.begin(HTTP_DEBUG_SERVER_URL);
    int httpCode = -1;
    httpCode = http_debugger.POST(debug_logging_string);
    if (httpCode == HTTP_CODE_OK) 
    {
      debug_logging_string = BOT_ID + " ";
    }
  }
}

vector<string> split_string(string &path, char delimiter) {
  string temp = "";
  vector<string> ans;
  for (int i = 0; i < path.length(); i++) 
  {
    if(path[i] != delimiter) 
    {
      temp += path[i];
    }
    else 
    {
      ans.push_back(temp);
      temp = "";
    }
  }
  ans.push_back(temp);
  return ans;
}

void InitialisePlannedPath()
{
  planned_path.clear();
  for(int i = 0 ; i < PLANNED_PATH.size() ; i++)
  {
    planned_path.push_back(PLANNED_PATH[i]);
  }
  path_index = 0;
}

void setup() {  
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
    add_log("EEPROM working fine!");
    BOT_ID = EEPROM.readString(20);
    BOT_FORMAT = EEPROM.readInt(0);
  }
  add_log("BOT ID = " + BOT_ID + " BOT FORMAT = " + String(BOT_FORMAT) + " Firmware: " + String(__FILE__));
  add_log("Connecting... " + String(SSID) + " " + String(PASSWORD));
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) 
  {
    add_log(".");
    delay(500);
  }

  add_log(" Wifi Connected! : ");
  // creating a core 2 task to handle debugger http request
  xTaskCreatePinnedToCore(HTTP_DEBUG_LOGGER, "debug_logging", 10000, NULL, 1, &http_debug_log, 1);
}

void loop() {
  if(true) 
  {
    // At infeed
    if(find(infeed_list.begin(), infeed_list.end(), currentStation) != infeed_list.end())
    {
      InitialisePlannedPath();
      currentStation = planned_path[0];
      expectedStation = planned_path[1];
      assignedStation = planned_path[2];
      reservedStation = planned_path[3];
    }

    bool udp_response_flag = false;
    while(!udp_response_flag) 
    {
      string udp_response_string = UDP_req_resp();
      vector<string> udp_response_string_arr = split_string(udp_response_string, '_'); // Convert response string into an array of different components
      if(udp_response_string_arr.size() <= 3)
      {
        add_log("No station recieved! Wait 2 seconds.");
        delay(2000);
      }
      else
      {
        // expectedStation in the response string
        vector<string> station_string_arr = split_string(udp_response_string_arr[4], '-');
        if(udp_response_string_arr[2] == "N")
        {
          if(find(station_string_arr.begin(), station_string_arr.end(), expectedStation) != station_string_arr.end())
          {
            add_log("Allowed to go forward!");
            planned_path.erase(planned_path.begin());
            currentStation = planned_path[0];
            expectedStation = planned_path[1];
            assignedStation = "X";
            reservedStation = "X";
            if(planned_path.size() > 3)
            {
              assignedStation = planned_path[2];
              reservedStation = planned_path[3];
            }
            else if(planned_path.size() > 2)
            {
              assignedStation = planned_path[2];
            }
            udp_response_flag = true;
            delay(1000);
          }
          else
          {
            add_log("Not allowed to go forward - STOP!");
            delay(1000);
          }
        }
        else if(udp_response_string_arr[2] == "R")
        {
          change_planned_path("R");
          add_log("Redirected!");
          currentStation = planned_path[0];
          expectedStation = planned_path[1];
          assignedStation = "X";
          reservedStation = "X";
          if(planned_path.size() > 3)
          {
            assignedStation = planned_path[2];
            reservedStation = planned_path[3];
          }
          else if(planned_path.size() > 2)
          {
            assignedStation = planned_path[2];
          }
          udp_response_flag = true;
          delay(1000);
        }
        else if(udp_response_string_arr[2] == "D")
        {
          change_planned_path("D");
          add_log("Dynamic Infeed Successful!");
          currentStation = planned_path[0];
          expectedStation = planned_path[1];
          assignedStation = "X";
          reservedStation = "X";
          if(planned_path.size() > 3)
          {
            assignedStation = planned_path[2];
            reservedStation = planned_path[3];
          }
          else if(planned_path.size() > 2)
          {
            assignedStation = planned_path[2];
          }
          udp_response_flag = true;
          delay(1000);
        }
      }
    }
  }
}

string UDP_req_resp() {
  while (true) 
  {  
    // decision for Rule value
    String rule = "N";
    if(find(dropOff.begin(), dropOff.end(), assignedStation) != dropOff.end())
    {
      rule = "D";
    }
    add_log("UDP Request: ");
    // Start preparing the request | Format = Bot_Id_Response_Id_OccupiedStationId_Node_Id_AllocatedStationId_AssignedStationId_ReservedStationId_MotorStatus_Rule_InfeedStationId
    String sending_msg = BOT_ID + "_" + String(++udp_request_id) + "_" + String(currentStation.c_str()) + "_" + "0" + "_" + 
                        String(expectedStation.c_str()) + "_" + String(assignedStation.c_str()) + "_" + String(reservedStation.c_str())
                         + "_" + "0" + "_" + rule + "_" + String(currentInfeed.c_str());
    add_log(sending_msg);
    udp.beginPacket(UDP_IP, UDP_PORT);
    udp.print(sending_msg);
    udp.endPacket();

    pkt_response_time = 0;
    // Get response within 200 ms or else request will be sent again
    while (pkt_response_time < 200) 
    {
      char incomingPacket[255];
      int packetSize = udp.parsePacket();
      if (packetSize) 
      {
        int len = udp.read(incomingPacket, 255);
        incomingPacket[len] = 0;
        // Getting response in string format
        string incomingPacketStr = incomingPacket;
        add_log("T: " + String(pkt_response_time) + " " + String(incomingPacketStr.c_str()));
        int index = 0;       // gives index of 1st underscore
        for(;index<incomingPacketStr.size();index++)
        {
          if(incomingPacketStr[index] == '_')
          {
            break;
          }
        }
        string response_id_str = incomingPacketStr.substr(0, index);
        string request_id_str = to_string(udp_request_id);

        String botId = "";
        index++;
        while(incomingPacketStr[index] != '_')
        {
          botId += incomingPacketStr[index];
          index++;
        }

        if (response_id_str == request_id_str && botId == BOT_ID) 
        {
          return incomingPacketStr;
        }
      }
    }
  }
}

void change_planned_path(string rule)
{
  if(rule == "R")
  {
    planned_path.clear();

    for(int i = 1 ; i < REDIRECTED_PATH.size() ; i++)
    {
      planned_path.push_back(REDIRECTED_PATH[i]);
    }
  }
  if(rule == "D")
  {
    planned_path.clear();

    for(int i = 1 ; i < DYNAMIC_INFEED_PATH.size() ; i++)
    {
      planned_path.push_back(DYNAMIC_INFEED_PATH[i]);
    }
  }
}
