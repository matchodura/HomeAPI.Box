/* DHTServer - ESP8266 Webserver with a DHT sensor as an input
 
   Based on ESP8266Webserver, DHTexample, and BlinkWithoutDelay (thank you)
 
   Version 1.0  5/3/2014  Version 1.0   Mike Barela for Adafruit Industries
*/
#include <ESP8266WiFi.h>

#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <BH1750.h>

#include <DHT.h>
#include <ArduinoJson.h>


#define DHTTYPE DHT22
#define DHTPIN 0
#define timeSeconds 10
#define SCL 5
#define SDA 4

const char* ssid     = "940C6DAC53AA";
const char* password = "rymcymcym";

//const char* ssid     = "LEON.PL_2332_5G";
//const char* password = "65f6e57d";

// Set your Static IP address
IPAddress local_IP(192, 168, 0, 21);
// Set your Gateway IP address
IPAddress gateway(192, 168, 0, 1);

IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional


//pir sensor
const int motionSensor = 15;
const int led = 14;
const int ledInfo = 12;
ESP8266WebServer server(80);
 
// Initialize DHT sensor 
// NOTE: For working with a faster than ATmega328p 16 MHz Arduino chip, like an ESP8266,
// you need to increase the threshold for cycle counts considered a 1 or 0.
// You can do this by passing a 3rd parameter for this threshold.  It's a bit
// of fiddling to find the right value, but in general the faster the CPU the
// higher the value.  The default for a 16mhz AVR is a value of 6.  For an
// Arduino Due that runs at 84mhz a value of 30 works.
// This is for the ESP8266 processor on ESP-01 
DHT dht(DHTPIN, DHTTYPE, 11); // 11 works fine for ESP8266


//bht1750

BH1750 luxmeter;


 
float humidity, temp_f, luxes;  // Values read from sensor
String webString="";     // String to display

// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousMillis = 0;        // will store last temp was read
const long interval = 2000;              // interval at which to read sensor

unsigned long previousMillisNotify = 0;        // will store last temp was read
              // interval at which to read sensor


// Timer: Auxiliary variables
unsigned long now = millis();
unsigned long lastTrigger = 0;
boolean startTimer = false;
boolean isMotion = false;
boolean wasMotion = false;

struct clientData{
  char device[10];
  char temp[8];
  char humidity[8];
};




const int rainMeterDigital = 16;
const int rainMeterAnalog = A0;
int rainMeterMeasure;
String rainMeterStatus = "";

String deviceName = "box_1";

String URLon = "http://192.168.0.183/api/motion/on?device=zlodziej";
String URLoff = "http://192.168.0.183/api/motion/off?device=zlodziej";

void handle_root() {
  server.send(200, "text/plain", "Hello from the weather esp8266, read from /temp or /humidity");
  delay(100);
}

ICACHE_RAM_ATTR void detectsMovement() {
  Serial.println("MOTION DETECTED!!!");  
  digitalWrite(led, HIGH);  
  startTimer = true;
  lastTrigger = millis();  
  isMotion = true;
}

