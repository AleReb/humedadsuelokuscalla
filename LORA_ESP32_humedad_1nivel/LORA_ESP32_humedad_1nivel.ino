/*
   pinout especial del paxcounter y usa otros pines para conectarse

  TTGO LoRa32 V2.1.6:
  ESP32          LoRa (SPI)  SDCARD(SPI)    Display (I2C)  LED           BAT
  -----------    ----------  -----------    -------------  ------------------
  GPIO5  SCK     SCK
  GPIO27 MOSI    MOSI
  GPIO19 MISO    MISO
  GPIO18 SS      NSS
  GPIO23         RST
  GPIO26         DIO0
  GPIO33         DIO1
  GPIO32         DIO2
  GPIO21 SDA                     SDA
  GPIO22 SCL                     SCL
  GPIO25                                                 LED COMO SE USA UN NEOPIXEL NO SE USA EL LED DEL ESP
  GPIO35                                                 LECTURA 100K BAT
  GPIO34                                                 LECTURA panel solar 12k / 3.3k
  GPIO14                 SD_CLK
  GPIO2                  SD_MISO
  GPIO15                 SD_MOSI
  GPIO13                 SD_CS
  GPIO12                 DAT2 // no se usa
  GPIO4                  DAT1 // no se usa con la libreria mySD



*/
//  SPI port #2:  SD Card Adapter
#define SD_CLK 14
#define SD_MISO 02
#define SD_MOSI 15
#define SD_CS 13
#define LoRa_CS 18
#define Select LOW     //  Low CS means that SPI device Selected
#define DeSelect HIGH  //  High CS means that SPI device Deselected

#include <mySD.h>
#include <SPI.h>
#include <lmic.h>
#include <hal/hal.h>


#include <U8x8lib.h>

#define DISABLE_PING 0
#define DISABLE_BEACONS 0

#include <Wire.h>

#include <HardwareSerial.h>
HardwareSerial mySerial2(1);

float volsolar;  //================================================= solar
String inData;
String mensaje;
boolean sd = true;
boolean err = false;

//float volsolar = 0.0; // Voltaje
String hum = "";   // Humedad
float temp = 0.0;  // Temperatura
int sdState = 0;   // Estado de la SD
String fecha = "";
String hora = "";
unsigned int counter2 = 0;

#include <Adafruit_NeoPixel.h>
#define PIN 25       // On Trinket or Gemma, suggest changing this to 1
#define NUMPIXELS 1  // Popular NeoPixel ring size
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

RTC_DATA_ATTR char filenameCSV[25] = "";  //para construir el archivo csv base cada vez que se prenda y guarde
//Tiempo para debugear
unsigned long previousMillis = 0;  // Almacena la última vez que se leyeron los datos
const long interval = 3000;        // Intervalo en milisegundos (3 segundos)



static const u1_t PROGMEM APPEUI[8] = { 0x82, 0xE5, 0x3A, 0xC9, 0x88, 0x3F, 0x35, 0x6E };  //INSTRUCCION: esbribir inversamente (lsb) entre las llaves el APPEUI o JOINEUI creado en la plataforma
void os_getArtEui(u1_t* buf) {
  memcpy_P(buf, APPEUI, 8);
}
static const u1_t PROGMEM DEVEUI[8] = { 0x01, 0x1B, 0x55, 0x0C, 0xA3, 0x9C, 0xB8, 0xCA };  //INSTRUCCION: esbribir inversamente (lsb) entre las llaves el DevEUI creado en la plataforma
void os_getDevEui(u1_t* buf) {
  memcpy_P(buf, DEVEUI, 8);
}
static const u1_t PROGMEM APPKEY[16] = { 0x59, 0xE1, 0xBA, 0x78, 0x7D, 0x17, 0xEB, 0x1E, 0x66, 0xB9, 0x34, 0x90, 0x97, 0x03, 0x9E, 0x7E };  //INSTRUCCION: esbribir tal cual (msb) entre las llaves el AppKey creado en la plataforma
void os_getDevKey(u1_t* buf) {
  memcpy_P(buf, APPKEY, 16);
}

