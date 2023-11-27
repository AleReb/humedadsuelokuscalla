#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <SoftwareSerial.h>
#include <Wire.h>
#include "RTClib.h"
#include <U8g2lib.h>
#include <Adafruit_NeoPixel.h>
//////////////////////////////////////////////////////////////////////////////////////////////////////
//rtc

RTC_DS3231 myRTCB;

String fecha;
String hora;
////pantalla
///////////////////////////////////////////////////////////////////////////////////////////////////////
#include <U8g2lib.h>
//U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);  //chica
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);  //mas grande

#define RE 33
#define SCK 2
#define MISO 15
#define MOSI 13
#define CS 12

//#define DE D6
String myString;
String temp;

int id = 0;

byte values[20];
byte values2[20];
//byte values3[20];

SPIClass spi = SPIClass(VSPI);

// Definición global
EspSoftwareSerial::UART mod;  // Reemplaza mods[1] por mod

float val1;  // Valor del sensor
String val1S;
const byte sensorRequest[] = { 0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B };
bool isSensorConnected = true;
int failedAttempts = 0;
const int MAX_ATTEMPTS = 10;
bool sdin = true;
bool rtcBool = true;

///////////////////////////// led
#define PIN 0        //PIN DEL LED RGB
#define NUMPIXELS 1  // Popular NeoPixel ring size
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
//////////////////////////////////////////////////// bateria
const int batpin = 34;
int bat = 0;
float voltageBattery = 0;
////////////////////////////////////////////// first run
String conex = "0";
int sensorIndex;
bool primerloop = true;
//////////////////rebot
unsigned long lastRebootMillis = 0;
const unsigned long rebootInterval = 4 * 60 * 60 * 1000;  // 4 horas en milisegundos

void checkReboot() {
  if (millis() - lastRebootMillis > rebootInterval) {
    lastRebootMillis = millis();
    // Código para reiniciar el ESP32
    ESP.restart();
  }
}
///////////////////////////////////////////SD tiempo de guardado
unsigned long lastSaveMillis = 0;
const unsigned long saveInterval = 3 * 60 * 1000;  //5 * 60 * 1000; 5 minutos en milisegundos

void checkSaveData() {
  if (millis() - lastSaveMillis > saveInterval) {
    lastSaveMillis = millis();
    saveData();  // Tu función personalizada para guardar datos
  }
}

void saveData() {
  if (sdin == true) {
    ROUTINE_REGISTRY();
  }  // Implementa tu lógica de guardado de datos aquí
}
//////////////////////////////////////////////////////reintento de sensor en caso de error
unsigned long lastSensorRetryMillis = 0;
const unsigned long sensorRetryInterval = 60 * 1000;  // 1 minuto en milisegundos

void checkSensorRetry() {
  if (millis() - lastSensorRetryMillis > sensorRetryInterval) {
    lastSensorRetryMillis = millis();
    isSensorConnected = true;  // reintentara leer
  }
}

void setup() {
  Wire.begin();
  Serial.begin(115200);
  spi.begin(SCK, MISO, MOSI, CS);
  // si esta montada la memoria
  if (!SD.begin(CS, spi, 80000000)) {
    //Serial.println("Card Mount Failed");
    sdin = false;
  } else {
   // Serial.println("SD Mount OK");
  }

  mod.begin(4800, SWSERIAL_8N1, 18, 19, false);

  pinMode(RE, OUTPUT);

  if (!myRTCB.begin()) {
  //  Serial.println("Couldn't find RTC");
    Serial.flush();
    rtcBool = false;
  }

  u8g2.begin();
  u8g2.clearBuffer();                  // clear the internal memory
  u8g2.setFont(u8g2_font_crox4hb_tr);  // choose a suitable font grande
  u8g2.setFont(u8g2_font_t0_11_tf);
  u8g2.drawStr(0, 10, "HumSueloV1");
  if (sdin == true) {
    u8g2.drawStr(0, 40, "SD: OK");
  } else {
    u8g2.drawStr(0, 40, "SD: NO");
  }
  if (rtcBool == true) {
    u8g2.drawStr(0, 50, "RTC: OK");
  } else {
    u8g2.drawStr(0, 50, "RTC: NO");
  }
  u8g2.sendBuffer();
  pixels.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.setPixelColor(0, pixels.Color(255, 255, 0));
  pixels.show();
  delay(2000);
}

