// A simple CO2 meter using the Adafruit SCD30 breakout and the Adafruit 128x32 OLEDs

// Changes by Ofer:
// 1) change display to arrange all data in one row, without TDM on data fields (humidity).
// 2) When "low Vbat" - print Vbat in inverse text (black pixel on white background)
// 3) Change "low Vbat" threshold to 3.47V
// 4) Serial print all data available in one row.
// 5) Add averaging filter to readings for greater stability of readings. (less jitter).
// 6) use temperature offset to compensate of chip heating.

#include <Adafruit_SCD30.h>
#include <Adafruit_SSD1306.h>

#include <SPI.h>
#include <SD.h>
#include "RTClib.h"
RTC_PCF8523 rtc;

#include <Adafruit_NeoPixel.h>
#define NEO_PIN 8
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, NEO_PIN, NEO_GRB + NEO_KHZ800);

int verNumber = 3;

Adafruit_SCD30  scd30;

//---OLED display
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);
#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5

#define VBATPIN A6 //A13 //14 // changed per CPU board.

//========================= section to control compiler options =====================
#define RS232_info true	// hide this line if you want SW to avoid printing for initial info data.
#define RS232_Debug true	// hide this line if you want SW to avoid any serial activity for Debug data.
//#define RS232_Readings true	// hide this line if you want SW to avoid any serial activity for printing readings each 2 Sec.
#define RS232_Logging true	// hide this line if you want SW to avoid any serial activity for printing Log data each 30 Sec.
#define SD_option true		// hide this line if you HW does not include SD card. (avoid SD-card activity).
#define Relay true			// hide this line if Relay not needed.
//=============================== end ===============================================

//boolean initDone;
unsigned long initTimer;

// ============ Relay and LED indications ===================
const int ledPin =  13;      // the number of the LED pin
const int relayPin =  12;      // the number of the Relay pin
int bad_co2 = 1100;
int Hyst_co2 = 300;
// Variables will change:
int ledState = LOW;             // ledState used to set the LED
unsigned long previousMillis = 0;        // will store last time LED was updated
// ==========================================================

// the follow variables is a long because the time, measured in milliseconds,
// will quickly become a bigger number than can be stored in an int.
unsigned long interval = 1000;           // interval at which to blink (milliseconds)

unsigned long writeInterval = 30000;  // write results to SD card every 30 Sec.
unsigned long writeTimer;

unsigned long checkInterval = 2000;   // read CO2 every 2 sec.
unsigned long checkTimer;

float cur_co2 = 420;
float cur_humidity = 30;
float cur_temperature = 20;
float cur_measuredvbat = 3.0;
//boolean vbat_toggleShow;
// bool isInMenu = false;

unsigned long sdCard_checkTimer;
bool sdCard_found;

// ========= Added by ofer to support small menu ===============================
unsigned char MenuState = 0; // indicate what menu currently selected: 0=init , 1=info, 10=IDLE, ... as per MenuStateTree[].
                    // MenuState=0 after power-up, than go to MenuState=1, than go to MenuState=10 than per navigation.
#define DebuonceTime 75  // Key DE-bounce time in Milli's.
#define LongPress 3000   // define time to determine if key pressed for "long time' in Milli's.
unsigned long KEY_A_Time = 0;     // parameter to hold time of KEY A button.
unsigned long KEY_B_Time = 0;     // parameter to hold time of KEY B button.
unsigned long KEY_C_Time = 0;     // parameter to hold time of KEY C button.
unsigned char KeyA_state = 0;   // parameter to indicate the state of KeyA: 0= not pressed , 1=DebounceTime , 2= Pressed , 
								// 3= LongPress , 4= Application response to short press , 8= Application response to RAPID press.
unsigned char KeyB_state = 0;
unsigned char KeyC_state = 0;
int NextMenuItem = 1;		// Start with 1.
int Digits[4] = {0};		// Define array to hold digits to be set. (up to 5 digits)
int DigitsMin[4] = {0};	// define minimum SET range for each digit (for BCD start at 0)
int DigitsMax[4] = {9};	// define maximum SET range for each digit (for BCD end at 9)
unsigned char DigitsXoffset = 6;	// OLED X distance to next digit. (for character_size=1 it is 6 pixels between digits)
unsigned char NumberOfDigits = 3;	// define number of digits in the parameter to be set. (default = 3 digits number)
unsigned char OledSetupStage = 0;		// initial state machine for setup of digits.
 
