/*
Class to control Dial
*/

#ifndef _SERVO_DIAL_H
#define _SERVO_DIAL_H

#include <M5StickC.h>
#include <Adafruit_PWMServoDriver.h>

#define MIN_POS 0
#define MAX_POS 10
#define SERVOMIN  210  // Minimum value
#define SERVOMAX  600  // Maximum value

/// @brief Controls dial position using Servos
class ServoDial
{
public:
    ServoDial() {}
    void init(int servoConnection, Adafruit_PWMServoDriver *pwm=nullptr, int prevPos = 0);
    void setPos(int desPos);
    int getPos();
    void print();

private:
    int des_pos_to_val(int desPos);
    void set_des_pos(int d);
    Adafruit_PWMServoDriver *_pwm;
    int _servoConnection;
    int _currPos;
};

#endif
