/*
Class to control meter
*/

#ifndef _METER_H
#define _METER_H

#include <M5StickC.h>
#include <Adafruit_PWMServoDriver.h>

#define MAX_POS 9620 // equivalent to 360 degree rotation
#define NUM_ITEMS 10 // Number of things to show. Not tested with number of items that are not clean divisor of MAX_POS
#define MIN_POS_INCR MAX_POS / NUM_ITEMS

class Meter
{
public:
    Meter(int clockA, int clockB, int currPos, int prevPos = 0);
    void setPos(int desPos);
    inline bool moveOneStep();

private:
    void pwm_digitalWrite(int pin, int value);
    inline void doTick();
    int des_pos_to_val(int desPos);
    void set_des_pos(int d);
    int _clockA;  // wire 1 connected to the clock
    int _clockB;  // wire 2 connected to the clock (order doesn't matter)
    int _tickPin; // keeps track of which clock pin should be fired next
    unsigned long _prevPos;
    unsigned long _currPos;
    unsigned long _desPos;
    volatile int _sv; // set value
    volatile int _d;
};

#endif
