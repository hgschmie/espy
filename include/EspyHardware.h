/*
 * espy hardware detection and management.
 */

#include <Wire.h>
#include <PCF8574.h>
#include <LiquidCrystal_I2C.h>

#include <espy.h>

#ifndef _ESPY_ESPYHARDWARE_H_
#define _ESPY_ESPYHARDWARE_H_

// espy hardware

// GPIO Pins for I2C Bus
#define I2C_SDA 0
#define I2C_SCL 2

// display size
#define DISPLAY_ROWS 2
#define DISPLAY_COLS 16

// error codes
#define HW_NO_ERROR 0
#define HW_NO_DISPLAY_FOUND -1
#define HW_NO_PCF_FOUND -2

#define BUTTON_IO_0 0x20u
#define BUTTON_IO_1 0x40u
#define BUTTON_IO_2 0x80u

#define BUTTON_SHIFT 5u
#define BUTTON_IO_MASK (BUTTON_IO_0 | BUTTON_IO_1 | BUTTON_IO_2)

#define LED_IO_0 0x01u
#define LED_IO_1 0x02u
#define LED_IO_2 0x04u
#define LED_IO_3 0x08u
#define LED_IO_4 0x10u

#define LED_IO_MASK (LED_IO_0 | LED_IO_1 | LED_IO_2 | LED_IO_3 | LED_IO_4 )

/*
 * Contains all the hardware information.
 */
class EspyHardware {
public:
    EspyHardware();

    uint8_t display_address = 0xffu;
    uint8_t pcf_address = 0xffu;
    int error = HW_NO_ERROR;

    PCF8574 *pcf;
    LiquidCrystal_I2C *display;

    void leds(uint8_t led_value) const;

    void text(const char *text[DISPLAY_ROWS]) const;

    uint8_t keys() const;

private:
    void _init_i2c_bus();

    void init_display();

    void init_pcf();

};


#endif

