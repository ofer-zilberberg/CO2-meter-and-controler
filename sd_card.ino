// Set the pins used
#define cardSelect 10 // 15 for huzzah
File logfile;

bool setup_sd() {


  // see if the card is present and can be initialized:
  if (!SD.begin(cardSelect)) {
    Serial.println("Card init. failed!");
    return false;
  }
//  Serial.println("SD Ready!");
  return true;
}


void logTo_sd(float batV_value, float co2_value, float temp, float humidity) {

  DateTime now = rtc.now();

  String dataString = "";

  dataString += String(now.year(), DEC);
  dataString += '/';
  dataString += String(now.month(), DEC);
  dataString += '/';
  dataString += String(now.day(), DEC);
  dataString += ",";
  dataString += daysOfTheWeek[now.dayOfTheWeek()];
  dataString += ",";
  dataString += String(now.hour(), DEC);
  dataString += ':';
  dataString += String(now.minute(), DEC);
  dataString += ':';
  dataString += String(now.second(), DEC);
  //  dataString += '\n';

  dataString += ",";
  dataString += String(co2_value, 2);				// String(val, decimalPlaces)
  dataString += ",";
  dataString += String(temp, 2);
  dataString += ",";
  dataString += String(humidity, 2);
  dataString += ",";
  dataString += String(batV_value,2);
//  dataString += '\n';								// new line
//  dataString += '\r';								// carriage return

  //  Serial.println(dataString);

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  //  File dataFile = SD.open("datalog.TXT", FILE_WRITE);

  //  todayString += ".TXT";


  //  char filename[] = "DATA-2021-01-22.TXT";
  //  char filename[] = "20210122.txt"
  char filename[] = "00000000.txt";;
  sprintf(filename, "%02d%02d%02d.txt", now.year(), now.month(), now.day());

  if (SD.exists(filename) == false) {
    Serial.print("exists == false ");
    Serial.println(filename);
  }
  SdFile::dateTimeCallback(dateTime);				// SETS FAT system TO SAVE TIME AND DATE INTO ANY FILE YOU CREATE.
  File dataFile = SD.open(filename, FILE_WRITE);
  // File dataFile = SD.open("myData.txt", FILE_WRITE);


  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
#ifdef RS232_Logging
    Serial.print("FileName: ");
    Serial.print(filename);
    Serial.print(" , Write Data: ");
    Serial.print(dataString);
    Serial.print(" , millis: ");
    Serial.println(millis());
#endif
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.print("error opening file: ");
    Serial.println(filename);
  }

  delay(100);
}
