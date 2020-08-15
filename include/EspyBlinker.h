/* -*- mode: C++; -*-
 *
 * A simple blink structure.
 */
#ifndef ESPY_ESPYBLINKER_H
#define ESPY_ESPYBLINKER_H

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
