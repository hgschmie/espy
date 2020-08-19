/* -*- mode: C++; -*-
 *
 * A simple blink structure.
 */

#ifndef _ESPY_ESPYBLINKER_H_
#define _ESPY_ESPYBLINKER_H_

#include <espy.h>

class EspyBlinker {
 public:
  explicit EspyBlinker(int initial_in_ticks);

  void blink();

  bool state;


 private:
    int initial;
    int counter;
};


#endif
