/////////////////////////////////////////////
//                                         //
//     Nixie Tube Temperature Display      //
//              Version 1.0                //
//     ------- Austin Gilbert --------     //
//                                         //
//        :      :      :       :          //
// '.    _.__   _.__   _.__    _.__    .'  //
//    ' |    | |    | |    |  |    | '     //
//      |--- | |/-\ | | /| |  ||-- |       //
//  --= |  / | |  / | |/_| |  ||-- | =--   //
//    . | /  | |/__*| |  | |  ||   | .     //
//  .'  |====| |====| |====|  |====|   '.  // 
//      -||||- -||||- -||||-  -||||-       //
//                                         //
/////////////////////////////////////////////

#include <Arduino.h>
#include <Wire.h>
#include <DHT.h>
#include <SPI.h>
#include <Arduino_JSON.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include <cmath>

// WiFi
const char* ssid = "MySpectrumWiFi2C-2G";
const char* password = "mastergoal312";

// OpenWeatherMap API
// String latitude = "47.661840";
// String longitude = "-122.308690";
String latitude = "34.178560";
String longitude = "-118.769340";
String units = "imperial";
String apiKey = "c38c04c4dfbd1f8a5710153012af6ae2";
String jsonBuffer;

// Data Retrieval Timer
unsigned long lastTime = 0;
unsigned long timerDelay = 10000; // 3 minutes
bool firstTime = true;

// K155ID1 Decoder Pins
#define A1 2 // Tube 1
#define B1 4
#define C1 5
#define D1 12
#define A2 14 // Tube 2
#define B2 15
#define C2 16
#define D2 17
#define A3 18 // Tube 3
#define B3 19
#define C3 21
#define D3 22
#define A4 23 // Tube 4 (& 2)
#define B4 25
#define C4 26
#define D4 27
#define dec 32
#define temp 13
char A[4] = {A1, A2, A3, A4};
char B[4] = {B1, B2, B3, B4};
char C[4] = {C1, C2, C3, C4};
char D[4] = {D1, D2, D3, D4};

// DHT Temperature Sensor
DHT dht(temp, DHT22);

// Other Variables
bool cycleBool = true;
int fSymbol = 1;