// Menu Tree:  Note - some screens does not display menus, rather used for setup an parameter.
//
// IDEL Screen (10)
//   |
// Main Menu (11)
//   +------------------------------+
//   |                              |
//   Clock Setup (12)               CO2 Setup (15)
//           |                            |
//           Date Setup: (13)             Fan Setup (16)
//           |      |                     |     |
//           |      (set Screen)          |     Start At:   (19) --> (set Screen)
//           |                            |     Hysteresis: (20) --> (set Screen)
//           Time Setup: (14)             |      
//                  |                     CO2 Calibration - Caution  (17)
//                  (set Screen)				               |
//                         			                        Are you sure: (18)
//																		|
//																Display results (21)
//
// MenuStateTree[] hold the jump structure for each MenuSate, starting with MenuState=10. (lower MenuState values are reserved for INIT period.)
// for each MenuState vector, the first item holds the jump MenuState value when NextMenuItem=0, the second item holds the jump MenuState value when NextMenuItem=1,
// and so on.   For cases where any key should continue - all vector will have the same value. (please note to block cursor on screen).
const byte MenuStateTree[] = {11,11,11,12,15,10,13,14,11,12,12,12,12,12,12,16,17,11,19,20,15,18,18,18,18,21,15,16,16,16,16,16,16,15,15,15}; 	// MenuState table
//                            \______/ \______/ \______/ \______/ \______/ \______/ \______/ \______/ \______/ \______/ \______/ \______/
//	for MenuState=  	    	 10	     11	     12         13		 14		  15	   16		17		18		  19		20		21
	
const char *MenuStateText[] = {"1) clock set", "2) CO2 setup", "3) Back",   // main menu    0.  [0..2]
"1) Set Date", "2) SetTime", "3) Back",                       				// Clock setup  1.  [3..5]
" dd :  MM : YYYY", "  :  :", "Invalid Date",                     			// Date setup   2.  [6..8]
" HH : MM  : SS", "  :  :", "Saved",                        				// Time Setup   3.  [9..11]
"1) FAN Operating", "2) CO2 Calibration", "3) Back",                		// CO2 setup  	4.  [12..14]
"1) Start At", "2) Hysteresis", "3) Back",                      			// Fan setup  	5.  [15..17]
"Caution, irreversible", "Use only when CO2 is", "430ppm. (Enter=Back)",    // CO2 calibration caution 6. [18..20]
"Are you sure", "1) Yes", "2) No",											// CO2 calibration  7.  [21..23]
"  range: 1000-1999", "1", " ",                          					// Fan start at   	8.  [24..26]
"  range: 100-699", " ", " "};                                              // Fan hysteresis   9.  [27..29]

// ================ END small Menu declarations ================================
/*
// ======= for debug ============
bool DebugFlag = true;
unsigned long DebugTimer = 0;
// ======== end debug ===========	*/

void setup(void) {
//  DebugFlag = true;
  MenuState = 0;                    // set menu to INIT
  OledSetupStage = 0;				// set SETUP state machine to 0.
#ifdef RS232_Debug
  Serial.begin(115200);
  //  while (!Serial) delay(10);     // will pause Zero, Leonardo, etc until serial console opens

  delay(500);

  Serial.println("SCD30 OLED CO2 meter!");
#endif
  // Try to initialize!
  if (!scd30.begin()) {
#ifdef RS232_Debug
    Serial.println("Failed to find SCD30 chip - PROGRAM STOP!");
#endif
    while (1) {                       // stay here forever if no CO2 sensor.
      delay(10);
    }
  }
#ifdef RS232_Debug
  Serial.println("SCD30 Found!");
#endif
 
  delay(2000);
  setup_rtc();

  //---OLED display
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
#ifdef RS232_Debug
    Serial.println(F("SSD1306 allocation failed"));
#endif
    for (;;);                                   // Don't proceed, loop forever
  }

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  if (!scd30.setMeasurementInterval(2)) {
#ifdef RS232_Debug
    Serial.println("Failed to set measurement interval");
#endif
    while (1) {
      delay(10);
    }
  }

  //only set those once, NOT every time we start the device
  scd30.setAltitudeOffset(35); //in ramat-hayal 35m above sea level   
  scd30.setTemperatureOffset(250);  // temperature offset of 2.5C.
  // 1015 => 10.15 degrees C
  // 31337 => 313.37 degrees C

