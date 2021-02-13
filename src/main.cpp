#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PMS.h>

#define RX_PIN 14
#define TX_PIN 12
#define DHT_PIN 13
#define SET_PIN D8
#define RESET_PIN D3
#define DHTTYPE DHT11 // DHT 11

#define BAUD 9600
// LCD
#define LCD_ADDR 0x27
#define LCD_COL 16
#define LCD_ROW 2

// Esp
#define WARM_UP 30000
#define SLEEP_DURATION 10
#define MILLISECOND 1000      // secondes
#define SECONDS 1000      // secondes
#define MINUTES 60 * SECONDS * MILLISECOND // secondes

struct D11Sensor
{
  float temperature;
  float humidity;
};

struct PMSensor
{
  String pm25;
  String pm10;
};

D11Sensor readD11Sensor();
void printLcdDisplay(D11Sensor d11Sensor, PMSensor pMSensor);
void connectToWifi();
void sendDataToWifi(D11Sensor d11Sensor, PMSensor pMSensor);
void initWakeup();
void hibernate();
void readData();
PMSensor readPMSensor();

//WIFI
const char *ssid = "GREG";
const char *password = "19032014";
const char *host = "192.168.1.4"; //edit the host adress, ip address etc.
const uint16_t httpPort = 8090;
String url = "/api/weather";

// Liquid crystal
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COL, LCD_ROW); // set the LCD address to 0x27 for a 16 chars and 2 line display

// DTH Sensor
DHT dht(DHT_PIN, DHTTYPE);

// PM sensor
PMS ag = PMS(true, BAUD);

////////////////////////////////////////

void setup()
{
  initWakeup();
  D11Sensor d11Sensor = readD11Sensor();
  if (isnan(d11Sensor.humidity) || isnan(d11Sensor.temperature))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Failed");
    hibernate();
  }
  PMSensor pMSensor = readPMSensor();
  printLcdDisplay(d11Sensor, pMSensor);
  delay(500);
  sendDataToWifi(d11Sensor, pMSensor);
  hibernate();
}

void loop()
{
}

void initWakeup()
{
  Serial.begin(BAUD);

  lcd.init(); // initialize the lcd
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  dht.begin();
  Serial.println("Wakeup PM");
  ag.PMS_Init(RX_PIN, TX_PIN, SET_PIN, RESET_PIN);
  delay(1 * SECONDS);
  Serial.flush();
  ag.reset();
  delay(1 * SECONDS);
  ag.wakeUp();
  connectToWifi();
}
void hibernate()
{
  Serial.println("sleep PM");
  Serial.flush();
  ag.sleep(); // Sleep PM sensor
  delay(500);
  ESP.deepSleep(SLEEP_DURATION * MINUTES);
}

PMSensor readPMSensor()
{
    lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Warming...");
  delay(WARM_UP); // wait 30 seconds
  PMSensor pMSensor;
  pMSensor.pm25 = ag.getPM2();
  pMSensor.pm10 = ag.getPM10();
  return pMSensor;
}

D11Sensor readD11Sensor()
{
  D11Sensor d11Sensor;
  d11Sensor.humidity = dht.readHumidity();       // read humidity
  d11Sensor.temperature = dht.readTemperature(); // read temperature
  return d11Sensor;
}

void printLcdDisplay(D11Sensor d11Sensor, PMSensor pMSensor)
{
  String humidity = F("H: ");
  humidity += String(d11Sensor.humidity, 0);
  humidity += F("%");
  String temperature = F("T: ");
  temperature += String(d11Sensor.temperature, 1);
  temperature += F("C");
  String pm25 = F("25: ");
  pm25 += pMSensor.pm25;
  String pm10 = F("10: ");

  pm10 += pMSensor.pm10;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(humidity);
  lcd.setCursor(7, 0);
  lcd.print(temperature);

  lcd.setCursor(0, 1);
  lcd.print(pm25);
  lcd.setCursor(7, 1);
  lcd.print(pm10);
  Serial.println(humidity);
  Serial.println(temperature);
  Serial.println(pm25);
  Serial.println(pm10);
}

void sendDataToWifi(D11Sensor d11Sensor, PMSensor pMSensor)
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
  postData += F(", \"pm25\": ");
  postData += pMSensor.pm25;
  postData += F(", \"pm10\": ");
  postData += pMSensor.pm10;
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
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting...");
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

void readData()
{
  PMS::DATA data;



  Serial.println("Send read request...");
  ag.requestRead();

  Serial.println("Reading data...");
  if (ag.readUntil(data))
  {
    Serial.println();

    Serial.print("PM 1.0 (ug/m3): ");
    Serial.println(data.PM_AE_UG_1_0);

    Serial.print("PM 2.5 (ug/m3): ");
    Serial.println(data.PM_AE_UG_2_5);

    Serial.print("PM 10.0 (ug/m3): ");
    Serial.println(data.PM_AE_UG_10_0);

    Serial.println();
  }
  else
  {
    Serial.println("No data.");
  }
}