void setup(void)
{
  // You can open the Arduino IDE Serial Monitor window to see what the code is doing
  Serial.begin(115200);  // Serial connection from ESP-01 via 3.3v console cable
  dht.begin();           // initialize temperature sensor

  Wire.begin(SDA,SCL);
  if (luxmeter.begin()) {
    Serial.println(F("BH1750 initialised"));
  }
  else {
    Serial.println(F("Error initialising BH1750"));
  }

  pinMode(rainMeterDigital, INPUT);
  pinMode(rainMeterAnalog, INPUT);

  
  // PIR Motion Sensor mode INPUT_PULLUP
  pinMode(motionSensor, INPUT_PULLUP);
  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);

  // Set LED to LOW
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  
  pinMode(ledInfo, OUTPUT);
  digitalWrite(ledInfo, LOW);
  
   // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  
  // Connect to WiFi network
  WiFi.begin(ssid, password);
  Serial.print("\n\r \n\rWorking to connect");
 
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("DHT Weather Reading Server");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
   
  server.on("/", handle_root);
  server.on("/changeurl/motionOn", handle_url_on);
  server.on("/changeurl/motionOff", handle_url_off);
  
  server.on("/temp", [](){  // if you add this subdirectory to your webserver call, you get text below :)
    gettemperature();       // read sensor
    webString=String((int)temp_f);   // Arduino has a hard time with float to string
    server.send(200, "text/plain", webString);            // send to someones browser when asked
  });
 
  server.on("/humidity", [](){  // if you add this subdirectory to your webserver call, you get text below :)
    gettemperature();           // read sensor
    webString="Humidity: "+String((int)humidity)+"%";
    server.send(200, "text/plain", webString);               // send to someones browser when asked
  });

   server.on("/light", [](){  // if you add this subdirectory to your webserver call, you get text below :)
    getlight();       // read sensor
   // webString=String((int)luxes);   // Arduino has a hard time with float to string
   // server.send(200, "text/plain", webString);            // send to someones browser when asked
     createlightJSON("lightmeter"); 
  });

  server.on("/rain", [](){  // if you add this subdirectory to your webserver call, you get text below :)
    getRainValues();       // read sensor
    webString=String(((int)rainMeterMeasure) + rainMeterStatus);   // Arduino has a hard time with float to string
    server.send(200, "text/plain", webString);            // send to someones browser when asked
   
  });
  
  server.on("/values", [](){  // if you add this subdirectory to your webserver call, you get text below :)
    
   gettemperature();       // read sensor
  
   server.on("/dht/config", handle_url_on);    
   createDHTJSON("DHT");    
  });

  server.on("/AllValues", [](){  // if you add this subdirectory to your webserver call, you get text below :)
    getlight();       // read sensor
    gettemperature(); 
    createTotalJSON(); 
  });



  
  server.begin();
  Serial.println("HTTP server started");
}

void TurnInfoLedOn(){
  digitalWrite(ledInfo, HIGH);  
}

void TurnInfoLedOff(){
  digitalWrite(ledInfo, LOW);  
}

void handle_url_on(){
      String message = "";
      message += server.args();            //Get number of parameters
      message += "\n";                            //Add a new line

      String response ="";
      
      for (int i = 0; i < server.args(); i++) {
      
      message += "Arg nº" + (String)i + " –> ";   //Include the current iteration value
      message += server.argName(i) + ": ";     //Get the name of the parameter
      message += server.arg(i) + "\n";              //Get the value of the parameter
      
      response = server.arg(i);
      } 


      Serial.print("Old URL:");
      Serial.println(URLon);
      
      URLon = response + "?deviceName=motion_sensor_1&boxId=1";
      
      Serial.print("New URL is set!");
      Serial.println(URLon);
      
      server.send(200, "text/plain", response);       //Response to the HTTP request
}

void handle_url_off(){
      String message = "";
      message += server.args();            //Get number of parameters
      message += "\n";                            //Add a new line

      String response ="";
      
      for (int i = 0; i < server.args(); i++) {
      
      message += "Arg nº" + (String)i + " –> ";   //Include the current iteration value
      message += server.argName(i) + ": ";     //Get the name of the parameter
      message += server.arg(i) + "\n";              //Get the value of the parameter
      
      response = server.arg(i);
      } 


      Serial.print("Old URL:");
      Serial.println(URLoff);
      
      URLoff = response + "?deviceName=motion_sensor_1&boxId=1";
      
      Serial.print("New URL is set!");
      Serial.println(URLoff);
      
      server.send(200, "text/plain", response);       //Response to the HTTP request
}
 