void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  dht.begin();

  // Connecting to WiFi
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  // Connecting to Sensor
  float t = dht.readTemperature(true);
  while (isnan(t)&& millis() < 5000) {
    Serial.println(F("Failed to read form DHT sensor!"));
  }
  Serial.print("Temperature");
  Serial.print(t);
  Serial.println(F("°F"));
  
  // Set Inputs and Outputs
  pinMode(A1, OUTPUT);
  pinMode(B1, OUTPUT);
  pinMode(C1, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(B2, OUTPUT);
  pinMode(C2, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(B3, OUTPUT);
  pinMode(C3, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(A4, OUTPUT);
  pinMode(B4, OUTPUT);
  pinMode(C4, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(dec, OUTPUT);
  pinMode(temp, INPUT_PULLUP);

  // Apply Voltages
  for (char i = 0; i < 4; i++) {
    digitalWrite(A[i], HIGH);
    digitalWrite(B[i], HIGH);
    digitalWrite(C[i], HIGH);
    digitalWrite(D[i], HIGH);
  }
  
//  digitalWrite(dec, HIGH);
//  decimal(false);
//  digitalWrite(temp, LOW);
}

void loop() {
  if ((millis() - lastTime) > timerDelay || firstTime) {
    if(WiFi.status() == WL_CONNECTED) {
      String serverPath = "https://api.openweathermap.org/data/2.5/onecall?lat=" + latitude + "&lon=" + longitude + "&exclude=hourly&appid=" + apiKey + "&units=" + units;

      jsonBuffer = httpGETRequest(serverPath.c_str());
      JSONVar myObject = JSON.parse(jsonBuffer);

      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }
      
      double onlineTempDouble = myObject["current"]["temp"];
      float onlineTemp = (float) onlineTempDouble;
      
      // Cycles Digits to prevent Cathode Poisoning
      if (firstTime) {
        cycle();
        firstTime = false;
      }
      
      writevalue(onlineTemp);
      
      Serial.print("Online Temperature: ");
      Serial.print(myObject["current"]["temp"]);
      Serial.println(F("°F"));
    } else {
      Serial.println("WiFi Disconnected");
    }

    float sensorTemp = dht.readTemperature(true);
    if (isnan(sensorTemp)) {
      Serial.println(F("Failed to read from DHT sensor!"));
    } else {
      Serial.print("Sensor Temperature: ");
      Serial.print(sensorTemp);
      Serial.println(F("°F"));
    }
    lastTime = millis();
  }
}

void writevalue(float value) {
  int digits {value * 10};
  if (digits < 1000) {
    int digit1 = digits / 100;
    int digit2 = digits / 10 % 10;
    int digit3 = digits % 10;
    writenumber(0, digit1);
    writenumber(1, digit2);
//    decimal(true);
    writenumber(2, digit3);
    writenumber(3, fSymbol);
  } else {
    digits = round(value);
    int digit1 = digits / 100;
    int digit2 = digits / 10 % 10;
    int digit3 = digits % 10;
    writenumber(0, digit1);
    writenumber(1, digit2);
    
    digitalWrite(dec, HIGH);
//    decimal(false);
    writenumber(2, digit3);
    writenumber(3, fSymbol);
  }
}

String httpGETRequest(const char* serverName) {
  HTTPClient http;
    
  // Your IP address with path or Domain name with URL path 
  http.begin(serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}

void writenumber(int a, int b) {
  switch (b) {
    case 0:
      digitalWrite(A[a], LOW);
      digitalWrite(B[a], LOW);
      digitalWrite(C[a], LOW);
      digitalWrite(D[a], LOW);
      break;
    case 1:
      digitalWrite(A[a], HIGH);
      digitalWrite(B[a], LOW);
      digitalWrite(C[a], LOW);
      digitalWrite(D[a], LOW);
      break;
    case 2:
      digitalWrite(A[a], LOW);
      digitalWrite(B[a], HIGH);
      digitalWrite(C[a], LOW);
      digitalWrite(D[a], LOW);
      break;
    case 3:
      digitalWrite(A[a], HIGH);
      digitalWrite(B[a], HIGH);
      digitalWrite(C[a], LOW);
      digitalWrite(D[a], LOW);
      break;
    case 4:
      digitalWrite(A[a], LOW);
      digitalWrite(B[a], LOW);
      digitalWrite(C[a], HIGH);
      digitalWrite(D[a], LOW);
      break;
    case 5:
      digitalWrite(A[a], HIGH);
      digitalWrite(B[a], LOW);
      digitalWrite(C[a], HIGH);
      digitalWrite(D[a], LOW);
      break;
    case 6:
      digitalWrite(A[a], LOW);
      digitalWrite(B[a], HIGH);
      digitalWrite(C[a], HIGH);
      digitalWrite(D[a], LOW);
      break;
    case 7:
      digitalWrite(A[a], HIGH);
      digitalWrite(B[a], HIGH);
      digitalWrite(C[a], HIGH);
      digitalWrite(D[a], LOW);
      break;
    case 8:
      digitalWrite(A[a], LOW);
      digitalWrite(B[a], LOW);
      digitalWrite(C[a], LOW);
      digitalWrite(D[a], HIGH);
      break;
    case 9:
      digitalWrite(A[a], HIGH);
      digitalWrite(B[a], LOW);
      digitalWrite(C[a], LOW);
      digitalWrite(D[a], HIGH);
      break;
  }
}

//void decimal(bool active) {
//  if (active = true) {
//    digitalWrite(dec, HIGH);
//  } else {
//    digitalWrite(dec, LOW);
//  }
//}

void cycle() {
  int starttime = millis();
  int endtime = starttime;
  while ((endtime - starttime) <=5000) { // do this loop for 5 seconds
    for (int i = 0; i < 10; i++) {
      for (int m = 0; m < 4; m++){
        writenumber(m, i);
      }
      delay(20);
    }
    endtime = millis();
  }
}

void off(int a) {
  digitalWrite(A[a], HIGH);
  digitalWrite(B[a], HIGH);
  digitalWrite(C[a], HIGH);
  digitalWrite(D[a], HIGH);
}
