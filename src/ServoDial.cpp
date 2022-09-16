// Servo Dial
// This code is released into the public domain.  Attribution is appreciated.
//
// This code controls dials via PCA9685 PWM chip
// It assumes that
// #define MAX_POS 180 // equivalent to 360 degree rotation
// #define NUM_ITEMS 10 // Number of things to show. Not tested with number of items that are not clean divisor of MAX_POS
// are defined in ServoDial.h
// The code converts the desired position to corresponding PWM signal setting


#include "ServoDial.h"

/// @brief Initializes the dial with the wire connection and also the position it is supposed to be.
/// @param servoConnection - wire on PCA9685 PWM chip
/// @param pwm - Adafruit PWM Servo Driver
/// @param prevPos - What the position was when the program started
void ServoDial::init(int servoConnection, Adafruit_PWMServoDriver *pwm, int prevPos)
{
    _pwm = pwm;
    _currPos = prevPos;
    _servoConnection = servoConnection;
    Serial.println("Dial init:");
    this->print();
}


/// @brief Converts desired Pos to the corresponding PWM value
/// @param desPos
/// @return pwm_value
int ServoDial::des_pos_to_val(int desPos)
{
    assert(desPos>=MIN_POS && desPos<MAX_POS);
    int pwm_value = map(desPos, MIN_POS, MAX_POS, SERVOMIN, SERVOMAX);
    Serial.print("New Setting for desPos: ");
    Serial.print(desPos);
    Serial.printf("\tpwm_value: %d", pwm_value);
    return pwm_value;
}

void ServoDial::setPos(int desPos)
{
    int pwm_value = this->des_pos_to_val(desPos);
    _currPos = desPos;
    cli(); // Interrupt disabled for indivisible processing
    _pwm->setPWM(_servoConnection, 0, pwm_value);
    sei();   // Interrupt enabled because the setting is completed
    Serial.println("Setting Complete");
    this->print();
}

int ServoDial::getPos()
{
    return _currPos;
}

void ServoDial::print()
{
    Serial.print("Dial: currPos: ");
    Serial.print(_currPos);
    Serial.print("\t_servoConnection: ");
    Serial.print(_servoConnection);
    Serial.print("\n");
}