ext::File root;
ext::File dataFile;
ext::File myFile;

//For TTGO LoRa32 V2.x use:
U8X8_SSD1306_128X64_NONAME_HW_I2C display_(/*rst*/ U8X8_PIN_NONE);

static uint8_t mydata[55];
static int counter;

static osjob_t sendjob;
int runs = 0;
const unsigned TX_INTERVAL = 120;
// For TTGO LoRa32 V2.1.6
const lmic_pinmap lmic_pins = {
  .nss = LoRa_CS,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 23,
  .dio = { /*dio0*/ 26, /*dio1*/ 33, /*dio2*/ 32 }
};


void printHex2(unsigned v) {
  v &= 0xff;
  if (v < 16)
    Serial.print('0');
  Serial.print(v, HEX);
}

void onEvent(ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev) {
    case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;

    case EV_JOINED:
      Serial.println(F("EV_JOINED"));
      {
        u4_t netid = 0;
        devaddr_t devaddr = 0;
        u1_t nwkKey[16];
        u1_t artKey[16];
        LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
        Serial.print("netid: ");
        Serial.println(netid, DEC);
        Serial.print("devaddr: ");
        Serial.println(devaddr, HEX);
        Serial.print("AppSKey: ");
        for (size_t i = 0; i < sizeof(artKey); ++i) {
          if (i != 0)
            Serial.print(" ");
          printHex2(artKey[i]);
        }
        Serial.println("");
        Serial.print("NwkSKey: ");
        for (size_t i = 0; i < sizeof(nwkKey); ++i) {
          if (i != 0)
            Serial.print(" ");
          printHex2(nwkKey[i]);
        }
        Serial.println();
        display_.drawString(0, 5, "Network Joined");
      }


      initSD();
      // Disable link check validation (automatically enabled
      // during join, but because slow data rates change max TX
      // size, we don't use it in this example.
      LMIC_setLinkCheckMode(0);
      break;
    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));

      break;
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
      break;
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      display_.drawString(0, 3, "Data Sent");
      WriteReadSD();
      if (LMIC.txrxFlags & TXRX_ACK)
        Serial.println(F("Received ack"));
      if (LMIC.dataLen) {
        Serial.print(F("Received "));
        Serial.print(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));

        display_.drawString(0, 4, "Data received");
        Serial.println(F("Data is "));

        // Change the following codes to process incoming data !!
        for (int counter = 0; counter < LMIC.dataLen; counter++) {
          Serial.print(LMIC.frame[LMIC.dataBeg + counter], HEX);
        }
        Serial.println(F(" "));

      } else
        display_.drawString(0, 4, "        ");
      // Schedule next transmission
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      break;
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
    case EV_TXSTART:
      Serial.println(F("EV_TXSTART"));
      break;
    case EV_TXCANCELED:
      Serial.println(F("EV_TXCANCELED"));
      break;
    case EV_JOIN_TXCOMPLETE:
      Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      pixels.show();  // Send the updated pixel colors to the hardware.
      delay(250);
      pixels.setPixelColor(0, pixels.Color(255, 0, 255));
      pixels.show();  // Send the updated pixel colors to the hardware.
      delay(250);
      mySerial2.println(0);
      break;

    default:
      Serial.print(F("Unknown event: "));
      Serial.println((unsigned)ev);
      break;
  }
}
void do_send(osjob_t* j) {
  display_.clear();
  display_.drawString(0, 0, "udd test");  ///////////////////////////////////////// Cambia según tu dispositivo
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else {

    ++counter;
    mydata[0] = 0;  //este es el mensaje completo
    mensaje = "";
    if (err == false) {
      mensaje = mensaje + String(counter) + "," + String(volsolar) + "," + hum + "," + String(temp) + "," + String(sdState);
    }
    if (err == true) {
      mensaje = mensaje + String(counter) + "," + String(volsolar) + ",err";
    }

    Serial.println(mensaje);
    int str_lenM = mensaje.length() + 1;  // esta funcion es para asignarle el valor de casillas al array
    char menC[str_lenM];                  //este es el char array
    mensaje.toCharArray(menC, str_lenM);
    Serial.println(menC);
    strcat((char*)mydata, menC);  //copio el mensaje   ‘\0’ average

    // Prepare upstream data transmission at the next possible time. strlen((char*)mydata), 0)
    //LMIC_setTxData2(1, mydata, sizeof(mydata) + 1, 0);
    LMIC_setTxData2(1, mydata, sizeof(mydata) + 1, 0);
    Serial.println(F("Packet queued"));
    display_.drawString(0, 2, "Packet Queued");
    pixels.setPixelColor(0, pixels.Color(0, 0, 255));
    pixels.show();  // Send the updated pixel colors to the hardware.
    mySerial2.println(1);
    delay(1000);  // Pause before next pass through loop
  }
  pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  pixels.show();  // Send the updated pixel colors to the hardware.
}

