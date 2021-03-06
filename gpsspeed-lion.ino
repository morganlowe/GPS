/* GPS Speedo by Morgan Lowe. https://www.thingiverse.com/thing:4303760
   I dont recall were I found the custom font so uh, whoever made, go you!
   Big Font very similar to https://forum.arduino.cc/index.php?topic=8882.0
   uses Arduino Nano and I2C 20x4 LCD with Serial GPS
   Uses Statistics for arduino by  robtillaart  http://playground.arduino.cc/Main/Statistics
   Uses TinyGPS++ by Arduiniana http://arduiniana.org/libraries/tinygpsplus/
   Do what you will with this!
   On the nano SDA is pin A4 and SCL is pin A5
   Rx from GPS to pin 4 and TX to pin 3.
   Wire Pixels to pin 7.
   Have fun!
   Thanks to AJMartel https://github.com/AJMartel/GPS for the improved code!
*/
#define LAND                           // LAND or SEA        >If SEA the unit will be in Knots, LAND could be KPH or MPH
#define IMPERIAL                       // METRIC or IMPERIAL >If METRIC KPH and Meters, IMPERIAL MPH and Feet
#include <avr/pgmspace.h>              //Used to store "constant variables" in program space (mainly for messages displayed on LCD)
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include "Statistic.h"
#include <Wire.h>                      // Comes with Arduino IDE++
#include <LiquidCrystal_I2C.h>
//LED Driver
#include <Adafruit_NeoPixel.h>
#define PIN            7               // Which pin on the Arduino is connected to the NeoPixels?
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      3               //set number of pixels here, 3 for displaying the status of the satellites. 
// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int onval = 1;
int delayval = 500;                    // delay for half a second
int satval = 0;
unsigned long timeOut;                 // Backlight timeout variable
unsigned long previousMillis = 0;
unsigned long previousMillis1 = 0;
unsigned long interval;
static const int RXPin = 3, TXPin = 4; //GPS Pins
static const uint32_t GPSBaud = 9600;  //GPS Baud rate, set to default of Ublox module
Statistic GPSStats;                    //setup stats
const unsigned long interval1 = 15000; // interval at which to do something (milliseconds)

//Time
// Offset hours from gps time (UTC)
int UTC_offset = -5;                   // US CST with DST

// The TinyGPS++ object
TinyGPSPlus gps;
// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

LiquidCrystal_I2C lcd(0x27, 20, 4);    // Set the LCD I2C address

//Below are the custom characters for the large speed display
int x = 0;
// the 8 arrays that form each segment of the custom numbers
byte LT[8] =
{
  B00111,
  B01111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};
byte UB[8] =
{
  B11111,
  B11111,
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};
byte RT[8] =
{
  B11100,
  B11110,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};
byte LL[8] =
{
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B01111,
  B00111
};
byte LB[8] =
{
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111,
  B11111
};
byte LR[8] =
{
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11110,
  B11100
};
byte UMB[8] =
{
  B11111,
  B11111,
  B11111,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111
};
byte LMB[8] =
{
  B11111,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B11111,
  B11111
};

TinyGPSCustom zdop(gps, "GPVTG", 7);   // $GPVTG sentence, 7th element

void setup()
{
  ss.begin(GPSBaud);
  lcd.init();                          // initialize the lcd
  timeOut = millis();                  // Set the initial backlight time
  delay(10);
  GPSStats.clear();                    //Reset the stats
  //LED Setup
  pixels.begin();                      // This initializes the NeoPixel library.
  lcd.clear();                         // clear the screen - intro below!
  // assignes each segment a write number
  lcd.createChar(1, UB);
  lcd.createChar(2, RT);
  lcd.createChar(3, LL);
  lcd.createChar(4, LB);
  lcd.createChar(5, LR);
  lcd.createChar(6, UMB);
  lcd.createChar(7, LMB);
  lcd.createChar(8, LT);
  lcd.backlight();                     // turn on backlight
  lcd.clear();                         // clear the screen - intro below!
  lcd.setCursor(1, 0);                 // put cursor at colon x and row y
  lcd.print(F("   MORNINGLION "));        // print a text
  lcd.setCursor(0, 1);                 // put cursor at colon x and row y
  lcd.print(F("     INDUSTRIES"));        // print a text
  lcd.setCursor(1, 2);                 // put cursor at colon x and row y
  lcd.print(F("GPS data - Lion 4.0"));    // print a text
  lcd.setCursor(0, 3);                 // put cursor at colon x and row y
  lcd.print(F("  GPS Speedometer "));     // print a text
  delay (5000);  //time  boot screen is shown, 5 seconds
  lcd.clear();
}

