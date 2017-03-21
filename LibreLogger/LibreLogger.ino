//LibreLogger Version 1
//Under the GNU Public License, Version 3

//All Libraries provided are credit to their owners.
//They were all available from GitHub, and their legal usage was assumed.
//If there is an issue with this, contact me at hamwater@gmail.com

//humidity sensor libraries
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_Sensor.h>

//RTC libraries
#include <TimeLib.h>
#include <DS3232RTC.h>
#include <Wire.h>

//webserver libraries
#include <SPI.h>
#include <Ethernet.h>

//sd card Library
#include <SD.h>

//LCD Library
#include <LiquidCrystal.h>

//declare humidity sensor
#define DHTPIN 2
#define DHTTYPE DHT11

//Using display in 8-Bit mode
//Change for your specifc setup if needed. Avoid using Pins 4 and 10, as they are for the Ethernet Shield.

// LCD RS pin to digital pin 5
// LCD Enable pin to digital pin 3
// LCD D4 pin to digital pin 9
// LCD D5 pin to digital pin 8
// LCD D6 pin to digital pin 7
// LCD D7 pin to digital pin 6
LiquidCrystal lcd(5, 3, 9, 8, 7, 6);

//start webserver

//Change this to the MAC Address of your specific shield.
//This is typically found on the bottom of your W5100 shield.
byte mac[]={
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

//change for your local network.
IPAddress ip(172, 16, 1, 177);

EthernetServer server(80);

//This creates the DHT. Change from pins above if needed
DHT dht(DHTPIN, DHTTYPE);

int chipSelect = 4;

//Stores the temperature as an integer.
int temp;
int tempC;

//counts how many loops the machine has been through.
long cycles = 0;

//counts the amout of requests/refreshes that the webserver has gotten.
long clients = 0;

void setup() {

  //lcd start
  lcd.begin(16, 2);
  lcd.print("LibreLogger");
  lcd.setCursor(0, 1);
  lcd.print("V 1.0 3/21/2017");
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  /*
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  */
  Serial.print("Initializing memory card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("card access failed.");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  setSyncProvider(RTC.get);

}

void printDigitsLCD(int digits){
  if(digits < 10){
    lcd.print('0');
  }
    lcd.print(digits);
  }

void loop() {
  // put your main code here, to run repeatedly:
      temp = (int) dht.readTemperature(true);
      tempC = (int) dht.readTemperature();
      float humidity = dht.readHumidity();
      delay(499);
      //logic for lcd display
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print(temp);
      lcd.print("F ");
      lcd.print(tempC);
      lcd.print("C ");
      lcd.print(humidity);
      lcd.print("%");
      lcd.setCursor(0, 1);
      printDigitsLCD(hourFormat12());
      lcd.print(":");
      printDigitsLCD(minute());
      lcd.print(":");
      printDigitsLCD(second());
      //logic to print AM or PM
      if(isPM()){
        lcd.print("PM");
      }
      if(isAM()){
        lcd.print("AM");
      }
      lcd.print(" ");
      lcd.print(month());
      lcd.print("/");
      lcd.print(day());

      //Conditon to log every 6000 loops. Change if needed.

      //Example that would log every 15 minutes:
      //if(minute() % 15 == 0 && seconds() == 0){

      if(cycles % 6000 == 0){
        File dataFile = SD.open("datalog.csv", FILE_WRITE);
      // if the file is available, write to it:
        if (dataFile) {
          // MM/DD/YYYY HR:MN:SC, TEMPF, TEMPC, HUMIDITY

          dataFile.print(month());
          dataFile.print("/");
          dataFile.print(day());
          dataFile.print("/");
          dataFile.print(year());
          dataFile.print(" ");
          //adds a zero to numbers below ten, prevents issues like 9:3 for 9:03
          //would be in a seperate method, but the datafile isn't opened untill the loop
          if(hour() < 10){
            dataFile.print('0');
          }
          dataFile.print(hour());
          dataFile.print(":");
          if(minute() < 10){
            dataFile.print('0');
          }
          dataFile.print(minute());
          dataFile.print(":");
          if(second() < 10){
          dataFile.print('0');
          }
          dataFile.print(second());
          dataFile.print(",");
          dataFile.print(temp);
          dataFile.print(",");
          dataFile.print(tempC);
          dataFile.print(",");
          dataFile.print(humidity);
          dataFile.println();
          dataFile.close();

          Serial.println("saved to disk.");
          Serial.print(temp);
          Serial.print(" F / ");
          Serial.print(tempC);
          Serial.print("C ");
          Serial.print(humidity);
          Serial.println("% humidity");
        }
        // if the file isn't open, error.
        else {
          Serial.println("error opening datalog.csv");
        }
      }
  EthernetClient client = server.available();
  //this is a mess. will be cleaned up in later versions.
  if (client) {
    Serial.println("new client");
    clients++;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          client.println();
          //begin page output
          client.print("<font size=10> <center>");
          client.print(temp);
          client.print(" F / ");
          client.print(tempC);
          client.print(" C");
          client.print("<br />"); //line break
          client.print(humidity);
          client.println(" % humidity");
          client.print("<br />");
          client.println("Cycles since reset: ");
          client.println(cycles);
          client.print("<br />");
          client.println("Hits: ");
          client.println(clients);
          client.print("<br />");
          client.print(month());
          client.print("/");
          client.print(day());
          client.print("   ");
          if(hourFormat12() < 10){
            client.print("0");
          }
          client.print(hourFormat12());
          client.print(":");
          if(minute() < 10){
            client.print("0");
          }
          client.print(minute());
          client.print(":");
          if(second() < 10){
            client.print("0");
          }
          client.print(second());
          client.print("<br />");
          client.print("</font> </center>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
  cycles++;
  //end webserver
}
