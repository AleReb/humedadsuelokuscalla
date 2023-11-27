///Pestaña nueva
void readFile(fs::FS &fs, const char *path) {
 // Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
   // Serial.println("Failed to open file for reading");
    return;
  }

 // Serial.print("Read from file: ");
  while (file.available()) {
    myString = file.readStringUntil('\n');
  }

  id = getValor(myString, ',', 0).toInt();
  //Serial.println(id);
  file.close();
}

String getValor(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
void writeFile(fs::FS &fs, const char *path, const char *message) {
  // Serial.printf("Writing file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);// File file = fs.open(path, FILE_WRITE); 
  if (!file) {
    u8g2.setCursor(0, 50);
    u8g2.print("error sd");
    u8g2.sendBuffer();
   // Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    u8g2.setCursor(0, 50);
    u8g2.print("File ok");
    u8g2.sendBuffer();
    //  Serial.println("File written");
  } else {
    u8g2.setCursor(0, 60);
    u8g2.print("error sd");
    u8g2.sendBuffer();
    //  Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  //Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    //u8g2.clearBuffer();  // clear the internal memory
    u8g2.setCursor(0, 60);
    u8g2.print("error sd");
    u8g2.sendBuffer();
    //  Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    //  Serial.println("Message appended");
    u8g2.setCursor(0, 60);
    u8g2.print("sd SAVE");
    u8g2.sendBuffer();
  } else {
    u8g2.setCursor(0, 60);
    u8g2.print("error sd");
    u8g2.sendBuffer();
    //Serial.println("Append failed");
  }
  file.close();
}

void ROUTINE_REGISTRY() {
  String tosave = String(id) + "," + val1S + "," + temp + "," + String(voltageBattery)  + "," + conex + "," + fecha + "," + hora + "\n";
  appendFile(SD, "/ActiveSensor_DB.csv", tosave.c_str());
  //  writeFile(SD, "/ActiveSensor.csv", tosave.c_str()); //solo escrive la ultima vez
}

String getFecha(DateTime now) {
  String fecha = String(now.day(), DEC) + "/";
  fecha += String(now.month(), DEC) + "/";
  fecha += String(now.year(), DEC);
  return fecha;
}

String getHora(DateTime now) {
  String hora = String(now.hour(), DEC) + ":";
  hora += String(now.minute(), DEC) + ":";
  hora += String(now.second(), DEC);
  return hora;
}
