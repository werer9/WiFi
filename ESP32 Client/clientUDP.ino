/*
    Name: Caelan Murch
    Project: ESP32-Client
    Date: 19/12/2018

    Project description:
    Client that sends RSSI data from Sparkfun ESP32 Thing board
    to a server via AP

    Software description: 
    Measure RSSI of any AP that has an SSID that contains 'Measure'
    Store the RSSI values in an array and send them to a python server
    The server host must have an IP address of 192.168.0.118 port 8787 on the Wireless network

    (c) Caelan Murch

*/

#include <WiFi.h> // Include wifi header
#include <WiFiUdp.h> // UDP header
#include <cstring> // C string library

// Setup wifi information
const char *AP_SSID = "Measure7";
const char *AP_PASS = "pass1234";

const char *NET_SSID = "measure";
const char *NET_PASS = "measure200";
const char *NET_HOST = "192.168.0.118";
const int NET_PORT = 8787;

// Prototype functions
int getRSSI(String SSID[], int RSSI[]);
void sendRSSI(String SSID[], int RSSI[], int apcount);
void connectWiFi(char *ssid, char *pass);
void WiFiEvent(WiFiEvent_t event);
void reconnectWifi();

// buffer
char buffer[1024];

// RSSI and SSID arrays
String SSID[100];
int RSSI[100];

// UDP client
WiFiUDP udp;

// Setup function
void setup() 
{
    // Begin serial output
    Serial.begin(115200);

    // Set wifi mode
    WiFi.mode(WIFI_AP_STA);

    // Setup wifi event handler
    WiFi.onEvent(WiFiEvent);

    // Start softap
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.println("Started soft-ap... ");

    // connect to AP
    connectWiFi(NET_SSID, NET_PASS);

    // Init transfer buffer
    udp.begin(WiFi.localIP(), NET_PORT);
}

// Loop function
void loop() 
{
    // number of networks broadcasting that contain 'Measure' in SSID
    int count = getRSSI(SSID, RSSI);

    if (count == 0) {
        // Do nothing if no 'Measure' APs are found
    } else {
        // set transmit power
        WiFi.setTxPower(WIFI_POWER_19_5dBm);
        // begin sending UDP packet
        udp.beginPacket(NET_HOST, NET_PORT);
        // send message to server
        udp.printf("TX%d Hello from Measure7\r\n", (int)WiFi.getTxPower());
        sendRSSI(SSID, RSSI, count);
        // end UDP packet
        udp.endPacket();

        // check size of reply 
        int packetSize = udp.parsePacket();

        if (packetSize) {
            // Print reply to serial
            int len = udp.read(buffer, 1024);
            if (len > 0) {
                buffer[len] = 0;
            }
            Serial.printf("Message: %s\r\n", buffer);
        }
    }
}

// // Get RSSI of relevant APs 
int getRSSI(String SSID[], int RSSI[])
{
    // number of wifi networks
    
    // Set wifi mode
    WiFi.mode(WIFI_AP_STA);
    Serial.println("Scanning networks");
    // Get number of APs
    int n = WiFi.scanNetworks();
    Serial.println("Finished scanning");
    delay(1000);
    int count = 0;
    // Check if there are too many APs
    if (n <= 0 || n >= 60) {
        Serial.println("No networks found...");
    } else {
        for (int i = 0; i < n; i++) {
            // If SSID contains 'Measure', store them in arrays
            if (strstr(WiFi.SSID(i).c_str(), "Measure") != NULL) {
                SSID[count] = WiFi.SSID(i);
                RSSI[count] = WiFi.RSSI(i);
                count++;
            }
        }
    }

    // return number of APs that contain 'Measure' in SSID
    return count;
}


// Send RSSI to server
void sendRSSI(String SSID[], int RSSI[], int apcount)
{
    for (int i = 0; i < apcount; i++) {
        udp.printf("SSID: %s RSSI: %d\r\n", SSID[i].c_str(), RSSI[i]);
    }
}

// connect to wifi function
void connectWiFi(const char *ssid, const char *pass) {
    // Connect to WiFi
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    // Print network information to serial output
    Serial.println("");
    Serial.printf("Connected to: %s    IP Address: %s\n", 
        WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

// WiFi Event handler
void WiFiEvent(WiFiEvent_t event) 
{
    Serial.printf("[WiFi-event] event: %d\n", event);

    switch (event) {
        case SYSTEM_EVENT_STA_DISCONNECTED:
            WiFi.reconnect();
            break;
        case SYSTEM_EVENT_STA_START:
            esp_restart();
            break;
        case SYSTEM_EVENT_STA_LOST_IP:
            WiFi.reconnect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.print("Obtained IP address: ");
            Serial.println(WiFi.localIP());
            break;
        case SYSTEM_EVENT_STA_STOP:
            WiFi.reconnect();
            break;
        default:
            break;

    }
}