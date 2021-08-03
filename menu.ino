// ====== Functions and dependencies: =======================
//
// MenuNavigation <-- KeyNavigation <-- CheckKey
//										   |
// SetupValueNavigation <------------------+
//										   |
// navigation <----------------------------┘
// 
// displaymenu <-- PrintMenuHeader
//
// ==========================================================
//
// Menu Layout on screen:
// there are 3 layout types of the screen, the first is for navigation menu.
//	┌———————————————————————————————————┐
//  │ Time field				M.N		│ where time= hh:mm:ss , M is the current MenueState , N is the Next item select.
//  │ 1) first menu						│
//  │ 2) second menu					│ Menu text is taken from MenuStateText[].
//	│ 3) Back							│
//	└———————————————————————————————————┘
//
// the second screen layout is for Setting a parameter value:
//	 ___________________________________
//  │ Time field				M.N		│ where time= hh:mm:ss , M is the current MenueState , N is the Next item select.
//  │ help text							│ Line 2 for presenting text to user.
//  │ |¯| |¯| |¯| |¯|					│Line 3,4 are for presenting the value in font size=2.
//	│ |_| |_| |_| |_|					│
//	└———————————————————————————————————┘
//
// the last menu layout is the IDLE screen where all information is displayed:
//	┌———————————————————————————————————┐
//  │ Time field		 SD information │ where time= hh:mm:ss , SD information is either "LOGGING" or "NO SD"
//  │  VBAT		Humidity	Temperature	│
//  │ |¯| |¯| |¯| |¯|		CO2			│Line 3,4 are for presenting the value in font size=2.
//	│ |_| |_| |_|.|_|		PPM			│
//	└———————————————————————————————————┘
//


// ================= CheckKey - routine to check for key press. ==========================
// State machine for detecting key press:
//
// ┌——┐ Key    ┌——┐ DeBounce ┌——┐ Application
// │0 │———————>│1 │—————————>│2 │——————————————┐
// └——┘ Press  └——┘ Timer    └——┘ Polling      │
//   ^                         │      		   V
//   │\_______________________/		   ┌———————————┐
//   │ 		Key released	  \        │Application│
//   ^						   ^       └———————————┘
//   │                         |               │
//  __  Rapid   __   Long     __  Application  │
// |8 +———————>|3 |<—————————+4 |<——————————-——┘
// |__| Timer  |__|  Press   |__|  Response
//   ^           │   Timer
//   │           V
//  ______________
// | Application  |
// |______________|
//
// Application should check periodically for KEY?_STATE, if it is 2 or 3 (indicating a press or rapid press), than application should response
// with KEY?_STATE =4 or =8 correspondingly to acknowledge key press was handled.
//
// KeyA (UP) and KeyC (DOWN) have a rapid press function - i.e. when holding the key for long time
// it provide a virtual press indication each 600mS.
// For KeyB (ENTER) - application should response only to Key?_state=2 (indicating Key press) with changing to Key?_state=4 
//					  (i.e. only once, ignoring the virtual press indications at state 3)
// For KeyA and KeyB - Same as KeyB and in addition application can respond also for Key?_state=3 (indicating Rapid Virtual Press) 
//					   with changing Key?_state=8 for continues Rapid Virtual Press
// after key is released routine reset the state machine.
// The value in KEY?_STATE will maintained until changed by the navigation routine for either "first press" when Key?_state=2 
// or virtual press when Key?_state=3.