#ifdef Relay
	pinMode(relayPin, OUTPUT);				// define Relay pin as OUTPUT
	digitalWrite(relayPin, LOW);			// start with relay not active
#endif

  // set the digital pin as output:
  pinMode(8, OUTPUT);				// for neo-pixel

  strip.begin();
  strip.setBrightness(50);
  strip.show(); // Initialize all pixels to 'off'

  display.display();
  delay(500); // Pause for half second

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setRotation(0);

#ifdef SD_option
  sdCard_found = setup_sd();
  #ifdef RS232_info
  Serial.println("SD Ready!");
  #endif
#endif
  MenuState = 1;          // indicate init done - move to INFO display
  initTimer = millis();
  checkTimer = millis();
  writeTimer = millis();

#ifdef RS232_Debug
  Serial.println("setup done");
#endif
}

void loop() {
  CheckKey();                   // check for key press and menu navigation.
//----- check if user forgot to return to IDLE screen ---------------
  if (MenuState > 10) {         // in not in IDLE screen
        if ((unsigned long)(millis() - initTimer) > 60000) {  // if pass 60Sec inside a menu that is not IDLE menu.
            MenuState = 10;     // if menu left over for more than 60 Sec - return to Idle screen.
        }  
  } 
  if ( MenuState == 1) {        //check if ( MenuState == 1), used to display INFO screen for 5 Sec.
  //------print initial info
    if ((unsigned long)(millis() - initTimer) > 5000) {  // if pass 5Sec after init
      PrintRS232InfoScreen ();  // print RS232 DATA for info screen.
      MenuState = 10;           // change state to IDLE menu, to make sure it will execute only one time.
    } else {                    // if less than 5Sec after init
      PrintOLEDInfoScreen ();   // print OLED DATA for info screen.
      strip.setPixelColor(0, strip.Color(0, 0, 255));
      strip.show();
    }
  } else if ( MenuState == 10) { //check if ( MenuState == 10 , meaning MenuState=IDLE) i.e. After first 5 sec of initial info display.
    initTimer = millis();       // Timer to avoid "Menu leftover" i.e operator stay in setting menu too long.
    strip.setPixelColor(0, strip.Color(0, 0, 128));
    strip.show();
	MenuNavigation(3);			// handle 3 sub-menu items with navigation keys
#ifdef SD_option
//---check every 5 seconds if SD card is present -----
	if ((unsigned long)(millis() - sdCard_checkTimer) > 5000) {
		sdCard_checkTimer = millis();
		sdCard_found = setup_sd();
	}
//----------------------------------------------------
#endif
// ======== check if time to read sensors =======================================================
      if ((unsigned long)(millis() - checkTimer) > checkInterval) {	// sequentially read CO2. (once in 2 Sec)
        checkTimer = millis();
        ReadAllSensors ();          // Read all sensors
		PrintIdleScreen ();			// take care to print all information on Screen
      } //end "read sensor" - if (Milli's() - writeTimer > writeInterval)
// ========== end read sensors ==================================================================

#ifdef SD_option
// ========= check if time to log data into SD-card ====================
      if (cur_co2 != 9999.9 && ((unsigned long)(millis() - writeTimer) > writeInterval)) {  // check if no reading error and time to write log. (30 Sec)
        writeTimer = millis();												// restart the 30 Sec timer
        logTo_sd(cur_measuredvbat, cur_co2, cur_temperature, cur_humidity); // time stamp included.
      }
// ============== end log to SD-card ===================================
#endif

      if (cur_co2 > bad_co2) {
        fadeNeoPixel(true);
#ifdef Relay
  #ifdef RS232_info
        if (digitalRead(relayPin)==LOW) {       // check if this is the first time
            Serial.println("Activate Relay");   // provide indication on RS232
        }
  #endif
		digitalWrite(relayPin, HIGH);			// Activate relay 
	  } else if(cur_co2 < (bad_co2-Hyst_co2)) {
  #ifdef RS232_info
            if (digitalRead(relayPin)==HIGH) {  // check if this is the first time
                Serial.println("SutDown Relay");    // provide indication on RS232
            }
            digitalWrite(relayPin, LOW);        // Stop Activate relay 
  #endif
			fadeNeoPixel(false);
#endif
      } else {
        fadeNeoPixel(false);
      }

 } else if ( MenuState == 11) {
	displaymenu(3,true,false);		// display 3 items of correspondence menu with cursor mark.
	MenuNavigation(3);			// handle 3 sub-menu items with navigation keys
 } else if ( MenuState == 12) {	// Clock setup
	displaymenu(3,true,false);		// display 3 items of correspondence menu with cursor mark.
	MenuNavigation(3);			// handle 3 sub-menu items with navigation keys
// ============== Prepare data for next screen ============
    if (MenuState==13) {                // if changing to "set date"
        DateTime now = rtc.now();       // retrieve current date
        Digits[0] = now.day();          // use current date as start point for change.
        Digits[1] = now.month();
        Digits[2] = now.year();
//      DigitsMin[4] = {0,0,0,0};       // define minimum SET range for each digit (for BCD start at 0)
        DigitsMin[0] = {1};   
        DigitsMin[1] = {1};   
        DigitsMin[2] = {2010};   
        DigitsMin[3] = {0};   
//      DigitsMax[4] = {31,12,99,99};   // define maximum SET range for each digit (for BCD end at 9)
        DigitsMax[0] = {31};
        DigitsMax[1] = {12};
        DigitsMax[2] = {2099};
        DigitsMax[3] = {99};
        NextMenuItem = 0;               // start at first digit
    } else if (MenuState==14) {         // if changing to "set time"
        DateTime now = rtc.now();       // retrieve current time
        Digits[0] = now.hour();         // use current time as start point for setting.
        Digits[1] = now.minute();
        Digits[2] = now.second();
        DigitsMin[4] = {0};             // define minimum SET range for each digit (for BCD start at 0)
//      DigitsMax[4] = {23,59,59,99};   // define maximum SET range for each digit (for BCD end at 9)
        DigitsMax[0] = {23};
        DigitsMax[1] = {59};
        DigitsMax[2] = {59};
        DigitsMax[3] = {99};
        NextMenuItem = 0;               // start at first digit
    }
 } else if ( MenuState == 13) { // Date setup
    strip.setPixelColor(0, strip.Color(128, 64, 128));
    strip.show();
	PrintMenuHeader ();			// display menu header on first screen line. 
	display.setCursor(0, 8);	// display help for setting on 2'nd line
    display.print(MenuStateText[int((MenuState-11)*3+0)]);  // display correspondence menu line.
//  ========= write digit separators =============
	display.setTextSize(2);
    display.setCursor(0, 16);
    display.print(MenuStateText[int((MenuState-11)*3+1)]);  // display correspondence menu line.
//  ============ end digit separators ==========
	DisplaySetDigits (0 , 16 , 3 , 36 , 2);		// write digits into display buffer. // use NextMenuItem to define What digit to SET
	display.display();  		// update screen with all changes 
// =========== digits navigation ========================
	SetupValueNavigation (3, 12);	// check if last digit set, and return to previous menu.
// ============== end of digit navigation ===============
	if (MenuState == 12) {			// if new value ready to be set:
//    set RTC +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
		DateTime now = rtc.now();	// retrieve update for current time.
		DateTime t(Digits[2], Digits[1], Digits[0], now.hour(), now.minute(), now.second());  // prepare setup string.
		if (t.isValid()) {			// check if date is valid. (i.e. 31-FEB is not valid)
			rtc.adjust(DateTime (Digits[2], Digits[1], Digits[0], now.hour(), now.minute(), now.second()));	// if yes - update RTC.
		} else {
			PrintMenuHeader ();			// display menu header on first screen line. 
			display.setCursor(0, 8);	// display help for setting on 2'nd line
			display.print("Date not valid");
            display.display();  // update screen with all changes  
			MenuState == 13;			// remain in current screen
			delay (1000);					// delay for display the error
		}
	}
 } else if ( MenuState == 14) {	// Time Setup
    strip.setPixelColor(0, strip.Color(128, 64, 128));
    strip.show();
	PrintMenuHeader ();			// display menu header on first screen line. 
	display.setCursor(0, 8);	// place for display help for setting on 2'nd line
    display.print(MenuStateText[int((MenuState-11)*3+0)]);  // display correspondence menu help line.
//  ========= write digit separators =============
	display.setTextSize(2);                                 // define font size
    display.setCursor(0, 16);                               // define position
    display.print(MenuStateText[int((MenuState-11)*3+1)]);  // display correspondence menu line.
//  ============ end digit perpetrators ==========
	DisplaySetDigits (0 , 16 , 3 , 36 , 2);		// write digits into display buffer. // use NextMenuItem to define What digit to SET
	display.display();  		// update screen with all changes 
// =========== digits navigation ========================
	SetupValueNavigation (3, 12);	// check if last digit set, and return to previous menu.
// ============== end of digit navigation ===============
  if (MenuState == 12) {      // if new value ready to be set:
//    set RTC +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
    DateTime now = rtc.now(); // retrieve update for current time.
    rtc.adjust(DateTime (now.year(), now.month(), now.day(), Digits[0], Digits[1], Digits[2])); // update RTC.
  }
 } else if ( MenuState == 15) { // if CO2 setup menu
    displaymenu(3,true,false);        // display 3 items of correspondence menu with cursor mark.
    MenuNavigation(3);          // handle 3 sub-menu items with navigation keys
 } else if ( MenuState == 16) { // if FAN setup menu
    displaymenu(3,true,false);        // display 3 items of correspondence menu with cursor mark.
    MenuNavigation(3);          // handle 3 sub-menu items with navigation keys
// ============== Prepare data for next screen ============
    if (MenuState==19) {                // if changing to "Start at"
		String number = String(bad_co2, DEC);	// convert threshold into string of decimal values Digits[0] = MSB
//		itoa(bad_co2, Digits, 10);		// convert parameter into string of decimal values Digits[0] = MSB
        Digits[0] = number[1]-48;       // ignore the leading '1' of 1999 by starting with number[1].
        Digits[1] = number[2]-48;       // convert String to number by subtracting '0'.
        Digits[2] = number[3]-48;
        Digits[3] = number[0]-48;
        DigitsMin[0] = {0};   
        DigitsMin[1] = {0};   
        DigitsMin[2] = {0};   
        DigitsMin[3] = {0};   
        DigitsMin[4] = {0};             // define minimum SET range for each digit (for BCD start at 0)
        DigitsMax[0] = {9};
        DigitsMax[1] = {9};
        DigitsMax[2] = {9};
        DigitsMax[3] = {9};
        DigitsMax[4] = {9};
        NextMenuItem = 0;               // start at first digit
    } else if (MenuState==20) {         // if changing to "set hysteresis"
		String number = String(Hyst_co2, DEC);	// convert Hysteresis into string of decimal values Digits[0] = MSB
        Digits[0] = number[0]-48;
        Digits[1] = number[1]-48;
        Digits[2] = number[2]-48;
        Digits[3] = number[3]-48;
        DigitsMin[0] = {1};   
        DigitsMin[1] = {0};   
        DigitsMin[2] = {0};   
        DigitsMin[3] = {0};   
        DigitsMin[4] = {0};             // define minimum SET range for each digit (for BCD start at 0)
        DigitsMax[0] = {6};
        DigitsMax[1] = {9};
        DigitsMax[2] = {9};
        DigitsMax[3] = {9};
        DigitsMax[4] = {9};
        NextMenuItem = 0;               // start at first digit
    }
 } else if ( MenuState == 17) { // if CO2 calibration menu
    displaymenu(3,false,true);  // display 3 items of correspondence menu with cursor mark.
    MenuNavigation(3);          // handle 3 sub-menu items with navigation keys
 } else if ( MenuState == 18) { // if CO2 calibration pass "caution" warning
    displaymenu(3,true,false);  // display 3 items of correspondence menu with cursor mark.
    MenuNavigation(3);          // handle 3 sub-menu items with navigation keys
	if ( MenuState == MenuStateTree[(18-10)*3+1]) {	// if choose "YES", continue to calibration:
		initTimer = millis();						// start timer for next screen
		scd30.forceRecalibrationWithReference(430);	// force CO2 calibration to 430 Ppm
	}	
 } else if ( MenuState == 19) { // FAN start setup
    strip.setPixelColor(0, strip.Color(64, 0, 0));
    strip.show();
	PrintMenuHeader ();			// display menu header on first screen line. 
	display.setCursor(0, 8);	// display help for setting on 2'nd line
    display.print(MenuStateText[int((MenuState-11)*3)]);  // display correspondence menu help.
//  ========= write leading digits =============
	display.setTextSize(2);
    display.setCursor(0, 16);
    display.print(MenuStateText[int((MenuState-11)*3+1)]);  // display correspondence menu line.
//  ============ end leading digits ===========
	DisplaySetDigits (12 , 16 , 3 , 12 , 2);		// write digits into display buffer. // use NextMenuItem to define What digit to SET
	display.display();  		// update screen with all changes 
// =========== digits navigation ========================
	SetupValueNavigation (3, MenuStateTree[(MenuState-10)*3+2]);	// check if last digit set, and return to previous menu according to MenuStateTree[].
// ============== end of digit navigation ===============
	if ( MenuState == MenuStateTree[(19-10)*3+2]) {			// if new value ready to be set upon exit:
        bad_co2 = 1000+(Digits[0]*100)+(Digits[1]*10)+Digits[2];    // store new parameter
	}
 } else if ( MenuState == 20) { // Hysteresis setup
    strip.setPixelColor(0, strip.Color(64, 32, 0));
    strip.show();
	PrintMenuHeader ();			// display menu header on first screen line. 
	display.setCursor(0, 8);	// display help for setting on 2'nd line
    display.print(MenuStateText[int((MenuState-11)*3)]);  // display correspondence menu help.
//  ========= write leading digits =============
	display.setTextSize(2);
    display.setCursor(0, 16);
    display.print(MenuStateText[int((MenuState-11)*3+1)]);  // display correspondence menu line.
//  ============ end leading digits ============
	DisplaySetDigits (12 , 16 , 3 , 12 , 2);	// write digits into display buffer. // use NextMenuItem to define What digit to SET
	display.display();  		// update screen with all changes 
// =========== digits navigation ========================
	SetupValueNavigation (3, MenuStateTree[(MenuState-10)*3+2]);	// check if last digit set, and return to previous menu according to MenuStateTree[].
// ============== end of digit navigation ===============
	if ( MenuState == MenuStateTree[(20-10)*3+2]) {			// if new value ready to be set upon exit:
        Hyst_co2 = (Digits[0]*100)+(Digits[1]*10)+Digits[2];    // store new parameter
	}
 } else if ( MenuState == 21) { // Hysteresis setup
    strip.setPixelColor(0, strip.Color(64, 32, 32));
    strip.show();
	PrintMenuHeader ();			// display menu header on first screen line. 
	display.setCursor(0, 8);	// display help for setting on 2'nd line
    display.print("Calibrate CO2=430ppm");  // display correspondence menu help.
	display.setCursor(0, 16);	// display timer on 2'nd line
    display.print( (3000 - (millis() - initTimer)) );	// Display 3Sec timer on screen.
    display.display();
	if ((millis() - initTimer) >= 3000)  {	// if pass 3sec
		MenuState = MenuStateTree[(21-10)*3+2];			// return to previous menu.
	}
 } else {						// when MenuState is not recognize as Valid
    strip.setPixelColor(0, strip.Color(128, 128, 0));
    strip.show();
    PrintMenuHeader ();         // display menu header on first screen line. 
    display.setTextSize(1);
    display.setCursor(0, 8);
    display.print("MenuState Error");
    display.setCursor(100, 8);
	display.print(MenuState,DEC);
    display.display();          // update screen with all changes 
	Serial.print("MenuState Error= ");
    Serial.print(MenuState, 0);
    delay(2000);
    MenuState=10;        // return to idle screen
 }

  //--blink on-board LED to indicate code is running
  blink_it();

}


