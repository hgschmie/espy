//
// Detect the hardware on the I2C bus
//

#include <espy.h>
#include <EspyHardware.h>

#include <Wire.h>

#define MAX_I2C_DEV_DETECTED 8


EspyHardware::EspyHardware()
        : display_address(0xff), pcf_address(0xff), error(HW_NO_ERROR), pcf(nullptr) {

    _init_i2c_bus();

    if (pcf_address == 0xff) {
        error = HW_NO_PCF_FOUND;
        Serial.println("NO PCF FOUND!");
    } else if (display_address == 0xff) {
        error = HW_NO_DISPLAY_FOUND;
        Serial.println("NO DISPLAY FOUND!");
    }
}

void EspyHardware::init_display() {

}

void EspyHardware::init_pcf() {
    pcf = new PCF8574(pcf_address);
    pcf->begin();
}

void EspyHardware::leds(uint8_t led_value) const {
    if (pcf != nullptr) {
        pcf->write8(BUTTON_IO_MASK | ~(led_value & LED_IO_MASK));
    }
}


void EspyHardware::_init_i2c_bus() {
    uint8_t i2c_address[MAX_I2C_DEV_DETECTED];
    int i2c_address_ptr = 0;

    // Start I2C Bus
    Wire.begin(I2C_SDA, I2C_SCL);

    // scan bus for devices
    for (uint8_t address = 1; address < 127; address++) {
        // The i2c_scanner uses the return value of
        // the Write.endTransmission to see if
        // a device did acknowledge to the address.
        Wire.beginTransmission(address);

        uint8_t wire_error = Wire.endTransmission();
        if (wire_error == 0) {
            i2c_address[i2c_address_ptr++] = address;
            if (i2c_address_ptr == MAX_I2C_DEV_DETECTED) {
                break; // for
            }
        } else {
            Wire.clearWriteError();
        }
    }

    // look at the devices found
    for (int i = 0; i < i2c_address_ptr; i++) {
        if ((pcf_address == 0xff) && ((i2c_address[i] & ((uint8_t) 0x7)) == 0)) {
            // all lower bits are 0. claim the first match as the
            // PCF chip
            pcf_address = i2c_address[i];
            init_pcf();
        } else if (display_address == 0xff) {
            display_address = i2c_address[i];
            init_display();
        }
    }
};
