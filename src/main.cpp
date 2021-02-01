#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SoftwareSerial.h>

#define RXPIN 3
#define TXPIN 1
#define DHTPIN 13
#define DHTTYPE DHT11 // DHT 11
// LCD
#define LCD_ADDR 0x27
#define LCD_COL 16
#define LCD_ROW 2

// Esp
#define SLEEP_DURATION 15
#define SECONDS 1000000      // secondes
#define MINUTES 60 * SECONDS // secondes

struct D11Sensor
{
  float temperature;
  float humidity;
  float heatIndex;
};

D11Sensor readD11Sensor();
void printLcdDisplay(D11Sensor d11Sensor);
void connectToWifi();
void sendDataToWifi(D11Sensor d11Sensor);

//WIFI
const char *ssid = "GREG";
const char *password = "19032014";
const char *host = "192.168.1.4"; //edit the host adress, ip address etc.
const uint16_t httpPort = 8090;
String url = "/api/weather";

LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COL, LCD_ROW); // set the LCD address to 0x27 for a 16 chars and 2 line display
DHT dht(DHTPIN, DHTTYPE);

// PM sensor 
SoftwareSerial pmsSerial(RXPIN, TXPIN);


void setup()
{
  lcd.init(); // initialize the lcd
  lcd.backlight();

  Serial.begin(9600);
  // sensor baud rate is 9600
  pmsSerial.begin(9600);
  
  dht.begin();
  connectToWifi();

  D11Sensor d11Sensor = readD11Sensor();
  if (isnan(d11Sensor.humidity) || isnan(d11Sensor.temperature))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    ESP.deepSleep(SLEEP_DURATION * MINUTES);
  }
  printLcdDisplay(d11Sensor);

  delay(500);
  sendDataToWifi(d11Sensor);
  ESP.deepSleep(SLEEP_DURATION * MINUTES);
}

void loop()
{
}

D11Sensor readD11Sensor()
{
  D11Sensor d11Sensor;
  d11Sensor.humidity = dht.readHumidity();       // read humidity
  d11Sensor.temperature = dht.readTemperature(); // read temperature
  d11Sensor.heatIndex = NAN;
  if (!isnan(d11Sensor.humidity) || !isnan(d11Sensor.temperature))
  {
    d11Sensor.heatIndex = dht.computeHeatIndex(d11Sensor.temperature, d11Sensor.humidity, false);
  }
  return d11Sensor;
}

void printLcdDisplay(D11Sensor d11Sensor)
{
  String humidity = F("Hum: ");
  humidity += String(d11Sensor.humidity, 0);
  humidity += F("%");
  String temperature = F("Temp: ");
  temperature += String(d11Sensor.temperature, 1);
  temperature += F("C");
  String heatIndex = F("Heat: ");
  heatIndex += String(d11Sensor.heatIndex, 1);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(humidity);
  lcd.setCursor(0, 1);
  lcd.print(temperature);
  Serial.println(humidity);
  Serial.println(temperature);
  Serial.println(heatIndex);
}

void sendDataToWifi(D11Sensor d11Sensor)
{

  Serial.print("connecting to ");
  Serial.println(host); // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(host, httpPort))
  {
    Serial.println("connection failed");
    return;
  }
  Serial.print("Requesting URL: ");
  Serial.println(url); //Post Data
  String postData = F("{ \"temperature\": ");
  postData += String(d11Sensor.temperature, 1);
  postData += F(", \"humidity\": ");
  postData += String(d11Sensor.humidity, 0);
  postData += F(" }");
  Serial.println(postData);

  HTTPClient http;
  http.begin(client, host, httpPort, url);
  http.addHeader("Content-Type", "application/json");
  auto httpCode = http.POST(postData);
  Serial.println(httpCode); //Print HTTP return code
  String payload = http.getString();
  Serial.println(payload); //Print request response payload
  http.end();              //Close connection Serial.println();
  Serial.println("closing connection");
}

void connectToWifi()
{
  // Setup wifi
  delay(10); // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default, would try to act as both a client and an access-point and could cause network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}