/**
 * @author  LSITA
 * @brief   HD44780 LCD driver via PCF8574 I/O expander
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* my PCF8574 (not -A) address is: 0b0100111. Write: 0x4E,	Read: 0x4F,	7bit without RW: 0x27 */
#define PCF8574_ADDR_WRITE	(0x4E)
#define PCF8574_ADDR_READ	(0x4F)

/**
 * @defgroup HD44780_Functions
 * @brief    Library Functions
 * @{
 */


/**
 * @brief  Initializes HD44780 LCD
 * @brief  cols: Width of lcd
 * @param  rows: Height of lcd
 * @retval None
 */
void HD44780_Init(uint8_t cols, uint8_t rows);

/**
 * @brief  Turn display on
 * @param  None
 * @retval None
 */
void HD44780_DisplayOn(void);

/**
 * @brief  Turn display off
 * @param  None
 * @retval None
 */
void HD44780_DisplayOff(void);

/**
 * @brief  Clears entire LCD
 * @param  None
 * @retval None
 */
void HD44780_Clear(void);

/**
 * @brief  Puts string on lcd
 * @param  x: X location where string will start
 * @param  y; Y location where string will start
 * @param  *str: pointer to string to display
 * @retval None
 */
void HD44780_Puts(uint8_t x, uint8_t y, char* str);

/**
 * @brief  Enables cursor blink
 * @param  None
 * @retval None
 */
void HD44780_BlinkOn(void);

/**
 * @brief  Disables cursor blink
 * @param  None
 * @retval None
 */
void HD44780_BlinkOff(void);

/**
 * @brief  Shows cursor
 * @param  None
 * @retval None
 */
void HD44780_CursorOn(void);

/**
 * @brief  Hides cursor
 * @param  None
 * @retval None
 */
void HD44780_CursorOff(void);

/**
 * @brief  Scrolls display to the left
 * @param  None
 * @retval None
 */
void HD44780_ScrollLeft(void);

/**
 * @brief  Scrolls display to the right
 * @param  None
 * @retval None
 */
void HD44780_ScrollRight(void);

/**
 * @brief  Creates custom character
 * @param  location: Location where to save character on LCD. LCD supports up to 8 custom characters, so locations are 0 - 7
 * @param *data: Pointer to 8-bytes of data for one character
 * @retval None
 */
void HD44780_CreateChar(uint8_t location, uint8_t* data);

/**
 * @brief  Puts custom created character on LCD
 * @param  x: X location where character will be shown
 * @param  y: Y location where character will be shown
 * @param  location: Location on LCD where character is stored, 0 - 7
 * @retval None
 */
void HD44780_PutCustom(uint8_t x, uint8_t y, uint8_t location);

void HD44780_demo(void);


#ifdef __cplusplus
}
#endif