void blink_it()
{

  // here is where you'd put code that needs to be running all the time.

  // check to see if it's time to blink the LED; that is, if the
  // difference between the current time and last time you blinked
  // the LED is bigger than the interval at which you want to
  // blink the LED.

  if ((unsigned long)(millis() -previousMillis) > interval) {
    // save the last time you blinked the LED
    previousMillis = millis();

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW)
      ledState = HIGH;
    else
      ledState = LOW;

    // set the LED with the ledState of the variable:
    digitalWrite(ledPin, ledState);
  }

}

void fadeNeoPixel(bool _doIt) {
  if (_doIt) {
    int temp_percent = map(millis() % 10000, 0, 10000, -255, 255);
    //    Serial.print("temp_percent ");
    //    Serial.print(temp_percent);
    //    Serial.println();

    strip.setPixelColor(0, strip.Color(abs(temp_percent), 0, 0));
    strip.show();
  } else {
    strip.setPixelColor(0, strip.Color(0, 0, 0));
    strip.show();
  }

}

void PrintIdleScreen ()
{
#ifdef RS232_Readings
// ========= changed serial print into one long line by Ofer ============
	Serial.print("CO2: ");
	Serial.print(cur_co2, 1);
	erial.print(" ppm");
	Serial.print("\t");         // prints a tab
	Serial.print("TEMP: ");
	Serial.print(cur_temperature, 2);
	Serial.print(" C");
	Serial.print("\t");         // prints a tab
	Serial.print("Humidity: ");
	Serial.print(cur_humidity, 1);
	Serial.print(" %");	
	Serial.print("\t");         // prints a tab
	Serial.print("Vbat: ");
	Serial.print(cur_measuredvbat, 2);
	Serial.println(" V");	// print CR at the end.
	Serial.println("");
// ======================================================================
#endif
// ============= print IDLE screen header line ===================
	display.clearDisplay();
	display.setTextSize(1);

	DateTime now = rtc.now();
	display.setCursor(0, 0);
	display.print(now.timestamp(DateTime::TIMESTAMP_TIME));
	if (sdCard_found == true) {
		display.setCursor(66, 0);
	display.println("SD LOGGING");
    } else {
		display.setCursor(66, 0);
		display.println("NO SD-CARD!");
    }
// ========= end print IDLE screen header line ===================

// ===== print all data on line 2, avoid no print of humidity, rather print inverse text during "low bat"  =======
 
// ================ changed by Ofer ========================================
    if (cur_measuredvbat < 3.52 ) {		// if "LOW Vbat"              // was 3.67V
// =========================================================================
	  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // display voltage in 'inverse' text'
      display.setCursor(5, 8);
      display.println(cur_measuredvbat);
      display.setCursor(30, 8);
      display.print("V");
	  display.setTextColor(SSD1306_WHITE);	// stop inverse text,
    } 
	else {								// if Vbat not "low" - display normal color.
      display.setCursor(5, 8);
      display.println(cur_measuredvbat);
      display.setCursor(30, 8);
      display.print("V");
    }

    display.setCursor(55, 8);
    display.print(cur_humidity, 1);
    display.setCursor(80, 8);
    display.print("%");
    display.setCursor(95, 8);
    display.print(cur_temperature, 1);
    display.setCursor(120, 8);
    display.print("C");

    display.setCursor(90, 16);
    display.println(" CO2");
    display.setCursor(90, 24);
    display.println(" ppm");

    display.setTextSize(2);
    display.setCursor(16, 17);
    display.print(cur_co2, 1);
    display.display();
// ===============================================================================================================
}