void loop() {

  int satval = gps.satellites.value(); //add sats availible to satval
  if (satval >= 13) {
    satval = 12;
  }

  //below turns off the backlight if it's just waiting for satelites
  if (satval >= 1) {
    timeOut = millis();
    lcd.backlight();                   // turn on backlight
    onval = 1;
  }
  else if ((millis() - timeOut) > 30000) {  // Turn off backlight after 30 seconds
    lcd.noBacklight();
    onval = 0;
  }

  delay(100);

  //LEDs
  // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255

  if (onval == 1) {
    switch (satval) {
      case 0:
        pixels.setPixelColor(0, pixels.Color(80, 0, 0));
        pixels.setPixelColor(1, pixels.Color(80, 0, 0));
        pixels.setPixelColor(2, pixels.Color(80, 0, 0));
        break;
      case 1:
        pixels.setPixelColor(0, pixels.Color(80, 30, 0));
        pixels.setPixelColor(1, pixels.Color(80, 0, 0));
        pixels.setPixelColor(2, pixels.Color(80, 0, 0));
        break;
      case 2:
        pixels.setPixelColor(0, pixels.Color(80, 30, 0));
        pixels.setPixelColor(1, pixels.Color(80, 30, 0));
        pixels.setPixelColor(2, pixels.Color(80, 0, 0));
        break;
      case 3:
        pixels.setPixelColor(0, pixels.Color(80, 30, 0));
        pixels.setPixelColor(1, pixels.Color(80, 30, 0));
        pixels.setPixelColor(2, pixels.Color(80, 30, 0));
        break;
      case 4:
        pixels.setPixelColor(0, pixels.Color(0, 80, 0));
        pixels.setPixelColor(1, pixels.Color(80, 30, 0));
        pixels.setPixelColor(2, pixels.Color(80, 30, 0));
        break;
      case 5:
        pixels.setPixelColor(0, pixels.Color(0, 80, 0));
        pixels.setPixelColor(1, pixels.Color(0, 80, 0));
        pixels.setPixelColor(2, pixels.Color(80, 30, 0));
        break;
      case 6:
        pixels.setPixelColor(0, pixels.Color(0, 80, 0));
        pixels.setPixelColor(1, pixels.Color(0, 80, 0));
        pixels.setPixelColor(2, pixels.Color(0, 80, 0));
        break;
      case 7:
        pixels.setPixelColor(0, pixels.Color(0, 0, 80));
        pixels.setPixelColor(1, pixels.Color(0, 80, 0));
        pixels.setPixelColor(2, pixels.Color(0, 80, 0));
        break;
      case 8:
        pixels.setPixelColor(0, pixels.Color(0, 0, 80));
        pixels.setPixelColor(1, pixels.Color(0, 0, 80));
        pixels.setPixelColor(2, pixels.Color(0, 80, 0));
        break;
      case 9:
        pixels.setPixelColor(0, pixels.Color(0, 0, 80));
        pixels.setPixelColor(1, pixels.Color(0, 0, 80));
        pixels.setPixelColor(2, pixels.Color(0, 0, 80));
        break;
      case 10:
        pixels.setPixelColor(0, pixels.Color(80, 0, 80));
        pixels.setPixelColor(1, pixels.Color(0, 0, 80));
        pixels.setPixelColor(2, pixels.Color(0, 0, 80));
        break;
      case 11:
        pixels.setPixelColor(0, pixels.Color(80, 0, 80));
        pixels.setPixelColor(1, pixels.Color(80, 0, 80));
        pixels.setPixelColor(2, pixels.Color(0, 0, 80));
        break;
      case 12:
        pixels.setPixelColor(0, pixels.Color(80, 0, 80));
        pixels.setPixelColor(1, pixels.Color(80, 0, 80));
        pixels.setPixelColor(2, pixels.Color(80, 0, 80));
        break;
    }
  }

  if (onval == 0) {                    //if the backlight is off turn off pixels too
    pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    pixels.setPixelColor(1, pixels.Color(0, 0, 0));
    pixels.setPixelColor(2, pixels.Color(0, 0, 0));

  }

  pixels.show();                       // This sends the updated pixel color to the hardware.

  //clear out data if there is no sats to prevent false averages
  unsigned long currentMillis1 = millis();
  if (currentMillis1 - previousMillis1 >= interval1) {
    previousMillis1 = currentMillis1;
    if (gps.satellites.value() == 0) {
      GPSStats.clear();
      lcd.clear();
    }
  }

  //Add data to stats & Code for unit change
#ifdef LAND
#ifdef METRIC
  GPSStats.add(gps.speed.kmph());  //Send speed to Stats for average and max
  int speed = (gps.speed.kmph());  //collect speed for display
#else
  GPSStats.add(gps.speed.mph());   //Send speed to Stats for average and max
  int speed = (gps.speed.mph());   //collect speed for display
#endif
#else
  GPSStats.add(gps.speed.knots());   //Send speed to Stats for average and max
  int speed = (gps.speed.knots());   //collect speed for display
#endif
  screen();
  speedcalc(speed);
  lcd.setCursor(11, 3);
#ifdef LAND
#ifdef METRIC
  lcd.print(F("KPH"));
#else
  lcd.print(F("MPH"));
#endif
#else
  lcd.print(F("Kts"));
#endif

  smartDelay(1000);
}


// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}


static void screen() {


  //Time setup

  byte dispt = gps.time.hour();
  delay(10);

  // Calc current Time Zone time by offset value
  delay(10);
  dispt = (dispt + UTC_offset);

  //AM or PM calc
  
  if (dispt == 0) {
    dispt = (dispt + 12);
  }
  
  if (dispt > 24) {
    dispt = (dispt - 12);
  }

  if (dispt > 12) {
    dispt = (dispt - 12);
    lcd.setCursor(18, 1);
    lcd.print(F("PM"));
  }

    if (dispt <= 12) {
    lcd.setCursor(18, 1);
    lcd.print(F("AM"));
  }

  //put the : in the time
  lcd.setCursor(12, 1);
  if (dispt < 10) lcd.print(F(" "));
  lcd.print(dispt);
  lcd.print(F(":"));
  if (gps.time.minute() < 10) lcd.print(F("0"));
  lcd.print(gps.time.minute());


  //Measure altitude
  lcd.setCursor(14, 2);
#ifdef METRIC
  lcd.print(gps.altitude.meters());
#else
  lcd.print(gps.altitude.feet());
#endif
  lcd.setCursor(15, 3);
#ifdef METRIC
  lcd.print(F("METER"));
#else
  lcd.print(F("^FEET"));
#endif

  //average speed
  lcd.setCursor(0, 1);
  lcd.print(F("AVG:"));
  lcd.print(GPSStats.average(), 0);
#ifdef LAND
#ifdef METRIC
  lcd.print(F("KPH"));
#else
  lcd.print(F("MPH"));
#endif
#else
  lcd.print(F("Kts"));
#endif

  //max speed
  lcd.setCursor(0, 0);   // put cursor at colon 0 and row 0 = left/up
  lcd.print(F("MAX:"));
  lcd.print(GPSStats.maximum(), 0);
#ifdef LAND
#ifdef METRIC
  lcd.print(F("KPH"));
#else
  lcd.print(F("MPH"));
#endif
#else
  lcd.print(F("Kts"));
#endif

  // Sat Numbers
  lcd.setCursor(14, 0);
  lcd.print(gps.satellites.value());
  lcd.print(F(" SATS"));
}


