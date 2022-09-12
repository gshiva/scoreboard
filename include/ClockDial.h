/*
Class to control Dial
*/

#ifndef _DIAL_H
#define _DIAL_H

#include <M5StickC.h>
#include <Adafruit_PWMServoDriver.h>

#define MAX_POS 9620 // equivalent to 360 degree rotation
#define NUM_ITEMS 10 // Number of things to show. Not tested with number of items that are not clean divisor of MAX_POS
#define MIN_POS_INCR MAX_POS / NUM_ITEMS

class ClockDial
{
public:
    ClockDial() {}
    void init(int clockA=0, int clockB=0, Adafruit_PWMServoDriver *pwm=nullptr, int prevPos = 0);
    void setPos(int desPos);
    bool moveOneStep();
    int getPos();
    void print();

private:
    void pwm_digitalWrite(int pin, int value);
    inline void doTick();
    int des_pos_to_val(int desPos);
    void set_des_pos(int d);
    int _clockA; // wire 1 connected to the clock
    int _clockB; // wire 2 connected to the clock (order doesn't matter)
    Adafruit_PWMServoDriver *_pwm;
    unsigned long _prevPos;
    unsigned long _currPos;
    int _sv; // set value
    int _tickPin;     // keeps track of which clock pin should be fired next
    int _d;
};

#endif
