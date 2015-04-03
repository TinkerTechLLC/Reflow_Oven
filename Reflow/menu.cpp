#include "Arduino.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include "menu.h"

/************************************************

	Constructors and Initialization Function

*************************************************/


// Constuctor with optional contrast setting
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

	// Populate visible elements counting by 1 from up to total array size
	for (byte i = 0; i < m_VIS_COUNT; i++) {
		m_vis_elements[i] = i;
	}
	
	// Initialize display 	
	display.begin();	
}


/************************************************

			 Menu Setup Function

*************************************************/

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
			m_cursor_pos = m_menu_size;
			// Re-set the visible elements to the end of the list
			for (byte i = m_menu_size - m_VIS_COUNT; i < m_VIS_COUNT; i++) {
				m_vis_elements[i] = i;
			}
		}

		// If this would move us off the screen, but not past the last menu element, scroll the menu contents
		else if (m_cursor_pos < m_vis_elements[0] && m_cursor_pos >= 0){
			for (byte i = 0; i < m_VIS_COUNT; i++) {
				m_vis_elements[i]--;
			}
		}
	}

	// If the element is selected, increase its value by the appropriate increment
	else if (m_element_selected)
		*m_contents[m_cursor_pos].value += m_contents[m_cursor_pos].increment;

	refresh();
}

// Move cursor down one element or decrease element value
void Menu::down(){

	// Move the cursor if the element is not selected
	if (!m_element_selected) {

		m_cursor_pos++;

		// If the new position exceeds the menu length, loop the menu back to the start
		if (m_cursor_pos > m_menu_size) {
			m_cursor_pos = 0;
			// Re-set the visible elements back to the beginning of the list
			for (byte i = 0; i < m_VIS_COUNT; i++) {
				m_vis_elements[i] = i;
			}
		}

		// If this would move us off the screen, but not past the last menu element, scroll the menu contents
		else if (m_cursor_pos > m_vis_elements[m_VIS_COUNT] && m_cursor_pos <= m_menu_size){
			for (byte i = 0; i < m_VIS_COUNT; i++) {
				m_vis_elements[i]++;
			}
		}
	}

	// If the element is selected, decrease its value by the appropriate increment
	else if (m_element_selected)
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
}

// Set the header text. This is the line that will appear at 
// the top of the screen and will not move when elements scroll
void Menu::setHeader(String p_header){
	m_header = p_header;
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

	display.clearDisplay();
	
	for (byte i = 0; i < m_VIS_COUNT; i++) {
		displayElement(m_vis_elements[0], i);
	}

}