void CheckKey (void) {
  if (digitalRead(BUTTON_A) == false) {
    if (KeyA_state==0) {		// if start pressing a key - mark the start time of press.
		KEY_A_Time = millis();   	// mark the "start of press" time
		KeyA_state = 1;           // move key state to "DE-bounce time check"
    } else if (KeyA_state==1 && ((millis()-KEY_A_Time) >= DebuonceTime)) { // if valid first press detected:
		KeyA_state = 2;          // move key state to "Short Pressed" (application response with KeyB_state=4)
	} else if (KeyA_state==4 && ((millis()-KEY_A_Time) >= LongPress)) {
		KeyA_state = 3;          // move key state to "continues rapid press"
		KEY_A_Time = millis();   // mark the "continues rapid press" time	  
	} else if (KeyA_state==8 && ((millis()-KEY_A_Time) >= 600)) {		// if "continues rapid press" time pass 
		KeyA_state = 3;          // move key state to "Long Pressed"
		KEY_A_Time = millis();   // mark the "continues rapid press" time	  
	}  
  } else {					// when key released or not pressed
		KeyA_state = 0; 	// clear state machine
		KEY_A_Time = 0;		// After key released - stop counting "press time".
  }
  
  if (digitalRead(BUTTON_B) == false) {
    if (KeyB_state==0) {		// if start pressing a key - mark the start time of press.
		KEY_B_Time = millis();   	// mark the "start of press" time
		KeyB_state = 1;           // move key state to "DE-bounce time check"
    } else if (KeyB_state==1 && ((millis()-KEY_B_Time) >= DebuonceTime)) { // if valid first press detected:
		KeyB_state = 2;          // move key state to "Short Pressed" (application response with KeyB_state=4)
	} else if (KeyB_state==4 && ((millis()-KEY_B_Time) >= LongPress)) {
		KeyB_state = 3;          // move key state to "continues rapid press"
		KEY_B_Time = millis();   // mark the "continues rapid press" time	  
	} else if (KeyB_state==8 && ((millis()-KEY_B_Time) >= 600)) {		// if "continues rapid press" time pass 
		KeyB_state = 3;          // move key state to "Long Pressed"
		KEY_B_Time = millis();   // mark the "continues rapid press" time	  
	}  
  } else {					// when key released or not pressed
		KeyB_state = 0; 	// clear state machine
		KEY_B_Time = 0;		// After key released - stop counting "press time".
  }
  
  if (digitalRead(BUTTON_C) == false) {
    if (KeyC_state==0) {		// if start pressing a key - mark the start time of press.
		KEY_C_Time = millis();   	// mark the "start of press" time
		KeyC_state = 1;           // move key state to "DE-bounce time check"
    } else if (KeyC_state==1 && ((millis()-KEY_C_Time) >= DebuonceTime)) { // if valid first press detected:
		KeyC_state = 2;          // move key state to "Short Pressed" (application response with KeyB_state=4)
	} else if (KeyC_state==4 && ((millis()-KEY_C_Time) >= LongPress)) {
		KeyC_state = 3;          // move key state to "continues rapid press"
		KEY_C_Time = millis();   // mark the "continues rapid press" time	  
	} else if (KeyC_state==8 && ((millis()-KEY_C_Time) >= 600)) {		// if "continues rapid press" time pass 
		KeyC_state = 3;          // move key state to "Long Pressed"
		KEY_C_Time = millis();   // mark the "continues rapid press" time	  
	}  
  } else {					// when key released or not pressed
		KeyC_state = 0; 	// clear state machine
		KEY_C_Time = 0;		// After key released - stop counting "press time".
  }
  
}
// ================= end CheckKey - routine to check for key press. ======================


//======== print header ==========================================
// this routine prepare the header line for each menu screen. it will display:
// 1) Current time (live updated).
// 2) Current Menu number "." Next picked item number
void PrintMenuHeader (void)
{
	display.clearDisplay();		// clear display buffer
    display.setTextColor(SSD1306_WHITE);  // regular text,
    display.setTextSize(1);		// use small font
    display.setCursor(0, 0);	// start at top left
    DateTime now = rtc.now();	// find time.
    display.print(now.timestamp(DateTime::TIMESTAMP_TIME));
    display.setCursor(64, 0);	// next field to update.
    display.print("Menu ");
    display.print(MenuState,DEC);
    display.setCursor(106, 0);
    display.print(".");
    display.print(NextMenuItem,DEC);
}
//=========== end header ========================================


