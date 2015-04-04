#include "Arduino.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include "menu.h"

/************************************************

	Constructors and Initialization Function

*************************************************/


// Constuctor with optional contrast setting
Menu::Menu() {

	// Initialize most of the class variables
	init();
	display.setContrast(35);
}

Menu::Menu(int p_contrast = 35) {

	// Initialize most of the class variables
	init();
	display.setContrast(p_contrast);
}

void Menu::init(){

	// Initialize vars
	m_cursor_pos = 0;
	m_element_selected = false;
	m_contents = NULL;
	m_header = "NULL";
	m_vis_count = m_MAX_LINES;

	// Populate visible elements counting by 1 from up to total array size
	for (byte i = 0; i < m_MAX_LINES; i++) {
		m_vis_elements[i] = i;
	}
	
	
}


/************************************************

			 Menu Setup Function

*************************************************/

void Menu::begin() {
	// Initialize display 	
	display.begin();	
}

// Set the pointer to the array containing the menu elements
void Menu::setContents(element(*p_contents), int p_menu_size) {
	m_contents = p_contents;
	m_menu_size = p_menu_size;
}


/************************************************

			   Cursor Functions

*************************************************/

// Move cursor up one element or increase element value
void Menu::up(){
	
	if (!m_element_selected){
		m_cursor_pos--;

		// If the new position is below zero, loop the menu back to the end
		if (m_cursor_pos < 0) {
			Serial.println("Looping to bottom");
			m_cursor_pos = m_menu_size - 1;
			// Re-set the visible elements to the end of the list
			for (byte i = 0; i < m_vis_count; i++) {
				m_vis_elements[i] = m_menu_size - m_vis_count + i;
			}
		}

		// If this would move us off the screen, but not past the last menu element, scroll the menu contents
		else if (m_cursor_pos < m_vis_elements[0] && m_cursor_pos >= 0){
			Serial.println("Scrolling up");
			for (byte i = 0; i < m_vis_count; i++) {
				m_vis_elements[i]--;
			}
		}
	}

	// If the element is selected, increase its value by the appropriate increment, but not above the maximum
	else if (m_element_selected && (*m_contents[m_cursor_pos].value + m_contents[m_cursor_pos].increment) <= m_contents[m_cursor_pos].max)
		*m_contents[m_cursor_pos].value += m_contents[m_cursor_pos].increment;

	refresh();
}

// Move cursor down one element or decrease element value
void Menu::down(){

	// Move the cursor if the element is not selected
	if (!m_element_selected) {

		m_cursor_pos++;

		// If the new position exceeds the menu length, loop the menu back to the start
		if (m_cursor_pos > m_menu_size - 1) {
			Serial.println("Looping to top");
			m_cursor_pos = 0;
			// Re-set the visible elements back to the beginning of the list
			for (byte i = 0; i < m_vis_count; i++) {
				m_vis_elements[i] = i;
			}
		}

		// If this would move us off the screen, but not past the last menu element, scroll the menu contents
		else if (m_cursor_pos > m_vis_elements[m_vis_count - 1] && m_cursor_pos <= m_menu_size - 1) {
			Serial.println("Scrolling down");
			for (byte i = 0; i < m_vis_count; i++) {
				m_vis_elements[i]++;
			}
		}
	}

	// If the element is selected, decrease its value by the appropriate increment, but not below the minimum
	else if (m_element_selected && (*m_contents[m_cursor_pos].value - m_contents[m_cursor_pos].increment) >= m_contents[m_cursor_pos].min)
		*m_contents[m_cursor_pos].value -= m_contents[m_cursor_pos].increment;

	refresh();
}

// Toggle the current element selection state
void Menu::select(){
	m_element_selected = !m_element_selected;
	refresh();
}


/************************************************

			  Display Functions

*************************************************/

// Clear the display
void Menu::clear() {
	display.clearDisplay();
	display.display();
}

// Set the header text. This is the line that will appear at 
// the top of the screen and will not move when elements scroll
void Menu::setHeader(String p_header){
	
	m_header = p_header;

	// If a header is used, offset the menu to make room
	if (m_header != "NULL")
		m_vis_count = m_MAX_LINES - m_HEADER_OFFSET;
	else
		m_vis_count = m_MAX_LINES;
}

// Print one element on a particular line
void Menu::displayElement(int p_item, int p_line) {

	// Move to the appropriate display line
	display.setCursor(0, p_line * m_CHAR_HEIGHT);

	// Print the selector icon if needed
	if (p_item == m_cursor_pos && !m_element_selected)
		display.print("*");

	// Move one character to the left
	display.setCursor(1 * m_CHAR_WIDTH, p_line * m_CHAR_HEIGHT);

	// Print the element name
	display.print(m_contents[p_item].name);

	// Print the appropriate separator, determined by whether the element is selected
	if (p_item == m_cursor_pos && m_element_selected)
		display.print("= ");
	else
		display.print(": ");

	// Print the element value and unit
	display.print(*m_contents[p_item].value);
	display.print(m_contents[p_item].unit);
}

// Redraw the screen with updated info
void Menu::refresh() {

	int header_offset = 0;

	display.clearDisplay();
	display.setTextSize(1);
	
	if (m_header != "NULL") {
		display.setCursor(0, 0);
		display.print(m_header);
		header_offset = 2;
	}

	for (byte i = 0; i < m_vis_count; i++) {
		Serial.print("Line: ");
		Serial.print(i + m_HEADER_OFFSET);
		Serial.print(" Menu item: ");
		Serial.println(m_vis_elements[i]);
		displayElement(m_vis_elements[i], i + m_HEADER_OFFSET);
	}

	display.display();

}
