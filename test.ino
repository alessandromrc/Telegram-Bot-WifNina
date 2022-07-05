
#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoJson.h>
#include "debouncing.h"
#include "tab/arduino_secrets.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int keyIndex = 0;

#define userID 699281065 // your user id here
#define botKey "" // your bot key here


int status = WL_IDLE_STATUS;
char server[] = "https://api.telegram.org";


WiFiSSLClient client;

Debouncing PSW_1(12, INPUT_PULLUP);

String urlEncode(String str) {
  String new_str = "";
  char c;
  int ic;
  const char* chars = str.c_str();
  char bufHex[10];
  int len = strlen(chars);

  for (int i = 0; i < len; i++) {
    c = chars[i];
    ic = c;
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
      new_str += c;
    else {
      sprintf(bufHex, "%X", c);
      if (ic < 16)
        new_str += "%0";
      else
        new_str += "%";
      new_str += bufHex;
    }
  }
  return new_str;
}

void setup() {
  // Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP
    // network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(1000);
  }
  Serial.println("Connected to WiFi");
  printWiFiStatus();
  sendMessage("RP 2040 is online!", String(userID));

}

void sendMessage(String message, String chatID) {
  if (client.connect(server, 443)) {
    client.println("POST /bot" + String(botKey) + "/sendMessage?chat_id=" +
                   chatID + "&text=" + urlEncode(message) + " HTTP/1.1");
    client.println("Host: api.telegram.org");
    client.println("Connection: keep-alive");
    client.println();
  }
}


void getLastMessage() {
  if (client.connect(server, 443)) {
    client.println("GET /bot" + String(botKey) + "/getUpdates?offset=1" + " HTTP/1.1");
    client.println("Host: api.telegram.org");
    client.println("Connection: keep-alive");
    client.println();
  }
}



void loop() {
  // if there are incoming bytes available
  // from the server, read them and print them:

  if (PSW_1.click()) {

    String incomingData;

    Serial.println("\nStarting connection to server...");
    getLastMessage();

    while (true) {
      char c = client.read();
      incomingData += c;

      if (incomingData.indexOf("}}]}") != -1) {

        auto count = incomingData.indexOf("{\"ok\":");

        incomingData.remove(0, count);

        DynamicJsonDocument doc(4096); // allocate 4MB to the json document

        deserializeJson(doc, incomingData);


        int messageCount = 0;
        for (int i = 0; i < doc["result"].size(); i++) {
          if (doc["result"][i]["message"]["text"] != "") {
            messageCount++;
          }
        }

        const char *message = doc["result"][messageCount - 1]["message"]["text"];
        const unsigned long UserID = doc["result"][messageCount - 1]["message"]["from"]["id"];
        const char *UserName = doc["result"][messageCount - 1]["message"]["from"]["username"];

        Serial.println(String(UserID) + "(" + String(UserName) +  "): " + String(message));

        //if the owner sends a message then echo it back

        if(UserID == userID)
        {
          Serial.println("Echoing back " + String(message));
          sendMessage(message, String(userID));
        }

        break;
      }
    }


  }

}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