void loop() {
  if (sdin == true && rtcBool == true && conex == "1" && isSensorConnected == true) {
    pixels.setPixelColor(0, pixels.Color(0, 255, 0));
    pixels.show();
  } else if (sdin == true && rtcBool == true && isSensorConnected == true) {
    pixels.setPixelColor(0, pixels.Color(0, 0, 255));
    pixels.show();
  } 
    else if (sdin == true && rtcBool == true && val1S == "E") {
   pixels.setPixelColor(0, pixels.Color(255, 165, 0));
    pixels.show();
  }
  else if (sdin == true && rtcBool == true) {
    pixels.setPixelColor(0, pixels.Color(255, 0, 255));
    pixels.show();
  }
   
  if (primerloop == true) {
    if (SD.exists("/ActiveSensor_DB.csv") == false) {
     // Serial.println("Archivo base no existe...");
      delay(500);
   //   Serial.println("Archivo creado");
      String tosave = "sessionID,Hum1,temp,bat,online,fecha,hora \n";
      writeFile(SD, "/ActiveSensor_DB.csv", tosave.c_str());
    } else {
      //Serial.println("Archivo base encontrado... abriendo");
    }
    readFile(SD, "/ActiveSensor_DB.csv");  // GET LAST ID
    id = id + 1;
    primerloop = false;
  }

  if (Serial.available() > 0) {
    char receivedChar = Serial.read();  // Lee el carácter entrante
    if (receivedChar == '0' || receivedChar == '1') {
      conex = receivedChar - '0';  // Convierte el carácter a int (0 o 1)
    }
  }
if(rtcBool == true) {
  DateTime now = myRTCB.now();
  fecha = getFecha(now);
  hora = getHora(now);
  fecha = getFecha(now);
  hora = getHora(now);
  temp = String(myRTCB.getTemperature());
}else{
  fecha = "N";
  hora = "N";
  fecha = "N";
  hora = "N";
  temp = "N";
}
  // Reading potentiometer value
  bat = analogRead(batpin);
  float batRaw = (bat * 3.3) / (4095);
  float calibrationFactor = 1.05;  // Ajusta este valor según sea necesario
  voltageBattery = ((bat / 4095.0) * 3.3 * (15000.0 + 3300.0) / 3300.0) * calibrationFactor;
  // Serial.println();
  // Serial.println("bat:" + String(bat) + " real voltaje: " + String(batRaw) + " convertido voltaje: " + String(voltageBattery));
  // Serial.println();
  sendSerialMessage();
  
  u8g2.clearBuffer();  // clear the internal memory
  u8g2.setCursor(0, 10);
  u8g2.print("F/HORA VB:" + String(voltageBattery));
  u8g2.setCursor(0, 20);
  u8g2.print(String(fecha) + " Temp:" + String(temp));
  u8g2.setCursor(0, 30);
  u8g2.print(String(hora) + " conex:" + String(conex));
  u8g2.setCursor(0, 40);
  u8g2.print("h1: " + val1S + " senok:" + String(isSensorConnected));

  checkSensor();

  checkSensorRetry();
  checkReboot();
  checkSaveData();
  u8g2.sendBuffer();
}

void data(byte* values) {
  digitalWrite(RE, HIGH);
  delay(100);
  if (mod.write(sensorRequest, sizeof(sensorRequest)) == 8) {
    digitalWrite(RE, LOW);
    for (byte i = 0; i < 9; i++) {
      values[i] = mod.read();
    }
  }
}

void checkSensor() {
  if (!isSensorConnected) {
    return;
  }

  byte values[9];
  data(values);

  val1 = ((values[3] * 256) + values[4]) * 0.1;
  val1S = String(val1);
  if (val1 >= 0 && val1 < 6553) {
    failedAttempts = 0;
  } else {
    val1S = "E";
    failedAttempts++;

    if (failedAttempts >= MAX_ATTEMPTS) {
      isSensorConnected = false;
      val1S = "N";
    } else {
      delay(1000);
    }
  }
}

void sendSerialMessage() {
  String message = String(voltageBattery) + "," + String(val1S) + "," +
                   String(temp) + "," + (sdin ? "1" : "0") + "," +
                   fecha + "," + hora;

  // Añade un delimitador de inicio y una suma de verificación al final
  unsigned char checksum = calculateChecksum(message);
  message = "<" + message + "," + String(checksum) + ">\n";

  Serial.print(message);
  delay(100);
}

unsigned char calculateChecksum(const String& data) {
  unsigned char checksum = 0;
  for (int i = 0; i < data.length(); i++) {
    checksum ^= data[i];
  }
  return checksum;
}