void custom0O()     // uses segments to build the number
{
  lcd.setCursor(x, 2);
  lcd.write(8);
  lcd.write(1);
  lcd.write(2);
  lcd.setCursor(x, 3);
  lcd.write(3);
  lcd.write(4);
  lcd.write(5);
}

void custom1()
{
  lcd.setCursor(x, 2);
  lcd.write(1);
  lcd.write(2);
  lcd.setCursor(x, 3);
  lcd.write(4);
  lcd.write(255);
  lcd.write(4);
}

void custom2()
{
  lcd.setCursor(x, 2);
  lcd.write(6);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x, 3);
  lcd.write(3);
  lcd.write(7);
  lcd.write(7);
}

void custom3()
{
  lcd.setCursor(x, 2);
  lcd.write(6);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x, 3);
  lcd.write(7);
  lcd.write(7);
  lcd.write(5);
}

void custom4()
{
  lcd.setCursor(x, 2);
  lcd.write(3);
  lcd.write(4);
  lcd.write(2);
  lcd.setCursor(x + 2, 3);
  lcd.write(255);
}

void custom5()
{
  lcd.setCursor(x, 2);
  lcd.write(255);
  lcd.write(6);
  lcd.write(6);
  lcd.setCursor(x, 3);
  lcd.write(7);
  lcd.write(7);
  lcd.write(5);
}

void custom6()
{
  lcd.setCursor(x, 2);
  lcd.write(8);
  lcd.write(6);
  lcd.write(6);
  lcd.setCursor(x, 3);
  lcd.write(3);
  lcd.write(7);
  lcd.write(5);
}

void custom7()
{
  lcd.setCursor(x, 2);
  lcd.write(1);
  lcd.write(1);
  lcd.write(2);
  lcd.setCursor(x + 1, 3);
  lcd.write(8);
}

void custom8()
{
  lcd.setCursor(x, 2);
  lcd.write(8);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x, 3);
  lcd.write(3);
  lcd.write(7);
  lcd.write(5);
}

void custom9()
{
  lcd.setCursor(x, 2);
  lcd.write(8);
  lcd.write(6);
  lcd.write(2);
  lcd.setCursor(x, 3);
  lcd.write(7);
  lcd.write(7);
  lcd.write(5);
}


void tempvar(int numar)
{
  switch (numar)
  {
    case 0:
      custom0O();
      break;

    case 1:
      custom1();
      break;

    case 2:
      custom2();
      break;

    case 3:
      custom3();
      break;

    case 4:
      custom4();
      break;

    case 5:
      custom5();
      break;

    case 6:
      custom6();
      break;

    case 7:
      custom7();
      break;

    case 8:
      custom8();
      break;

    case 9:
      custom9();
      break;
  }
}

//calculate the speed position on screen based on number of digits
void speedcalc(int speed)
{

  lcd.setCursor(0, 2);
  lcd.print(F("           "));
  lcd.setCursor(0, 3);
  lcd.print(F("           "));

  if (speed >= 100)
  {
    x = 0;
    tempvar(int(speed / 100));
    speed = speed % 100;

    x = x + 4;
    tempvar(speed / 10);

    x = x + 4;
    tempvar(speed % 10);
  }
  else if (speed >= 10)
  {
    lcd.setCursor(0, 2);
    lcd.print(F("   "));
    lcd.setCursor(0, 3);
    lcd.print(F("   "));

    x = 4;
    tempvar(speed / 10);

    x = x + 4;
    tempvar(speed % 10);
  }
  else if (speed < 10)
  {
    lcd.setCursor(0, 2);
    lcd.print(F("       "));
    lcd.setCursor(0, 3);
    lcd.print(F("       "));
    x = 8;
    tempvar(speed);
  }

}