void setup() {
  Serial.begin(115200);
  mySerial2.begin(115200, SERIAL_8N1, 4, 0);
  pixels.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)

  uint32_t currentFrequency;

  Serial.println("Test esp32 RM95");


  display_.begin();
  display_.setFont(u8x8_font_pxplusibmcgathin_r);
  display_.drawString(0, 0, "TTGO ESP32 LoRa Test");
  Serial.println(F("Module Configured for CHILE BAND (AU915 MHz)"));


  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
  LMIC_setClockError(MAX_CLOCK_ERROR * 5 / 100);
  //LMIC_setClockError(MAX_CLOCK_ERROR * 10 / 100);
  // Reset the MAC state. Session and pending data transfers will be discarded.
  // LMIC_reset();

  LMIC_setLinkCheckMode(0);
  LMIC_setAdrMode(false);
  LMIC_setDrTxpow(DR_SF7, 14);
  LMIC_enableSubBand(1);
  LMIC_selectSubBand(1);


  do_send(&sendjob);
  // set output pins
  pinMode(SD_CS, OUTPUT);
  pinMode(LoRa_CS, OUTPUT);
}
//================================================================ tiempo
unsigned long lastmillisrest = 0;
unsigned long previousMillisrest = 0;  // will store last time LED was updated
const long intervalrest = 3600000;     //una hora


/////////////==============================================================
void loop() {
  unsigned long currentMillisrest = millis();
  if (currentMillisrest - previousMillisrest >= intervalrest) {
    // save the last time you blinked the LED
    previousMillisrest = currentMillisrest;
    Serial.println("Restarting in 10 seconds");
    ESP.restart();
  }
  pixels.clear();          // Set all pixel colors to 'off'
  receiveSerialMessage();  // Llama continuamente a la función de recepción

  unsigned long currentMillis = millis();
  // Comprueba si ha pasado el intervalo
  if (currentMillis - previousMillis >= interval) {
    // Guarda el tiempo actual como el último en el que se leyeron los datos
    previousMillis = currentMillis;
    // Imprimir cada variable
    Serial.println("Voltaje Solar: " + String(volsolar));
    Serial.println("Humedad: " + hum);
    Serial.println("Temperatura: " + String(temp));
    Serial.println("Estado SD: " + String(sdState));
    Serial.println("Fecha: " + fecha);
    Serial.println("Hora: " + hora);
  }

  display_.setCursor(0, 1);
  display_.print("temp: " + String(temp) + "C");
  display_.setCursor(0, 4);
  display_.print("humS " + hum + "%");
  display_.setCursor(0, 6);
  display_.print("B:" + String(volsolar) + " SD:" + String(sdState));
  display_.setCursor(0, 7);
  display_.print(hora);


  os_runloop_once();
}
void WriteReadSD() {

  static uint32_t debs;
  if (millis() - debs >= 3000 && sd == true) {
    debs = millis();
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.

    digitalWrite(LoRa_CS, DeSelect);
    digitalWrite(SD_CS, Select);  //  SELECT (Low) SD Card SPI
    myFile = SD.open(filenameCSV, FILE_WRITE);

    // if the file opened okay, write to it:
    if (myFile) {
      Serial.print("Writing to " + String(filenameCSV));
      myFile.println(mensaje);
      // close the file:
      myFile.close();
      Serial.println("done.");
    } else {
      // if the file didn't open, print an error:
      Serial.println("error opening " + String(filenameCSV));
    }

    // re-open the file for reading:
    myFile = SD.open(filenameCSV);
    if (myFile) {
      Serial.println(filenameCSV);

      // read from the file until there's nothing else in it:
      while (myFile.available()) {
        Serial.write(myFile.read());
      }
      // close the file:
      myFile.close();
    } else {
      // if the file didn't open, print an error:
      Serial.println("error opening " + String(filenameCSV));
    }
    digitalWrite(SD_CS, DeSelect);
    delay(1000);
    digitalWrite(LoRa_CS, Select);  //  SELECT (low) LoRa SPI
  }
}
void initSD() {
  Serial.print("Initializing SD card...");
  digitalWrite(LoRa_CS, DeSelect);
  digitalWrite(SD_CS, Select);  //  SELECT (Low) SD Card SPI
  delay(100);
  /**/
  Serial.print("Initializing SD card...");
  pinMode(SD_CS, OUTPUT);  //  SELECT (Low) SD Card SPI
  /**/
  if (!SD.begin(SD_CS, SD_MOSI, SD_MISO, SD_CLK)) {
    Serial.println("initialization failed!");
    sd = false;
    return;
  }
  Serial.println("initialization done.");
  int n = 0;
  snprintf(filenameCSV, sizeof(filenameCSV), "data%03d.csv", n);  // includes a three-digit sequence number in the file name
  while (SD.exists(filenameCSV)) {
    n++;
    snprintf(filenameCSV, sizeof(filenameCSV), "data%03d.csv", n);
  }
  dataFile = SD.open(filenameCSV, FILE_READ);
  Serial.println(n);
  Serial.println(filenameCSV);
  dataFile.close();

  root = SD.open("/");

  printDirectory(root, 0);

  Serial.println(" done!");

  digitalWrite(SD_CS, DeSelect);
  delay(1000);
  digitalWrite(LoRa_CS, Select);  //  SELECT (low) LoRa SPI
}

