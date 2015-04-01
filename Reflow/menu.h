// menu.h

/*********************************************************************
						Nokia 5110 Menu System

This library supports a menu system for an arbitrary number of items.
When selected, the user may adjust the value of the current item within
a pre-set range of values. The increment size for each menu item may
be set to avoid excessive joystick moves to change large values.

This library uses the Adafruit PCD8544 and GFX libraries to handle
screen drawing tasts.

*********************************************************************/

// Character size on screen: 8px H x 6px W

#ifndef _MENU_h
#define _MENU_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

class Menu {
	
	public:
		Menu();								// Default constructor
		Menu(int);							// Constructor with optional constrast setting

		struct element {					// Structure of menu elements
			String name;
			int *value;
			String unit;
			int min;
			int max;			
			int increment;
		};

		void setContents(element(*), int);	// Accepts a pointer to an array of elements
		void setHeader(String);				// Sets the text to display at the top of the screen
		
		void up();						// Move cursor up one menu element
		void down();					// Move cursor down one menu element
		void select();					// Select or deselect the current menu element

		void clear();						// Completely clear the screen
		void refresh();						// Refresh the screen with updated menu info

		// Initialize 
		// Software SPI (slower updates, more flexible pin options):
		// pin 7 - Serial clock out (SCLK)
		// pin 6 - Serial data out (DIN)
		// pin 5 - Data/Command select (D/C)
		// pin 4 - LCD chip select (CS)
		// pin 3 - LCD reset (RST)
		Adafruit_PCD8544 display = Adafruit_PCD8544(7, 6, 5, 4, 3);;
		
		int m_cursor_pos;															// Current element targeted
		bool m_element_selected;													// Whether current element is selected for modification

	private:
		void init();
		static const int m_SCREEN_HEIGHT = 48;
		static const int m_SCREEN_WIDTH = 84;
		static const int m_CHAR_HEIGHT = 8;
		static const int m_CHAR_WIDTH = 6;

		String m_header;															// Text to display at top of the window. Ignored if m_header == "NULL"
	
		element(*m_contents);														// Pointer to the array of menu elements
		int	m_menu_size;															// Number of items in the array of menu elements
		static const int m_VIS_COUNT = m_SCREEN_HEIGHT / m_CHAR_HEIGHT;				// Number of elements that can be simultaneously viewed on the screen
		int m_vis_elements[m_VIS_COUNT];											// Array of element reference numbers that are currently visible

		void displayElement(int, int);												// Prints one element from the menu list at a specified location

		
};

#endif

