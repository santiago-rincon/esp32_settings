/* Líbrerias necesarias para el correcto funcionamiento de todo el sistema */
#include <WiFi.h> // Líbreria para la conexión WiFi
#include <HTTPClient.h> // Líbreria para las peticiones HTTP a la estación base
#include <DFRobot_SHT3x.h> // Líbreria para la serie de sensores SHT3X (SHT31)
#include "Adafruit_SHT4x.h" // Líbreria para la serie de sensores SHT4X (SHT40)
#include <esp_sleep.h>  // Líbreria para el manejo de los modos de energía
#include <driver/uart.h>  // Líbreria para controlar despertar  la ESP de modo Light sleep a través del UART 0

/* Configuración de constantes para la comunicación con la estación base */
const String ssid = "RaspberryAP";  // Nombre del SSID del punto de acceso creado por la estación base
const String password = "raspberry";  // Contraseña para conectarse al punto de acceso creado por la estación base
const String host = "192.168.20.1";   // Dirección IP del punto de acceso creado por la estación base (Gateway)
const String port = "8080";   // Número del puerto por el cual corre el servicio HTTP creado por la estación base
const String url = "http://" + host + ":" + port;   // Concatenación de la url completa a partir de los anteriores datos

/* Creación de objetos para la lectura de los sensores */
DFRobot_SHT3x sht3x(&Wire, 0x44, 4);  // Creación de un objeto tipo SHT3X para la lectura del sensor SHT31 con dirección 0x44 (dirección por defecto)
Adafruit_SHT4x sht4 = Adafruit_SHT4x();  // Creación de un objeto tipo SHT4X para la lectura del sensor SHT40 (la inicialización del objeto se encarga de buscar la dirección del dispositivo)

/* Definición de la configuración inicial de puerto UART y otros pines para debuging */
void setup() {
  Serial.begin(9600);   // Inicio del serial (UART0) a 9600 baudios (velocidad de comunicación por defecto de los módulos HC12)
  while (!Serial) delay(500);   // Bucle infinito hasta que el Serial se inicie correctamente
  uart_set_wakeup_threshold(0, 3);  // Establecimiento de umbral (3) para despertar la ESP32 a través del UART0 (0)
  esp_sleep_enable_uart_wakeup(0);  // Establecimiento del puerto UART el cual va a despertar la ESP32 (el UART 2 no permite esta función)
  pinMode(2, OUTPUT);   // Configuración del pin 2 como salida para conocer cuando la ESP32 esta en proceso de lectura, conexión y envío de datos (Proceso completo)
  pinMode(15, OUTPUT);   // Configuración del pin 15 como salida para conocer cuando la ESP32 esta en proceso de lectura del los sensores
  pinMode(5, OUTPUT);   // Configuración del pin 5 como salida para conocer cuando la ESP32 esta en proceso de conexión con el WiFi
  pinMode(18, OUTPUT);   // Configuración del pin 18 como salida para conocer cuando la ESP32 esta en proceso de envío de datos a través del protocolo HTTP
}

/* Loop infinito que se ejecutará cada vez que la ESP32 despierte del modo Light sleep */
void loop() {
  digitalWrite(2, HIGH);  // pin 2 encendido para notificar el inicio del proceso completo
  digitalWrite(15, HIGH);  // pin 15 encendido para notificar el inicio del proceso de la lectura de sensores
  double* measuresSht31 = getSHT31();   // Obtención de la temperatura la humedad relativa (vector de dos posisciones) del sensor SHT31
  double temp = measuresSht31[0];   // Asiganación de la primera posición del vector obtenido a la variable temp
  double hs = measuresSht31[1];   // Asiganación de la segunda posición del vector obtenido a la variable hs
  // double* measuresSht40 = getSHT40();  // Obtención de la temperatura la humedad relativa (vector de dos posisciones) del sensor SHT40
  // double temp = measuresSht40[0];   // Asiganación de la primera posición del vector obtenido a la variable temp
  // double ha = measuresSht40[1];  // Asiganación de la segunda posición del vector obtenido a la variable ha
  // double hs = 50.25;  // Valor asignado mientras se define el sensor de esta lectura
  double ha = 50.25;  // Valor asignado mientras se define el sensor de esta lectura
  double co2 = getCo2();  // Asignación de la cantidad de CO2 a través de la función getCo2
  double rad = getLuminosity();  // Asignación de la cantidad de radiación solar a través de la función getLuminosity
  delay(10000);   // Espera de 10 segundos para que le módulo WiFi se configure correctamente una vez despierta del modo Light sleep
  digitalWrite(15, LOW);  // pin 15 apagado para notificar el fin del proceso de la lectura de sensores
  digitalWrite(5, HIGH);  // pin 5 encendido para notificar el inicio del proceso de la conexion WiFi
  wifiConnection(ssid, password);   // Intento de conexión al punto de acceso definido en las contantes globales
  String mac  = getMac();   // Obtención de la dirección MAC una vez realizada la conexión exitosamente
  digitalWrite(5, LOW);  // pin 5 apagado para notificar el fin del proceso de la conexion WiFi
  String data = "{\"temp\":" + String(temp) + ",\"hs\":" + String(hs) + ",\"ha\":" + String(ha) + ",\"co2\":" + String(co2) + ",\"rad\":" + String(rad) + ",\"mac\":\"" + mac + "\"}";  // Construcción del objeto JSOn que se enviará a la estación base
  digitalWrite(18, HIGH);  // pin 18 encendido para notificar el inicio del proceso de envío de datos a través del protocolo HTTP
  sendRequest(url, data);   // Envío de datos a través del protocolo HTTP a la URL definida y con los datos construidos anteriormente
  delay(10000);   // Espera de 10 segundos para asegurar el correcto envío de datos
  digitalWrite(18, LOW);  // pin 18 apagado para notificar el fin del proceso de envío de datos a través del protocolo HTTP
  digitalWrite(2, LOW);  // pin 2 apagado para notificar el fin del proceso completo
  esp_light_sleep_start();  // ESP32 en modo light sleep
}