void printDirectory(ext::File dir, int numTabs) {

  while (true) {
    ext::File entry = dir.openNextFile();
    if (!entry) {
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');  // we'll have a nice indentation
    }
    // Print the name
    Serial.print(entry.name());
    /* Recurse for directories, otherwise print the file size */
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      /* files have sizes, directories do not */
      Serial.print("\t\t");
      Serial.println(entry.size());
    }
    entry.close();
  }
}



void receiveSerialMessage() {
  while (mySerial2.available() > 0) {
    char received = mySerial2.read();
    if (received == '<') {  // Inicio del mensaje
      inData = "";
    } else if (received == '>') {  // Fin del mensaje
      processMessage(inData);
      inData = "";  // Limpia inData para el próximo mensaje
    } else {
      inData += received;
    }
  }
}


void processMessage(String message) {
  int lastCommaIndex = message.lastIndexOf(',');
  String checksumStr = message.substring(lastCommaIndex + 1);
  message = message.substring(0, lastCommaIndex);

  if (calculateChecksum(message) == (unsigned char)checksumStr.toInt()) {
    // El checksum es correcto, procesar el mensaje
    int index = 0;
    int commaIndex = message.indexOf(',');
    while (commaIndex != -1) {
      String value = message.substring(0, commaIndex);
      message = message.substring(commaIndex + 1);

      switch (index) {
        case 0:
          volsolar = value.toFloat();
          break;
        case 1:
          hum = value;
          break;
        case 2:
          temp = value.toFloat();
          break;
        case 3:
          sdState = value.toInt();
          break;
        case 4:
          fecha = value;
          break;
        case 5:
          hora = value;
          break;
      }
      index++;
      commaIndex = message.indexOf(',');
    }
    // No hay más comas, procesar el último valor
    if (index == 5) {
      hora = message;
    }
  } else {
    // Error en el checksum
    Serial.println("Error de checksum");
  }
}

unsigned char calculateChecksum(const String& data) {
  unsigned char checksum = 0;
  for (int i = 0; i < data.length(); i++) {
    checksum ^= data[i];
  }
  return checksum;
}