void loop(void)
{
  server.handleClient();

  // Current time
  now = millis();
  // Turn off the LED after the number of seconds defined in the timeSeconds variable
  if(startTimer && (now - lastTrigger > (timeSeconds*1000))) {
    Serial.println("Motion stopped...");
    digitalWrite(led, LOW);
    startTimer = false;
    isMotion = false;
    wasMotion = true;
  }       


  if(isMotion){
      
        if(now - previousMillisNotify >= interval) {
          // save the last time you read the sensor 
          previousMillisNotify = now;   
            HTTPClient http;
            String urlOn = URLon;            
            http.begin(urlOn); // Works with HTTP           
            
            int httpCode = http.GET();
            if (httpCode > 0) {
              String payload = http.getString();
              Serial.println(payload); // Print response
            }
    
            else{
              Serial.print("Error: ");
              Serial.println(httpCode);
            }    
            
            http.end();       
        }
  }

  if(wasMotion){
      
        if(now - previousMillisNotify >= interval) {
          // save the last time you read the sensor 
          previousMillisNotify = now;   
            HTTPClient http;
            String urlOff = URLoff;   
            Serial.print(urlOff);         
            http.begin(urlOff); // Works with HTTP           
            
            int httpCode = http.GET();
            if (httpCode > 0) {
              String payload = http.getString();
              Serial.println(payload); // Print response
            }
    
            else{
              Serial.print("Error: ");
              Serial.println(httpCode);
            }    
            
            http.end();   
            wasMotion = false;    
        }
  }
  
} 


void createDHTJSON(String sensorType){
  const size_t capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(3) + 60;
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& root = jsonBuffer.createObject();
  //StaticJsonDocument<256> doc;
  //JsonObject root = doc.to<JsonObject>();
  Serial.println(sensorType);
     
    root["device"] = deviceName;
    root["sensorType"] = "DHT";
    root["temp"] = temp_f;
    root["humidity"] = humidity;
  

   String json;
   root.printTo(json);

   server.send(200, "application/json", json);
}

void createlightJSON(String sensorType){
  const size_t capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(3) + 60;
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& root = jsonBuffer.createObject();
 
   Serial.println(sensorType);
  
 
  root["device"] = deviceName;
  root["sensorType"] = "lightmeter";
  root["luxes"] = luxes;  
  
  
   String json;
   root.printTo(json);

   server.send(200, "application/json", json);
}


void createTotalJSON(){
  const size_t capacity = JSON_ARRAY_SIZE(2) + JSON_OBJECT_SIZE(3) + 60;
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& root = jsonBuffer.createObject();
    
  root["room"] = "pokoj";
  root["device"] = deviceName;
  root["temp"] = temp_f;
  root["humidity"] = humidity;
  root["luxes"] = luxes;  
  
   String json;
   root.printTo(json);

   server.send(200, "application/json", json);
}


void getRainValues(){
TurnInfoLedOn();

if(digitalRead(rainMeterDigital) == LOW) 
  {
    Serial.println("Digital value : wet"); 
    rainMeterStatus = " mokro";
   
  }
else
  {
    Serial.println("Digital value : dry");
    rainMeterStatus = " sucho";
   
  }
rainMeterMeasure=analogRead(rainMeterAnalog); 
 Serial.print("Analog value : ");
 Serial.println(rainMeterMeasure); 
 Serial.println("");

TurnInfoLedOff();
}


void getlight() {
  // Wait at least 2 seconds seconds between measurements.
  // if the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor
  // Works better than delay for things happening elsewhere also
  TurnInfoLedOn();
  unsigned long currentMillis = millis();
  
  if(currentMillis - previousMillis >= interval) {
    // save the last time you read the sensor 
    previousMillis = currentMillis;   
 
    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    
    luxes = luxmeter.readLightLevel();          // Read humidity (percent)
    
    
    if (isnan(luxes)) {
      Serial.println("Failed to read from BHT1750 sensor!");
      return;
    }
     
  }
  TurnInfoLedOff();
}

 
void gettemperature() {
  TurnInfoLedOn();
  // Wait at least 2 seconds seconds between measurements.
  // if the difference between the current time and last time you read
  // the sensor is bigger than the interval you set, read the sensor
  // Works better than delay for things happening elsewhere also
  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis >= interval) {
    // save the last time you read the sensor 
    previousMillis = currentMillis;   
 
    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    humidity = dht.readHumidity();          // Read humidity (percent)
    temp_f = dht.readTemperature(false);     // Read temperature as Fahrenheit
    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temp_f)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
   
  }
   TurnInfoLedOff();
}
