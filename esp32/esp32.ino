#include <WiFi.h>
#include <HTTPClient.h>
#include <DFRobot_SHT3x.h>
#include "Adafruit_SHT4x.h"
#include <esp_sleep.h>
#include <driver/uart.h>

const String ssid = "RaspberryAP"; 
const String password = "raspberry";
const String host = "192.168.20.1";
const String port = "8080";
const String url = "http://" + host + ":" + port;

DFRobot_SHT3x sht3x(&Wire, 0x44, 4);
Adafruit_SHT4x sht4 = Adafruit_SHT4x();

void setup() {
  Serial.begin(9600);
  while (!Serial) delay(500);
  uart_set_wakeup_threshold(0, 3);
  esp_sleep_enable_uart_wakeup(0);
  pinMode(2, OUTPUT);
  pinMode(15, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(18, OUTPUT);
}

void loop() {
  digitalWrite(2, HIGH);
  digitalWrite(15, HIGH);
  double* measuresSht31 = getSHT31();
  double temp = measuresSht31[0];
  double hs = measuresSht31[1];
  // double* measuresSht40 = getSHT40();
  // double ha = measuresSht40[1];
  double ha = 50.25;
  delay(10000);
  digitalWrite(15, LOW);
  digitalWrite(5, HIGH);
  wifiConnection(ssid, password);
  String mac  = getMac();
  digitalWrite(5, LOW);
  String data = "{\"temp\":" + String(temp) + ",\"hs\":" + String(hs) + ",\"ha\":" + String(ha) + ",\"mac\":\"" + mac + "\"}";
  digitalWrite(18, HIGH);
  sendRequest(url, data);
  delay(10000);
  digitalWrite(18, LOW);
  digitalWrite(2, LOW);
  esp_light_sleep_start();
}

double* getSHT31(){
  while (sht3x.begin() != 0) delay(500);
  double temperature = sht3x.getTemperatureC();
  double humidity = sht3x.getHumidityRH();
  static double measures[2];
  measures[0] = temperature;
  measures[1] = humidity;
  return measures;
}

double* getSHT40(){
  while(!sht4.begin()) delay(500);
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);
  sensors_event_t humidity, temp;
  sht4.getEvent(&humidity, &temp);
  static double measures[2];
  measures[0] = temp.temperature;
  measures[1] = humidity.relative_humidity;
  return measures;
}

void wifiConnection(String ssid, String password){
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) delay(1000);
}

String getMac(){
  return WiFi.macAddress();
}

void sendRequest(String url, String data){
  int attempts = 0;
  int responseCode = 0;
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  do{
    responseCode = http.POST(data);
    if(responseCode != 200) attempts++;
  } while (responseCode != 200 && attempts != 4);
  http.end();
}