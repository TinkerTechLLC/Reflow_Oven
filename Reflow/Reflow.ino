/*********************************************************************
				Solder Reflow Oven Firmware v0.1

This firmware handles input from a 5-direction joystick switch, a
momentary start switch, output of reflow parameters to a Nokia 5110 
LCD display, and PID control of a toaster oven heating elements for
solder reflowing. See https://github.com/mploof/Reflow_Oven for more
details.

*********************************************************************/

#include "Arduino.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include "menu.h"


#define JOYSTICK_OUTPUTS 5
#define JOYSTICK_START_PIN 8

#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16


/*** Joystick & Button Constants and Vars ***/

#define RELEASED 0
#define UP 8
#define DOWN 9
#define LEFT 10
#define RIGHT 11
#define SELECT 12
#define BUTTON_PIN 13
byte joystick_direction = RELEASED;
byte last_joystick_direction = RELEASED;

// Debugging state
bool debug = true;

// Create menu object
Menu menu;

/*** Reflow Program Parameters ***/

int soak1_temp = 160;		// Temp in deg C
int soak1_time = 40;		// Time in seconds
int soak2_temp = 160;		// Temp in deg C
int soak2_time = 40;		// Time in seconds
int reflow_temp = 215;		// Temp in deg C
int reflow_time = 55;		// Time in seconds
bool program_running = false;


/*********************************

		Menu Contents

**********************************/

const int ITEM_COUNT = 6;

Menu::element menu_items[ITEM_COUNT] = {

	// Name		Value			Unit	Min		Max		Increment
	{ "1 Temp", &soak1_temp,	"C",	100,	200,	5 },
	{ "1 Time", &soak1_time,	"s",	10,		200,	1 },
	{ "2 Temp", &soak2_temp,	"C",	100,	200,	5 },
	{ "2 Time", &soak2_time,	"s",	10,		200,	1 },
	{ "3 Temp", &reflow_temp,	"C",	100,	275,	5 },
	{ "3 Time", &reflow_time,	"s",	10,		200,	1 },

};

void setup()   {


	// Setup the menu
	menu.setContents(menu_items, ITEM_COUNT);

	// Set pin modes
	pinMode(2, INPUT);
	for (byte i = JOYSTICK_START_PIN; i < (JOYSTICK_START_PIN + JOYSTICK_OUTPUTS + 1); i++) {
		pinMode(i, INPUT);
	}
	for (byte i = 3; i <= 7; i++) {
		pinMode(i, OUTPUT);
	}
  
	// Start serial connection
	Serial.begin(9600);

	// Display initial temp and time info
	menu.refresh();

	// Listen for start program button
	attachInterrupt(0, inputISR, RISING);

	// Show splashscreen
	menu.display.display();
	delay(1000);
	menu.clear();

}

void loop() {

	// Until the program starts, just check user input and update the menu
	if (!program_running){
		menuUpdate();
		debugOutput();
	}

	// Otherwise start the program
	else {

		menu.clear();
		menu.display.setTextSize(2);
		menu.display.setCursor(0, 0);
		menu.display.print("Program");
		menu.display.println("Running");
		menu.display.display();

		while (program_running) {
			debugOutput();
		}

		menu.refresh();
	}
}

void menuUpdate() {

	if (joystick_direction != RELEASED) {
		// Deal with joystick input
		joystickHandler();
		// Refresh the display with new info
		menu.refresh();
	}
}


/*********************************

	 Interrupt Service Routine

**********************************/

void inputISR() {

	static unsigned long last_call = millis();

	// Check the start progam button input. If it's high, flip the program running state
	if (digitalRead(BUTTON_PIN) == HIGH) {
		// Button debouncing
		if (millis() - last_call < 500) {
			last_call = millis();
			return;
		}
		program_running = !program_running;
		last_call = millis();
		return;
	}

	// Check each of the joystick inputs and determine which direction was pressed
	if (!program_running) {
		// Joystick debouncing
		if (millis() - last_call < 200) {
			last_call = millis();
			return;
		}
		for (byte i = JOYSTICK_START_PIN; i < (JOYSTICK_START_PIN + JOYSTICK_OUTPUTS); i++) {
			if (digitalRead(i) == HIGH){
				joystick_direction = i;
				last_joystick_direction = i;
				last_call = millis();
				return;
			}
		}
	}
}


/*********************************

		 Joystick Handler

**********************************/

void joystickHandler() {	

	switch (joystick_direction) {

	case UP:
		menu.up();
		break;

	case DOWN:
		menu.down();
		break;

	case LEFT:
		menu.select();
		break;

	case RIGHT:
		menu.select();
		break;

	case SELECT:
		menu.select();
		break;

	}

	// Return the joystick to released state
	joystick_direction = RELEASED;
}

/*********************************

		Debug Functions

**********************************/

void debugOutput() {

	// If debug mode is not enabled, don't output info
	if (!debug)
		return;

	static unsigned long time = millis();

	if (millis() - time > 1000){
		if (!program_running){
			Serial.print("Joystick direction: ");
			Serial.println(last_joystick_direction);
			Serial.print("Current position: ");
			Serial.println(menu.m_cursor_pos);
			Serial.print("Position selected? ");
			Serial.println(menu.m_element_selected);
			Serial.println("");
			last_joystick_direction = RELEASED;
		}
		else {
			Serial.println("Progam still running!");
			Serial.println("");
		}

		time = millis();
	}
}