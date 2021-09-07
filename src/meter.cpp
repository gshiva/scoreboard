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

#include "meter.h"

Meter::Meter(int clockA, int clockB, int currPos, int prevPos)
    : _clockA(clockA), _clockB(clockB), _prevPos(prevPos), _currPos(currPos),
      _sv(currPos), _tickPin(clockA), _desPos(currPos)
{
}

void Meter::setPos(int desPos)
{
    int d = this->des_pos_to_val(desPos);
    cli(); // Interrupt disabled for indivisible processing
    _prevPos = _desPos;
    _currPos = 0;
    _sv = d; // Set to the target position of the pulse motor
    sei();   // Interrupt enabled because the setting is completed
}

inline bool Meter::moveOneStep()
{
    if (_currPos < _sv)
    {
        delay(40);
        this->doTick();
        _currPos += 10;
    }
    return _currPos < _sv;
}

inline void Meter::doTick()
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

int Meter::des_pos_to_val(int desPos)
{
    int scaled_Val = desPos * MIN_POS_INCR;
    int prev_scaled_Val = _prevPos * MIN_POS_INCR;
    int d;

    if (scaled_Val > prev_scaled_Val)
    {
        d = (scaled_Val - prev_scaled_Val);
    }
    else
    {
        d = ((MAX_POS) - (prev_scaled_Val - scaled_Val));
    }
    Serial.print("New Setting:");
    Serial.println(d);
    return d;
}