void PrintRS232InfoScreen ()
{
#ifdef RS232_info
      Serial.print("version ");
      Serial.println(verNumber);

      Serial.print("compile date: ");
      Serial.print(F(__DATE__));  // date this sketch was compiled
      Serial.print(", time: ");
      Serial.print(F(__TIME__));  // time this sketch was compiled
      Serial.println();

      String dataString = "";
      DateTime now = rtc.now();
      dataString += String(now.year(), DEC);
      dataString += '/';
      dataString += String(now.month(), DEC);
      dataString += '/';
      dataString += String(now.day(), DEC);
      dataString += ",";
      dataString += String(now.hour(), DEC);
      dataString += ':';
      dataString += String(now.minute(), DEC);
      dataString += ':';
      dataString += String(now.second(), DEC);
      Serial.println(dataString);

      if (scd30.selfCalibrationEnabled()) {
        Serial.print("Self calibration enabled");
      } else {
        Serial.print("Self calibration disabled");
      }
      Serial.println();

      Serial.print("Measurement Interval: ");
      Serial.print(scd30.getMeasurementInterval());
      Serial.println(" seconds");

      Serial.print("Forced Recalibration reference: ");
      Serial.print(scd30.getForcedCalibrationReference());
      Serial.println(" ppm");

      Serial.print("Altitude offset: ");
      Serial.print(scd30.getAltitudeOffset());
      Serial.println(" meters");

      Serial.print("Temperature offset: ");
      Serial.print((float)scd30.getTemperatureOffset() / 100.0);
      Serial.println(" degrees C");
#endif
  
}