/* Conexión del módulo WiFi (recibe por parámetros el ssid y la contraseña) */
void wifiConnection(String ssid, String password){
  WiFi.begin(ssid, password);   // Inicialización de un objeto WiFi con el ssid y la contraseña proporcionada por parámetros
  while(WiFi.status() != WL_CONNECTED) delay(1000);   // Bucle infinito hasta no lograr una conexión exitosa
}

/* Obtención de la dirección MAC de la ESP32 */
String getMac(){
  return WiFi.macAddress();   // Retorno en formato string de la dirección MAC de la ESP32
}

/* Envío de petición HTTP a la estación base (recibe por parámetros la URL destino y los datos en formato JSON) */
void sendRequest(String url, String data){
  int attempts = 0;   // Creación de variable entera para controlar el número de intentos de la petición en caso de que falle
  int responseCode = 0;   // Creación de variable entera para controlar el código de estado de la petición HTTP
  HTTPClient http;  // Creación de un objeto HTTP
  http.begin(url);  // Inicialización del objeto HTTP con la URLproporcionada por parámetro
  http.addHeader("Content-Type", "application/json");   // Adición de la cabecera para enviar los datos en formato JSON (necesaria para la estación base)
  /* Inicio de un bucle controlado para envir la petici´´on HTTP más de unavez en caso de que falle la primera vez */
  do{
    responseCode = http.POST(data);   // Almacenamiento del código de estado enviado por la estación base
    if(responseCode != 200) attempts++;   // Condicional para aumentar el número de intentos en caso de que el código de estado sea diferente de 200 (200 OK)
  } while (responseCode != 200 && attempts != 4);   // Control del bucle. El bucle se repite mientras el código de estado no sea 200 o en número de intentos sea difernete de 4 
  http.end();   // Culminación del objeto HTTP para evitar desbordameinto de memoria
}

/* Obtención de la temperatura y humedad del sensor SHT31 */
double* getSHT31(){
  while (sht3x.begin() != 0) delay(500);   // Bucle infinito hasta que no se establezca comunicación con el sensor SHT31
  double temperature = sht3x.getTemperatureC();   // Obtención de la temperatura en grados centigrados en formato flotente
  double humidity = sht3x.getHumidityRH();   // Obtención de la humedad relativa (%) en formato flotente
  static double measures[2];  // Creación de un vector de dos posiciones que se retornará posteriormente
  measures[0] = temperature;  // Asignación de la temperatura leida a la primera posisción del vector
  measures[1] = humidity;  // Asignación de la humedad relativa leida a la segunda posisción del vector
  return measures;  // Retorno del vector con los valores de temperatura y humedad (primera y segunda posición respectivamente)
}

/* Obtención de la temperatura y humedad del sensor SHT31 */
double* getSHT40(){
  while(!sht4.begin()) delay(500);   // Bucle infinito hasta que no se establezca comunicación con el sensor SHT40
  sht4.setPrecision(SHT4X_HIGH_PRECISION);  // Configuración de la presición del sensor
  sht4.setHeater(SHT4X_NO_HEATER);  // Configuración del calentador interno del sensor
  sensors_event_t humidity, temp;   // Creación de dos eventos que permiten leer la temperatura y humedad
  sht4.getEvent(&humidity, &temp);  // Obtención de temperatura y humedad a través de los eventos creados anteriormente
  static double measures[2];  // Creación de un vector de dos posiciones que se retornará posteriormente
  measures[0] = temp.temperature;  // Asignación de la temperatura leida a la primera posisción del vector
  measures[1] = humidity.relative_humidity;  // Asignación de la humedad relativa leida a la segunda posisción del vector
  return measures;  // Retorno del vector con los valores de temperatura y humedad (primera y segunda posición respectivamente)
}

/* Incluir lógica necesaria dentro de la función para la lectura del sensor de CO2 */
double getCo2(){
  return 600.25;
}

/* Incluir lógica necesaria dentro de la función para la lectura del sensor de luminosidad o radiación solar */
double getLuminosity(){
  return 126.29;
}