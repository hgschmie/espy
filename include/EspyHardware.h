/*
 * espy hardware detection and management.
 */

#ifndef ESPY_ESPYHARDWARE_H
#define ESPY_ESPYHARDWARE_H

#include <Arduino.h>
#include <PCF8574.h>

#define HW_NO_ERROR 0
#define HW_NO_DISPLAY_FOUND -1
#define HW_NO_PCF_FOUND -2

#define BUTTON_IO_0 0x20u
#define BUTTON_IO_1 0x40u
#define BUTTON_IO_2 0x80u

#define BUTTON_SHIFT ((uint8_t)5)
#define BUTTON_IO_MASK (BUTTON_IO_0 | BUTTON_IO_1 | BUTTON_IO_2)

// inline macro to read state of the buttons
#define READ_BUTTONS(PCF_CHIP) ((~(PCF_CHIP->readButton8(_BUTTON_IO_MASK)) & _BUTTON_IO_MASK) >> _BUTTON_SHIFT)

#define BUTTON_0 1u
#define BUTTON_1 2u
#define BUTTON_2 4u

#define LED_IO_0 0x01u
#define LED_IO_1 0x02u
#define LED_IO_2 0x04u
#define LED_IO_3 0x08u
#define LED_IO_4 0x10u

#define LED_IO_MASK (LED_IO_0 | LED_IO_1 | LED_IO_2 | LED_IO_3 | LED_IO_4 )

#define LED_0 0x00u
#define LED_1 0x01u
#define LED_2 0x02u
#define LED_3 0x03u
#define LED_4 0x04u

/*
 * Contains all the hardware information.
 */
class EspyHardware {
public:
    EspyHardware();

    uint8_t display_address = 0xffu;
    uint8_t pcf_address = 0xffu;
    uint8_t error = HW_NO_ERROR;

    PCF8574 *pcf;

    void leds(uint8_t led_value) const;


private:
    void _init_i2c_bus();

    void init_display();

    void init_pcf();

};


#endif //ESPY_ESPYHARDWARE_H
