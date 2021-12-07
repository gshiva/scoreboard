// Clock Tick Demonstration
//
// By Matt Mets, completed in 2008
//
// This code is released into the public domain.  Attribution is appreciated.
//
// This is a demonstration on how to control a cheapo clock mechanism with an Arduino.
// The clock mechanism works by using an electromagnet to pull a little fixed magnet,
// similar to how a DC motor works.  To control this with the Arduino, we need to hook a
// wire up to each side of the electromagnet (disconnect the exisiting clock circuity if
// possible).  Then, hook each of the wires to pins on the Arduino.  I chose pins 2 and 3
// for my circuit.  It is also a good idea to put a resistor (I chose 500 ohms) in series
// (between one of the wires and an Arduino pin), which will limit the amount of current
// that is applied.  Once the wires are hooked up, you take turns turning on one or the
// other pin momentarily.  Each time you do this, the clock 'ticks' and moves forward one
// second.  I have provided a doTick() routine to do this automatically, so it just needs
// to be called each time you want the clock to tick.
//

#include "Dial.h"

void Dial::init(int clockA, int clockB, Adafruit_PWMServoDriver *pwm, int prevPos)
{
    _clockA = clockA;
    _clockB = clockB;
    _pwm = pwm;
    _prevPos = prevPos;
    _currPos = des_pos_to_val(prevPos);
    _sv = _currPos;
    _tickPin = clockA;
    if (clockA == 3)
    {
        pinMode(_clockA, OUTPUT);
        pinMode(_clockB, OUTPUT);
    }
    Serial.println("Dial init:");
    this->print();
}

int Dial::des_pos_to_val(int desPos)
{
    int scaled_Val = desPos * MIN_POS_INCR;
    int prev_scaled_Val = _prevPos * MIN_POS_INCR;
    int d;

    if (scaled_Val >= prev_scaled_Val)
    {
        d = (scaled_Val - prev_scaled_Val);
    }
    else
    {
        d = ((MAX_POS) - (prev_scaled_Val - scaled_Val));
    }
    Serial.print("New Setting for desPos: ");
    Serial.print(desPos);
    Serial.printf("\tprevPos: %ld", _prevPos);
    Serial.printf("\tscaled_Val: %d", scaled_Val);
    Serial.printf("\tprev_scaled_Val: %d", prev_scaled_Val);
    Serial.printf("\tdiff = %d\n", d);
    return d;
}

void Dial::setPos(int desPos)
{
    int d = this->des_pos_to_val(desPos);
    cli(); // Interrupt disabled for indivisible processing
    _prevPos = desPos;
    _currPos = 0;
    _sv = d; // Set to the target position of the pulse motor
    sei();   // Interrupt enabled because the setting is completed
    Serial.println("Setting Complete");
    this->print();
}

int Dial::getPos()
{
    return _prevPos;
}

void Dial::print()
{
    Serial.print("Dial: currPos: ");
    Serial.print(_currPos);
    Serial.print("\t_sv: ");
    Serial.print(_sv);
    Serial.print("\t_prevPos: ");
    Serial.print(_prevPos);
    Serial.print("\tclockA: ");
    Serial.print(_clockA);
    Serial.print("\tclockB: ");
    Serial.print(_clockB);
    Serial.print("\t_tickPin: ");
    Serial.print(_tickPin);
    Serial.print("\n");
}

bool Dial::moveOneStep()
{
    if (_currPos < _sv)
    {
        // this->print();
        delay(40);
        this->doTick();
        _currPos += 10;
    }
    return _currPos < _sv;
}

inline void Dial::doTick()
{
    // Energize the electromagnet in the correct direction.
    pwm_digitalWrite(_tickPin, HIGH);
    delay(10);
    pwm_digitalWrite(_tickPin, LOW);

    // Switch the direction so it will fire in the opposite way next time.
    if (_tickPin == _clockA)
    {
        _tickPin = _clockB;
    }
    else
    {
        _tickPin = _clockA;
    }
}

void Dial::pwm_digitalWrite(int pin, int val)
{
    if (val == LOW)
    {
        _pwm->setPWM(pin, 0, 4096);
    }
    else
    {
        _pwm->setPWM(pin, 4096, 0);
    }
}