void PrintOLEDInfoScreen ()    // print OLED DATA for info screen.
{
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 0);
      display.print("version");
      display.setCursor(90, 0);
      display.println(verNumber, DEC);

      cur_measuredvbat = analogRead(VBATPIN);
      cur_measuredvbat *= 2;    // we divided by 2, so multiply back
      cur_measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
      cur_measuredvbat /= 1024; // convert to voltage

      display.setCursor(0, 15);
      display.print("battery");
      display.setCursor(90, 15);
      display.println(cur_measuredvbat, 1);

      display.display();
}

void ReadAllSensors ()    // Read all sensors once in 
{
        if (scd30.dataReady()) {
          if (!scd30.read()) {
#ifdef RS232_Debug
            Serial.println("Error reading SCD30 sensor data");
#endif
            cur_temperature = 99.99;    // Default value for read error.
            cur_humidity = 99.99;       // Default value for read error.
            cur_co2 = 9999.9;                // Default value for read error.
//            cur_measuredvbat = 9.99;    // Default value for read error.
            return;
          }
// ============= add average for stability by Ofer ===========================
          cur_temperature = (cur_temperature+scd30.temperature)*0.5;    // average with last result for stability.
          cur_co2 = (cur_co2 + scd30.CO2)*0.5;
          cur_humidity = (cur_humidity + scd30.relative_humidity)*0.5;
      
          cur_measuredvbat = analogRead(VBATPIN);
          cur_measuredvbat *= 2;    // we divided by 2, so multiply back
          cur_measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
          cur_measuredvbat /= 1024; // convert to voltage
// ===========================================================================

        } //end if (scd30.dataReady())
  
}


