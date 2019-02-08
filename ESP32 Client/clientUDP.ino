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
    The server host must have an IP address of 192.168.0.118 port 8787 on the Wireless network.
    There is a maximum of 60 other clients
    All boards are configured as a Station and AP

    (c) Caelan Murch

*/

#include <WiFi.h> // Include wifi header
#include <WiFiUdp.h> // UDP header
#include <cstring> // C string library

// Setup wifi information
const char *AP_SSID = "Measure37";
const char *AP_PASS = "pass1234";

const char *NET_SSID = "measure";
const char *NET_PASS = "measure200";
const char *NET_HOST = "192.168.0.118";
const int NET_PORT = 8787;

// Restart count
int restartCount;

// Prototype functions
int getRSSI(String SSID[], int RSSI[]);
void sendRSSI(String SSID[], int RSSI[], int apcount);
void connectWiFi(char *ssid, char *pass);
void WiFiEvent(WiFiEvent_t event);

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

    // set restart count to 0
    restartCount = 0;

    // Set wifi mode
    WiFi.mode(WIFI_AP_STA);

    // Set hostname
    WiFi.setHostname(AP_SSID);

    // Setup wifi event handler
    WiFi.onEvent(WiFiEvent);

    // Start softap on channel 11
    WiFi.softAP(AP_SSID, AP_PASS, 11);
    Serial.println("Started soft-ap... ");

    // connect to AP on any channel
    connectWiFi(NET_SSID, NET_PASS, 0);

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
        udp.printf("TX%d Hello from %s\r\n", (int)WiFi.getTxPower(), AP_SSID);
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
    // Get number of APs on channel 11
    WiFi.channel(11);
    int n = WiFi.scanNetworks(false, false, true, 300); // <- scan in passive
    // Scan wifi in active
    //int n = WiFi.scanNetworks();
    Serial.println("Finished scanning");
    delay(1000);
    int count = 0;
    // Check if there are too many APs
    if (n <= 0 || n >= 60) {
        Serial.printf("Number of APs: %d\n", n);
        Serial.println("No networks found...");
        restartCount++;
        // Restart if unable to connect after 8 tries
        if (restartCount == 8)
            esp_restart();
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
void connectWiFi(const char *ssid, const char *pass, int channel) {
    // Connect to WiFi
    WiFi.begin(ssid, pass, channel);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    // Print network information to serial output
    Serial.println("");
    Serial.printf("Connected to: %s    IP Address: %s\n", 
        WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

// Wifi Even handler. Based off example code.
void WiFiEvent(WiFiEvent_t event)
{
    Serial.printf("[WiFi-event] event: %d\n", event);
    // Switch case to process all possible events
    switch (event) {
        case SYSTEM_EVENT_WIFI_READY: 
            Serial.println("WiFi interface ready");
            break;
        case SYSTEM_EVENT_SCAN_DONE:
            Serial.println("Completed scan for access points");
            break;
        case SYSTEM_EVENT_STA_START:
            Serial.println("WiFi client started");
            break;
        case SYSTEM_EVENT_STA_STOP:
            WiFi.reconnect();
            delay(2000);
            Serial.println("WiFi clients stopped");
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            Serial.println("Connected to access point");
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            WiFi.reconnect();
            Serial.println("Disconnected from WiFi access point");
            break;
        case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
            Serial.println("Authentication mode of access point has changed");
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.print("Obtained IP address: ");
            Serial.println(WiFi.localIP());
            break;
        case SYSTEM_EVENT_STA_LOST_IP:
            WiFi.reconnect();
            Serial.println("Lost IP address and IP address is reset to 0");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
            Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_FAILED:
            Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
            Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_PIN:
            Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
            break;
        case SYSTEM_EVENT_AP_START:
            Serial.println("WiFi access point started");
            break;
        case SYSTEM_EVENT_AP_STOP:
            Serial.println("WiFi access point  stopped");
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            Serial.println("Client connected");
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            Serial.println("Client disconnected");
            break;
        case SYSTEM_EVENT_AP_STAIPASSIGNED:
            Serial.println("Assigned IP address to client");
            break;
        case SYSTEM_EVENT_AP_PROBEREQRECVED:
            Serial.println("Received probe request");
            break;
        case SYSTEM_EVENT_GOT_IP6:
            Serial.println("IPv6 is preferred");
            break;
        case SYSTEM_EVENT_ETH_START:
            Serial.println("Ethernet started");
            break;
        case SYSTEM_EVENT_ETH_STOP:
            Serial.println("Ethernet stopped");
            break;
        case SYSTEM_EVENT_ETH_CONNECTED:
            Serial.println("Ethernet connected");
            break;
        case SYSTEM_EVENT_ETH_DISCONNECTED:
            Serial.println("Ethernet disconnected");
            break;
        case SYSTEM_EVENT_ETH_GOT_IP:
            Serial.println("Obtained IP address");
            break;
    }
}