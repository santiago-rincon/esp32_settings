#include <HTTPClient.h>
#include <wiFi.h>

const char* ssid = "nodorecolector";  //CONECTARSE AL PUNTO DE ACCESO 
const char* password = "nodorecolector";
const double VCC = 3.3;             // NodeMCU on board 3.3v vcc
const double R2 = 10000;            // 10k ohm series resistor
const double adc_resolution = 4095; // 10-bit adc
const int PIN_TEMPERATURA = 34;  //AQUI VA EL SENSOR DE TEMPERATURA
const double A = 0.002231872;   // thermistor equation parameters
const double B = 0.000030095;
const double C = 0.000001068;
int status=0; 
const int PIN_HUMEDAD = 35;   //AQUI VA EL SENSOR DE HUMEDAD
RTC_DATA_ATTR int bootCount = 0;

int MEDIR_HUMEDAD(){
  int sensorV = analogRead(35); 
  int porcentage = map(sensorV, 0, 4095, 100,0);
  return int (porcentage);
}

String MEDIR_TEMPERATURA(){
  
   float Vout, Rth, temperature, adc_value; 
   adc_value = analogRead(PIN_TEMPERATURA);
   Vout = (adc_value * VCC) / adc_resolution;
   Rth = (VCC * R2 / Vout) - R2;
   temperature = (1 / (A + (B * log(Rth)) + (C * pow((log(Rth)),3))));   // Temperature in kelvin
   temperature = temperature - 273.15;  // Temperature in degree celsius
   return String (temperature);
}

void setup(){
  Serial.begin(115200);
  pinMode(32, INPUT_PULLUP);
  delay(1000);

  // Incrementa el número de reinicios.
  ++bootCount;
  Serial.println("Numero de reinicios: " + String(bootCount));

  // Esto es para el método ext0.
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_26,0); //1 = High, 0 = Low
 //esp_sleep_enable_timer_wakeup(1*60*1000000);
  Serial.println("despierto");
  WiFi.begin(ssid, password);
  Serial.print("Conectando...");
  while(WiFi.status() != WL_CONNECTED) { //COMPROBANDO LA CONEXION 
    delay(1000);
    Serial.print(".");
 }
  Serial.print("Conectado con éxito, la IP es: ");
  Serial.println(WiFi.localIP());
  Serial.print("La MAC de la ESP32 es: ");
  Serial.println(WiFi.macAddress());
  String MAC = WiFi.macAddress();  //guardo la MAC de la placa en esta variable 
  String TEMPERATURA = MEDIR_TEMPERATURA();
  int HUMEDAD = MEDIR_HUMEDAD();
  Serial.print("Temperatura = ");
  Serial.print(TEMPERATURA);
  Serial.println(" GRADOS celsius");
  Serial.print("Porcentaje de humedad = ");
  Serial.print(HUMEDAD);
  Serial.println("%");
  do{
    if(WiFi.status() == WL_CONNECTED){ //COMPRUEBO LA CONEXION PARA ENTRAR EN EL LOOP 
      HTTPClient http; //le asigno variable al cliente http 
      //String  datos_a_enviar = "{"Temperatura":temperature,"MAC":"AA:AA:AA:AA:AA:AA:AA"}"; // datos enviados al servidor 
      http.begin("http://192.168.2.1:3050/read-sensor"); //poner URL del servidor para conectarme 
      http. addHeader("Content-Type", "application/json"); //para enviar datos en cadena ordenada
      String data = "{\"measureTemp\":"+TEMPERATURA+",\"measureHumS\":"+HUMEDAD+",\"MAC\":\""+String(MAC)+"\",\"node\":1}";
      status = http.POST(data);
      if(status == 200){
      //String cuerpo_respuesta = http.getString();
        //Serial.println("El servidor respondió:  ");
      // Serial.println(cuerpo_respuesta);    
        Serial.println("OK");
        http.end(); //liberar recursos
      }
      else{
        Serial.println("Error de conexión");
        delay(3000);
      }
    } 
  } while(WiFi.status() != WL_CONNECTED || status!=200 );  
  delay(5000);
  esp_deep_sleep_start();
  Serial.println("ESTO NO SE DEBE IMPRIMIR");
}

void loop(){
  // This is not going to be called
}