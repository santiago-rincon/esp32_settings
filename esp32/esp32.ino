#include <WiFi.h>
#include <HTTPClient.h>
#include <DFRobot_SHT3x.h>
#include "Adafruit_SHT4x.h"

const String ssid = "RaspberryAP"; 
const String password = "raspberry";
const String host = "192.168.20.1";
const String port = "8080";
const String url = "http://" + host + ":" + port;

DFRobot_SHT3x sht3x(&Wire, 0x44, 4);
Adafruit_SHT4x sht4 = Adafruit_SHT4x();

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);
  double* measuresSht31 = getTempertureAndHumidityA();
  int temp = measuresSht31[0];
  int ha = measuresSht31[1];
  double hs = getHumidityS();
  String mac = "aa:aa:aa:aa:aa:aa";
  // wifiConnection(ssid, password);
  // String mac  = getMac();
  String data = "{\"temp\":" + String(temp) + ",\"hs\":" + String(hs) + ",\"ha\":" + String(ha) + ",\"mac\":\"" + mac + "\"}";
  // sendRequest(url, data);
}

void loop() {}

double getHumidityS(){
  while (sht3x.begin() != 0) {
    Serial.println("Error al comunicarse con el sensor SHT31, por favor confirma conexiones");
    delay(500);
  }
  Serial.println("Sensor SHT31 conectado correctamente");
  // double temperature = sht3x.getTemperatureC();
  double humidity = sht3x.getHumidityRH();
  Serial.println("Humedad relativa: " + String(humidity) + "%");
  return humidity;
}

double* getTempertureAndHumidityA(){
  while(!sht4.begin()){
    Serial.println("Error al comunicarse con el sensor SHT40, por favor confirma conexiones");
    delay(500);
  }
  Serial.println("Sensor SHT40 conectado correctamente");
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);
  sensors_event_t humidity, temp;
  sht4.getEvent(&humidity, &temp);
  static double measures[2];
  measures[0] = temp.temperature;
  measures[1] = humidity.relative_humidity;
  Serial.println("Temperatura: " + String(measures[0]) + "°C");
  Serial.println("Humedad relativa: " + String(measures[1]) + "%");
  return measures;
}

void wifiConnection(String ssid, String password){
  WiFi.begin(ssid, password);
  Serial.print("Conectando al Wifi...");
  while(WiFi.status() != WL_CONNECTED){
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Conectado con éxito, IP: ");
  Serial.print(WiFi.localIP());
  Serial.print(" MAC: ");
  Serial.println(WiFi.macAddress());
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
    Serial.print("Enviando datos a la raspberrry: ");
    responseCode = http.POST(data);
    String response = http.getString();
    if(responseCode == 200){
      Serial.println(response);
    }
    else{
      Serial.println("Error en el envío de datos: " + response);
      attempts++;
    }
  } while (responseCode != 200 && attempts != 4);
  http.end();
}