// Routine to display and set a parameter (with up to 4 digits) on screen, each digit is setup by it's turn.
// Please make sure to fill the DIGITS array's accordingly
// Start_X and Start_Y are the coordination where digit[0] will print on screen - others will follow to the right.
// DigitsXoffset is the print offset to next digit - i.e character distance between digits.
// NumberOfDigits is the total number of digits to be display and set (starting with digit[0] on left and up to digit[4] on the right
// each marked digit can be setup by navigation key, pressing the "enter" key will proceed to next digit.
// NextMenuItem - indicates what digit is currently under SET.
// note - current digit on setup is marked in reverse text color.
void DisplaySetDigits (unsigned char Start_X , unsigned char Start_Y , unsigned char NumberOfDigits , unsigned char DigitsXoffset , unsigned char FontSize)
{
    char i;
    display.setTextSize(FontSize);				// print with requested font size
	for (i=0; i<NumberOfDigits; ++i) {			// for all digits up to NumberOfDigits
		if (NextMenuItem == (i)){				// check if this digit is selected for setup
			display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text for SETUP digit
			Digits[i] = KeyNavigation (DigitsMin[i] , DigitsMax[i] , Digits[i], true);		// update if digit changed.
		} else {
			display.setTextColor(SSD1306_WHITE);	// stop inverse text for all other digits,
		}		
		display.setCursor((Start_X+i*DigitsXoffset), Start_Y);	// determine place to print digit
		if (Digits[i] <= 9 && DigitsMax[i] >= 10) {	// check if leading zero required (i.e. - number need >=2 digits)
			display.print("0");					// add leading zero.
		}
		display.print(Digits[i],DEC);							// print digit on screen
		display.setTextColor(SSD1306_WHITE);	// stop inverse text for all other digits,
	}
}

// this function is called by SD if it wants to know the time.
// for example, when it needed to assign "creation date" of file on SD.
void dateTime(uint16_t* date, uint16_t* time) {
  DateTime now = rtc.now();

  // return date using FAT_DATE macro to format fields
  *date = FAT_DATE(now.year(), now.month(), now.day());

  // return time using FAT_TIME macro to format fields
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}
