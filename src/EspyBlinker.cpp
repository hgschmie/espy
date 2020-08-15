/*
 * Manage all things display.
 */

#include <espy.h>

EspyBlinker::EspyBlinker(int initial_in_ticks)
  : initial(initial_in_ticks)
  , counter(initial_in_ticks)
  , state(false)
{}

void EspyBlinker::blink() {
    if (--counter <= 0) {
        state = !state;
        counter = initial;
    }
}