// Routine to set a parameter as per UP or DOWN key press.
// The routine returned the updated parameter value according to key press detected.
// The routin will responce to both short press and Rapid virtual continues key press.
// count direction should be "false" for menu navigation, since pressing "UP" when point to "last line" (i.e. 3) mean you should point to "last line"-1 (i.e. 2).
// For parameter value setup CountDirection=true,, since pressing "UP" when point to "parameter value" mean you should return - "parameter value"+1. 
int KeyNavigation (int ParameterMin , int ParameterMax , int Parameter, bool CountDirection)
{
	if ((CountDirection == false && KeyA_state == 2)||(CountDirection == true && KeyC_state == 2)) {	// if short press UP key
		if (Parameter <= ParameterMin) {	    // check if need to roll over
            if (CountDirection == false) {      // check direction to determine what key was pressed.
			    KeyA_state = 4;					// responce with proper key state.
            }else {                             // if other count direction
                KeyC_state = 4;                 // responce with proper key state.
            }
			return ParameterMax;				// returned the roll over number
		} else {							// if no need for roll over
            if (CountDirection == false) {      // check direction to determine what key was pressed.
                KeyA_state = 4;                 // responce with proper key state.
            }else {                             // if other count direction
                KeyC_state = 4;                 // responce with proper key state.
            }
			return Parameter-1;				// calculate new value and return it.
		}
	} else  if ((CountDirection == false && KeyC_state == 2)||(CountDirection == true && KeyA_state == 2)) {    // if short press DOWN key
        if (Parameter >= ParameterMax) {        // check if need to roll over
            if (CountDirection == false) {      // check direction to determine what key was pressed.
                KeyC_state = 4;                 // response with proper key state.
            }else {                             // if other count direction
                KeyA_state = 4;                 // response with proper key state.
            }
            return ParameterMin;                // returned the roll over number
        } else {                            // if no need for roll over
            if (CountDirection == false) {      // check direction to determine what key was pressed.
                KeyC_state = 4;                 // response with proper key state.
            }else {                             // if other count direction
                KeyA_state = 4;                 // response with proper key state.
            }
            return Parameter+1;             // calculate new value and return it.
        }
//  enable below section to operate on RAPID press as well: (never checked)
/*
    if ((CountDirection == false && KeyA_state == 3)||(CountDirection == true && KeyC_state == 3)) {    // if short press UP key
        if (Parameter <= ParameterMin) {        // check if need to roll over
            if (CountDirection == false) {      // check direction to determine what key was pressed.
                KeyA_state = 8;                 // response with proper key state.
            }else {                             // if other count direction
                KeyC_state = 8;                 // response with proper key state.
            }
            return ParameterMax;                // returned the roll over number
        } else {                            // if no need for roll over
            if (CountDirection == false) {      // check direction to determine what key was pressed.
                KeyA_state = 8;                 // response with proper key state.
            }else {                             // if other count direction
                KeyC_state = 8;                 // response with proper key state.
            }
            return Parameter-1;             // calculate new value and return it.
        }
    } else  if ((CountDirection == false && KeyC_state == 3)||(CountDirection == true && KeyA_state == 3)) {    // if short press DOWN key
        if (Parameter >= ParameterMax) {        // check if need to roll over
            if (CountDirection == false) {      // check direction to determine what key was pressed.
                KeyC_state = 8;                 // response with proper key state.
            }else {                             // if other count direction
                KeyA_state = 8;                 // response with proper key state.
            }
            return ParameterMin;                // returned the roll over number
        } else {                            // if no need for roll over
            if (CountDirection == false) {      // check direction to determine what key was pressed.
                KeyC_state = 8;                 // response with proper key state.
            }else {                             // if other count direction
                KeyA_state = 8;                 // response with proper key state.
            }
            return Parameter+1;             // calculate new value and return it.
        }
 */
	} else {
    return Parameter;
	}
}

// Function to navigate through menustate. when UP or DOWN key are pressed - it will change "NextMenuItem" accordingly,
// and when ENTER is pressed - it will use "NextMenuItem" value to change to correspondence "MenuState" 
// according to list in MenuStateTree[].
void MenuNavigation (int NoOfMenuItems) {
// ========= check if navigation key pressed ==============================================	
	NextMenuItem = KeyNavigation (0 , (NoOfMenuItems-1) , NextMenuItem, false);	// enable 3 cursor navigation for NextMenuItem.
// ========= check if sub-menu choose than go to correspondence MenuState ============================
	if (KeyB_state==2) { 		// if keyB (Enter) pressed:
		MenuState = MenuStateTree[(MenuState -10)*3 + NextMenuItem];
		NextMenuItem = 2;		// before jumping to new screen, mark last item.
		KeyB_state = 4;         // clear key state 
#ifdef RS232_Debug				// print navigation results on RS232
	Serial.print("MenuState= ");
	Serial.print(MenuState,DEC);
	Serial.print("NextMenu= ");
	Serial.println(NextMenuItem,DEC);
#endif
	}
}


