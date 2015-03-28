/*********************************************************************
				Solder Reflow Oven Firmware v0.1

This firmware handles input from a 5-direction joystick switch, a
momentary start switch, output of reflow parameters to a Nokia 5110 
LCD display, and PID control of a toaster oven heating elements for
solder reflowing. See https://github.com/mploof/Reflow_Oven for more
details.

*********************************************************************/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>

// Software SPI (slower updates, more flexible pin options):
// pin 7 - Serial clock out (SCLK)
// pin 6 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2
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
bool position_selected = false;


/*** Cursor Constants and Vars ***/

#define ROW_HEIGHT 8
#define COL_WIDTH 5

#define SOAK_TEMP 0
#define SOAK_TIME 1
#define REFLOW_TEMP 2
#define REFLOW_TIME 3

const int MIN_POSITION = SOAK_TEMP;
const int MAX_POSITION = REFLOW_TIME; // Make sure to update this if more positions are added
int cursor_position = SOAK_TEMP;


/*** Reflow Program Parameters ***/

int soak_temp			= 160;		// Temp in deg C
int soak_time			= 40;		// Time in seconds
int reflow_temp			= 215;		// Temp in deg C
int reflow_time			= 55;		// Time in seconds
bool program_running	= false;

// Debugging state
bool debug = true;

static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };

void setup()   {

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

	// Start LCD display process and set contrast
	display.begin();
	display.setContrast(35);

	// Show splash screen, wait, then clear screen
	display.display();
	delay(500);				
	display.clearDisplay();   

	// Display initial temp and time info
	updateDisplay();

	// Listen for start program button
	attachInterrupt(0, inputISR, RISING);

}

void loop() {

	// Until the program starts, just check user input and update the menu
	if (!program_running){
		menuUpdate();
		debugOutput();
	}
	// Otherwise start the program
	else {

		display.clearDisplay();
		display.setTextSize(2);
		display.setCursor(0, 0);
		display.print("Program");
		display.println("Running");
		display.display();

		while (program_running) {
			programUpdate();
			debugOutput();
		}

		updateDisplay();
	}
}

void menuUpdate() {

	if (joystick_direction != RELEASED) {
		// Deal with joystick input
		joystickHandler();
		// Refresh the display with new info
		updateDisplay();
	}
}

void programUpdate() {
	delay(10);
	return;
}

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
			Serial.println(cursor_position);
			Serial.print("Position selected? ");
			Serial.println(position_selected);
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

		 Menu Functions

**********************************/

void joystickHandler() {	

	switch (joystick_direction) {

	case UP:
		// Move the cursor up
		if (!position_selected){
			cursor_position--;
			if (cursor_position < MIN_POSITION)
				cursor_position = MAX_POSITION;
		}
		// Increase the value of the selected parameter
		else
			updateValues(UP);
		break;

	case DOWN:
		// Move the cursor down
		if (!position_selected){
			cursor_position++;
			if (cursor_position > MAX_POSITION)
				cursor_position = MIN_POSITION;
		}
		// Decrease the value of the selected parameter
		else
			updateValues(DOWN);
		break;

	case LEFT:
		position_selected = !position_selected;
		break;

	case RIGHT:
		position_selected = !position_selected;
		break;

	case SELECT:
		position_selected = !position_selected;
		break;

	}

	// Return the joystick to released state
	joystick_direction = RELEASED;
}

void updateValues(byte p_input) {
	int increment;

	if (p_input == UP)
		increment = 1;
	else if (p_input == DOWN)
		increment = -1;
	else
		increment = 0;

	switch (cursor_position){
	case SOAK_TEMP:
		soak_temp += increment*5;
		break;
	case SOAK_TIME:
		soak_time += increment;
		break;
	case REFLOW_TEMP:
		reflow_temp += increment*5;
		break;
	case REFLOW_TIME:
		reflow_time += increment;
		break;
	}
}

void updateDisplay() {
	
	const int CURSOR_OFFSET = 2;

	display.clearDisplay();

	// Set system parameters first
	display.setTextColor(BLACK);
	display.setCursor(0, 0);
	display.setTextSize(1);
	display.println("Cooking Params");
	display.print(" S Temp: ");
	display.print(soak_temp);
	display.println("C");
	display.print(" S Time: ");
	display.print(soak_time);
	display.println("s");
	display.print(" R Temp: ");
	display.print(reflow_temp);
	display.println("C");
	display.print(" R Time: ");
	display.print(reflow_time);
	display.println("s");
	
	// Then update the cursor position	
	if (position_selected) {
		display.setCursor(9 * COL_WIDTH, (cursor_position + CURSOR_OFFSET) * ROW_HEIGHT);
		display.print("=");
	}
	else {
		display.setCursor(0, (cursor_position + CURSOR_OFFSET) * ROW_HEIGHT);
		display.print("*");
	}

	// Display the updated screen
	display.display();
}
