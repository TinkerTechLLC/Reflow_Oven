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
#include <PID_v1.h>


#define JOYSTICK_OUTPUTS 4
#define JOYSTICK_START_PIN 8
#define MILLIS_PER_SEC 1000


/*** Joystick & Button Constants and Vars ***/

#define RELEASED 0
#define UP 8
#define DOWN 9
#define LEFT 10
#define RIGHT 11
//#define SELECT 12
#define BUTTON_PIN 13
byte joystick_direction = RELEASED;
byte last_joystick_direction = RELEASED;

const int INTERRUPT_PIN = 2;

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


/********************************************

	Thermocouple / PID Vars and Constants

*********************************************/

const int THERM_PIN = A0;
const int RELAY_PIN = 12;
const float ANALOG_REF = 5.0;	// Analog reference voltage
double current_temp = 0;
double output = 0;
double target_temp = 0;

double kp = 2.0;                // (P)roportional Tuning Parameter
double ki = 0.2;                // (I)ntegral Tuning Parameter
double kd = 0.5;                // (D)erivative Tuning Parameter

PID pid(&current_temp, &output, &target_temp, kp, ki, kd, DIRECT);

int pid_window_size = 5000;	// PID "PWM" cycle time. PID will set a duty cycle of this time window in milliseconds.
unsigned long pid_window_start_time;

void setup()   {
	
	// Start serial connection
	Serial.begin(9600);

	// Set pin modes
	pinMode(THERM_PIN, INPUT);																	// Set thermocouple analog input pin
	pinMode(INTERRUPT_PIN, INPUT);																// Set common joystick / button interrupt pin
	for (byte i = JOYSTICK_START_PIN; i < (JOYSTICK_START_PIN + JOYSTICK_OUTPUTS + 1); i++) 	// Set joystick input pins
		pinMode(i, INPUT);
	for (byte i = 3; i <= 7; i++)																// Set LCD output pins
		pinMode(i, OUTPUT);

	// Listen for joystick / button input
	attachInterrupt(0, inputISR, RISING);

	// Setup menu
	menu.begin();
	menu.setContents(menu_items, ITEM_COUNT);
	menu.setHeader("PRGM PARAMS");
	menu.refresh();
	
}

void loop() {

	// Until the program starts, just check user input and update the menu
	if (!program_running){
		menuCheck();
		debugOutput();
	}

	// Otherwise start the program
	else {
		runProgram();
	}
}

void menuCheck() {

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

	//case SELECT:
	//	menu.select();
	//	break;

	}

	// Return the joystick to released state
	joystick_direction = RELEASED;
}

/*********************************

	 Program  & PID Functions

**********************************/

void runProgram(){

	// Start PID
	pid.SetOutputLimits(0, pid_window_size);
	pid.SetMode(AUTOMATIC);

	// Show program running text
	menu.clear();
	menu.display.setTextSize(2);
	menu.display.setCursor(0, 0);
	menu.display.print("Program");
	menu.display.println("Running");
	menu.display.display();

	// Declare timing variables	
	unsigned long transition_1 = soak1_time * MILLIS_PER_SEC;
	unsigned long transition_2 = (soak1_time + soak2_time) * MILLIS_PER_SEC;
	unsigned long total_run_time = (soak1_time + soak2_time + reflow_time) * MILLIS_PER_SEC;
	
	unsigned long start_time = millis();
	unsigned long running_time = 0;
	pid_window_start_time = millis();

	while (program_running) {
		
		// Determine which phase we're in and set the appropriate target temperature 
		if (running_time < transition_1)
			target_temp = soak1_temp;
		else if (running_time > transition_1 && running_time <= transition_2)
			target_temp = soak2_temp;
		else if (running_time > transition_2)
			target_temp = reflow_temp;
		else if (running_time > total_run_time)					
			break;
		
		updateTemp();
		pid.Compute();
		
		// If one full PID window has passed, advance to the next
		if (millis() - pid_window_start_time > pid_window_size) {
			pid_window_start_time += pid_window_size;
		}

		// Determine whether we're in an on or off phase of the duty cycle
		if (output < millis() - pid_window_start_time)
			digitalWrite(RELAY_PIN, HIGH);
		else
			digitalWrite(RELAY_PIN, LOW);

		debugOutput();

		running_time = millis() - start_time;
	}

	// End program
	target_temp = 0;				// Reset target temp
	pid.SetMode(MANUAL);			// Turn off PID
	digitalWrite(RELAY_PIN, LOW);	// Turn off heater
	program_running = false;		// Turn off program running flag
	
	// Return to main menu
	menu.refresh();
}

void updateTemp(){

	// Set the current temperature variable based upon the arcane magic 
	// of the AD8495 thermocouple amplifier converstion equation. 
	// See https://blog.adafruit.com/2014/03/21/new-product-analog-output-k-type-thermocouple-amplifier-ad8495-breakout/

	current_temp = (((float)analogRead(A0) / 1023.0) * ANALOG_REF) - 1.25 / 0.005;

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