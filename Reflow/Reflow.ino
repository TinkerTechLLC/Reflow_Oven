/*********************************************************************
This is an example sketch for our Monochrome Nokia 5110 LCD Displays

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/338

These displays use SPI to communicate, 4 or 5 pins are required to
interface

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
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

// Hardware SPI (faster, but must use certain hardware pins):
// SCK is LCD serial clock (SCLK) - this is pin 13 on Arduino Uno
// MOSI is LCD DIN - this is pin 11 on an Arduino Uno
// pin 5 - Data/Command select (D/C)
// pin 4 - LCD chip select (CS)
// pin 3 - LCD reset (RST)
// Adafruit_PCD8544 display = Adafruit_PCD8544(5, 4, 3);
// Note with hardware SPI MISO and SS pins aren't used but will still be read
// and written to during SPI transfer.  Be careful sharing these pins!

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2
#define JOYSTICK_OUTPUTS 5
#define JOYSTICK_START_PIN 8

#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16

// Joystick Directions
#define UP 8
#define DOWN 9
#define LEFT 10
#define RIGHT 11
#define SELECT 12
#define RELEASED 0

byte joystick_direction = RELEASED;
byte last_joystick_direction = RELEASED;

// Cursor positions
#define SOAK_TEMP 0
#define SOAK_TIME 1
#define REFLOW_TEMP 2
#define REFLOW_TIME 3
const byte MIN_POSITION = SOAK_TEMP;
const byte MAX_POSITION = REFLOW_TIME; // Make sure to update this if more positions are added

byte cursor_position = SOAK_TEMP;

// Reflow parameters
int soak_temp = 160;
int soak_time = 40;
int reflow_temp = 215;
int reflow_time = 55;

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

  Serial.begin(9600);
  display.begin();
  // Starrt listen for joystick input
  attachInterrupt(0, joystickISR, FALLING);
  // init done

  // you can change the contrast around to adapt the display
  // for the best viewing!
  display.setContrast(35);

  display.display(); // show splashscreen
  delay(2000);
  display.clearDisplay();   // clears the screen and buffer

  // Display temp and time info
  updateDisplay();

}


void loop() {

	static unsigned long time = millis();

	if (joystick_direction != RELEASED) {
		joystickHandler();
	}

	if (millis() - time > 1000){
		Serial.print("Joystick direction: ");
		Serial.println(last_joystick_direction);
		last_joystick_direction = RELEASED;
		time = millis();
	}  
}

void joystickISR() {

	//Serial.println("Interrupt detected!");

	// Check each of the joystick inputs and determine which direction was pressed
	for (byte i = JOYSTICK_START_PIN; i < (JOYSTICK_START_PIN + JOYSTICK_OUTPUTS); i++) {
		if (digitalRead(i) == HIGH){
			joystick_direction = i;
			last_joystick_direction = i;
			return;
		}
	}
}

void joystickHandler() {

	static bool position_selected = false;

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
			if (cursor_position < MAX_POSITION)
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

	// Refresh the screen with new info
	updateDisplay();
	
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
		soak_temp += increment;
		break;
	case SOAK_TIME:
		soak_time += increment;
		break;
	case REFLOW_TEMP:
		reflow_temp += increment;
		break;
	case REFLOW_TIME:
		reflow_time += increment;
		break;
	}
}

void updateDisplay() {

	// Print system parameters first
	display.setTextColor(BLACK);
	display.setCursor(0, 0);
	display.setTextSize(1);
	display.print(" Soak Te: ");
	display.println(soak_temp);
	display.print(" Soak Ti: ");
	display.println(soak_time);
	display.print(" RF Te: ");
	display.println(reflow_temp);
	display.print(" RF Ti: ");
	display.println(reflow_time);
	
	// Then update the cursor position
	display.setCursor(0, cursor_position);
	display.print("*");
	display.display();
}




void testdrawbitmap(const uint8_t *bitmap, uint8_t w, uint8_t h) {
  uint8_t icons[NUMFLAKES][3];
  randomSeed(500);     // whatever seed
 
  // initialize
  for (uint8_t f=0; f< NUMFLAKES; f++) {
    icons[f][XPOS] = random(display.width());
    icons[f][YPOS] = 0;
    icons[f][DELTAY] = random(5) + 1;
    
    Serial.print("x: ");
    Serial.print(icons[f][XPOS], DEC);
    Serial.print(" y: ");
    Serial.print(icons[f][YPOS], DEC);
    Serial.print(" dy: ");
    Serial.println(icons[f][DELTAY], DEC);
  }

  while (1) {
    // draw each icon
    for (uint8_t f=0; f< NUMFLAKES; f++) {
      display.drawBitmap(icons[f][XPOS], icons[f][YPOS], logo16_glcd_bmp, w, h, BLACK);
    }
    display.display();
    delay(200);
    
    // then erase it + move it
    for (uint8_t f=0; f< NUMFLAKES; f++) {
      display.drawBitmap(icons[f][XPOS], icons[f][YPOS],  logo16_glcd_bmp, w, h, WHITE);
      // move it
      icons[f][YPOS] += icons[f][DELTAY];
      // if its gone, reinit
      if (icons[f][YPOS] > display.height()) {
	icons[f][XPOS] = random(display.width());
	icons[f][YPOS] = 0;
	icons[f][DELTAY] = random(5) + 1;
      }
    }
   }
}


void testdrawchar(void) {
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0,0);

  for (uint8_t i=0; i < 168; i++) {
    if (i == '\n') continue;
    display.write(i);
    //if ((i > 0) && (i % 14 == 0))
      //display.println();
  }    
  display.display();
}

void testdrawcircle(void) {
  for (int16_t i=0; i<display.height(); i+=2) {
    display.drawCircle(display.width()/2, display.height()/2, i, BLACK);
    display.display();
  }
}

void testfillrect(void) {
  uint8_t color = 1;
  for (int16_t i=0; i<display.height()/2; i+=3) {
    // alternate colors
    display.fillRect(i, i, display.width()-i*2, display.height()-i*2, color%2);
    display.display();
    color++;
  }
}

void testdrawtriangle(void) {
  for (int16_t i=0; i<min(display.width(),display.height())/2; i+=5) {
    display.drawTriangle(display.width()/2, display.height()/2-i,
                     display.width()/2-i, display.height()/2+i,
                     display.width()/2+i, display.height()/2+i, BLACK);
    display.display();
  }
}

void testfilltriangle(void) {
  uint8_t color = BLACK;
  for (int16_t i=min(display.width(),display.height())/2; i>0; i-=5) {
    display.fillTriangle(display.width()/2, display.height()/2-i,
                     display.width()/2-i, display.height()/2+i,
                     display.width()/2+i, display.height()/2+i, color);
    if (color == WHITE) color = BLACK;
    else color = WHITE;
    display.display();
  }
}

void testdrawroundrect(void) {
  for (int16_t i=0; i<display.height()/2-2; i+=2) {
    display.drawRoundRect(i, i, display.width()-2*i, display.height()-2*i, display.height()/4, BLACK);
    display.display();
  }
}

void testfillroundrect(void) {
  uint8_t color = BLACK;
  for (int16_t i=0; i<display.height()/2-2; i+=2) {
    display.fillRoundRect(i, i, display.width()-2*i, display.height()-2*i, display.height()/4, color);
    if (color == WHITE) color = BLACK;
    else color = WHITE;
    display.display();
  }
}
   
void testdrawrect(void) {
  for (int16_t i=0; i<display.height()/2; i+=2) {
    display.drawRect(i, i, display.width()-2*i, display.height()-2*i, BLACK);
    display.display();
  }
}

void testdrawline() {  
  for (int16_t i=0; i<display.width(); i+=4) {
    display.drawLine(0, 0, i, display.height()-1, BLACK);
    display.display();
  }
  for (int16_t i=0; i<display.height(); i+=4) {
    display.drawLine(0, 0, display.width()-1, i, BLACK);
    display.display();
  }
  delay(250);
  
  display.clearDisplay();
  for (int16_t i=0; i<display.width(); i+=4) {
    display.drawLine(0, display.height()-1, i, 0, BLACK);
    display.display();
  }
  for (int8_t i=display.height()-1; i>=0; i-=4) {
    display.drawLine(0, display.height()-1, display.width()-1, i, BLACK);
    display.display();
  }
  delay(250);
  
  display.clearDisplay();
  for (int16_t i=display.width()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, i, 0, BLACK);
    display.display();
  }
  for (int16_t i=display.height()-1; i>=0; i-=4) {
    display.drawLine(display.width()-1, display.height()-1, 0, i, BLACK);
    display.display();
  }
  delay(250);

  display.clearDisplay();
  for (int16_t i=0; i<display.height(); i+=4) {
    display.drawLine(display.width()-1, 0, 0, i, BLACK);
    display.display();
  }
  for (int16_t i=0; i<display.width(); i+=4) {
    display.drawLine(display.width()-1, 0, i, display.height()-1, BLACK); 
    display.display();
  }
  delay(250);
}