// function to check if last "ENTER" pressed after setting all digits of a parameter value,
// and then change menu state to the required "Return" menu state.
void SetupValueNavigation (char NumberOfDigits, unsigned char ReturnMenu)
{
// =========== digits navigation ========================
	if (KeyB_state==2) { 		// if keyB (Enter) pressed:
		NextMenuItem +=1;			// go to next digit
		if (NextMenuItem >= NumberOfDigits) {		// if last digit was set
		NextMenuItem = 2;		// point on last item
		MenuState = ReturnMenu;			// return to previous menu
		}
		KeyB_state = 4;         // clear key state 
	}
// ============== end of digit navigation ===============
}

// this routine uses "MenuStae" and "NextMenuItem" to Print into OLED the requested MENU text (stored in "MenuStateText[]")
// it also mark the currently selected menu item as per "NextMenuItem". the number of lines printed per individual menu are 
// transfered as a parameter.
// Menu text is stored in MenuStateText[] array, starting with first line of MenuState=10 stored in MenuStateText[0]
void displaymenu(int NoOfMenuItems, bool DisplayCursor, bool InverseMenuColor) {
// ============ handle correct display according to menustate and selected item ================
    strip.setPixelColor(0, strip.Color(0, 0, 128));
    strip.show();
	PrintMenuHeader ();			// display menu header on first screen line. 
	display.setCursor(0, 8);	// display help for setting on 2'nd line
	for (int j = 0; j < NoOfMenuItems; j++)	{	// for the all menu lines
        int TEMP = int((j+1)*8);		// calculate pixel line (Y coordination) on display
        int X=0;                        // start print at X location 0
        if (DisplayCursor== true) {     // if cursor needed
            X=7;                        // start printing at x location 7, leaving space on left for cursor.
        }
        if (InverseMenuColor == true) {
            display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text for next printing
        }
	    if (NextMenuItem == j && DisplayCursor==true) {	// if this line is selected by navigation keys.
		    display.setCursor(0,TEMP);
		    display.print(">");		// display marker for chosen item.
	    } 
		display.setCursor(X, TEMP);	// change position into menu area.
		display.print(MenuStateText[int((MenuState-11)*NoOfMenuItems+j)]);	// display correspondence menu line.
	}								// repeat for all menu lines.
    display.setTextColor(SSD1306_WHITE);    // stop inverse text for all other printing,
	display.display();  			// update screen with all changes  
// ============================================================================================	
}




// FirstNext - is the next MenuState if first menu item choose.
// SecondNext - is the next MenuState if second menu item choose.

void navigation (unsigned char FirstNext , unsigned char SecondNext, unsigned char ThirdNext) 
{
	NextMenuItem = KeyNavigation (1 , 3 , NextMenuItem, false);	// enable 3 cursor navigation for NextMenuItem.
// ========= check if KeyB than go to correspondence MenuState ============================
	if (KeyB_state==2) { 		// if keyB (Enter) pressed:
		if (NextMenuItem == 3) {		// if "Back"
			MenuState = ThirdNext;		// return to Higher Level screen
		} else if (NextMenuItem == 1) {
			MenuState = FirstNext;		// go to Lowe Level 1st screen
		} else if (NextMenuItem == 2) {
			MenuState = SecondNext;		// go to Lower Level 2nd screen
		}
	NextMenuItem = 1;		// before jumping to new screen, mark first item.
	KeyB_state = 4;         // clear key state 
#ifdef RS232_Debug
	Serial.print("MenuState= ");
	Serial.print(MenuState,DEC);
	Serial.print("NextMenu= ");
	Serial.println(NextMenuItem,DEC);
#endif
	}
// ============ change menu indication for selected item ================
}

