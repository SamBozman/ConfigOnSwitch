

#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_wifi.h>

#define ESP_getChipId() ((uint32_t)ESP.getEfuseMac())

// SSID and PW for Config Portal
String ssid = "MM_" + String(ESP_getChipId(), HEX);
const char *password = "mmesp32";

// SSID and PW for your Router
String Router_SSID;
String Router_Pass;

#define USE_AVAILABLE_PAGES false

#include <ESP_WiFiManager.h> //https://github.com/khoih-prog/ESP_WiFiManager

#define AP_REQUEST 25
#define RUN_SENSOR 13
#define PIN_LED 2

// Indicates whether ESP has WiFi credentials saved from previous session
bool initialConfig = false;

void heartBeatPrint(void) {
  static int num = 1;

  if (WiFi.status() == WL_CONNECTED)
    Serial.print("H"); // H means connected to WiFi
  else
    Serial.print("F"); // F means not connected to WiFi

  if (num == 80) {
    Serial.println();
    num = 1;
  } else if (num++ % 10 == 0) {
    Serial.print(" ");
  }
}

void check_status() {
  static ulong checkstatus_timeout = 0;

#define HEARTBEAT_INTERVAL 10000L
  // Print hearbeat every HEARTBEAT_INTERVAL (10) seconds.
  if ((millis() > checkstatus_timeout) || (checkstatus_timeout == 0)) {
    heartBeatPrint();
    checkstatus_timeout = millis() + HEARTBEAT_INTERVAL;
  }
}

void setup() {
  // put your setup code here, to run once:
  // initialize the LED digital pin as an output.
  pinMode(PIN_LED, OUTPUT);
  pinMode(AP_REQUEST, INPUT_PULLUP);
  pinMode(RUN_SENSOR, INPUT_PULLUP);

  Serial.begin(115200);
  Serial.println("\nStarting");

  unsigned long startedAt = millis();

  // Local intialization. Once its business is done, there is no need to keep it
  // around
  // Use this to default DHCP hostname to ESP8266-XXXXXX or ESP32-XXXXXX
  // ESP_WiFiManager ESP_wifiManager;
  // Use this to personalize DHCP hostname (RFC952 conformed)
  ESP_WiFiManager ESP_wifiManager("ConfigOnSwitch");

  ESP_wifiManager.setMinimumSignalQuality(-1);
  // Set static IP, Gateway, Subnetmask, DNS1 and DNS2. New in v1.0.5
  ESP_wifiManager.setSTAStaticIPConfig(
      IPAddress(192, 168, 2, 114), IPAddress(192, 168, 2, 1),
      IPAddress(255, 255, 255, 0), IPAddress(192, 168, 2, 1),
      IPAddress(8, 8, 8, 8));

  // We can't use WiFi.SSID() in ESP32as it's only valid after connected.
  // SSID and Password stored in ESP32 wifi_ap_record_t and wifi_config_t are
  // also cleared in reboot Have to create a new function to store in
  // EEPROM/SPIFFS for this purpose
  Router_SSID = ESP_wifiManager.WiFi_SSID();
  Router_Pass = ESP_wifiManager.WiFi_Pass();

  // Remove this line if you do not want to see WiFi password printed
  Serial.println("Stored: SSID = " + Router_SSID + ", Pass = " + Router_Pass);

  // SSID to uppercase
  ssid.toUpperCase();

  if (Router_SSID == "") {
    Serial.println(
        "We haven't got any access point credentials, so get them now");

    digitalWrite(PIN_LED,
                 HIGH); // Turn led on as we are in configuration mode.

    // it starts an access point
    // and goes into a blocking loop awaiting configuration
    if (!ESP_wifiManager.startConfigPortal((const char *)ssid.c_str(),
                                           password))
      Serial.println("Not connected to WiFi but continuing anyway.");
    else
      Serial.println("WiFi connected...yeey :)");
  }

  digitalWrite(PIN_LED,
               LOW); // Turn led off as we are not in configuration mode.

#define WIFI_CONNECT_TIMEOUT 30000L
#define WHILE_LOOP_DELAY 200L
#define WHILE_LOOP_STEPS (WIFI_CONNECT_TIMEOUT / (3 * WHILE_LOOP_DELAY))

  startedAt = millis();

  while ((WiFi.status() != WL_CONNECTED) &&
         (millis() - startedAt < WIFI_CONNECT_TIMEOUT)) {
    WiFi.mode(WIFI_STA);
    WiFi.persistent(true);
    // We start by connecting to a WiFi network

    Serial.print("Connecting to ");
    Serial.println(Router_SSID);

    WiFi.begin(Router_SSID.c_str(), Router_Pass.c_str());

    int i = 0;
    while ((!WiFi.status() || WiFi.status() >= WL_DISCONNECTED) &&
           i++ < WHILE_LOOP_STEPS) {
      delay(WHILE_LOOP_DELAY);
    }
  }

  Serial.print("After waiting ");
  Serial.print((millis() - startedAt) / 1000);
  Serial.print(" secs more in setup(), connection result is ");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("connected. Local IP: ");
    Serial.println(WiFi.localIP());
  } else
    Serial.println(ESP_wifiManager.getStatus(WiFi.status()));
}

void loop() {
  // is configuration portal requested?
  if (digitalRead(AP_REQUEST) == LOW) {
    Serial.println("\nConfiguration portal requested.");
    digitalWrite(PIN_LED, HIGH); // turn the LED on by making the voltage LOW
                                 // to tell us we are in configuration mode.

    // Local intialization. Once its business is done, there is no need to keep
    // it around
    ESP_WiFiManager ESP_wifiManager;

    // Check if there is stored WiFi router/password credentials.
    // If not found, device will remain in configuration mode until switched off
    // via webserver.
    Serial.print("Opening configuration portal. ");
    Router_SSID = ESP_wifiManager.WiFi_SSID();
    if (Router_SSID != "") {
      ESP_wifiManager.setConfigPortalTimeout(
          60); // If no access point name has been previously entered disable
               // timeout.
      Serial.println("Got stored Credentials. Timeout 60s");
    } else
      Serial.println("No stored Credentials. No timeout");

    // it starts an access point
    // and goes into a blocking loop awaiting configuration
    if (!ESP_wifiManager.startConfigPortal((const char *)ssid.c_str(),
                                           password)) {
      Serial.println("Not connected to WiFi but continuing anyway.");
    } else {
      // if you get here you have connected to the WiFi
      Serial.println("connected...yeey :)");
    }

    digitalWrite(PIN_LED,
                 LOW); // Turn led off as we are not in configuration mode.
  }

  // put your main code here, to run repeatedly
  check_status();
}
