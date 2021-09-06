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

////// Board Setup /////////////////////////////////////////////////////////////////////////
#include <M5StickC.h>
#include <Adafruit_PWMServoDriver.h>

#include <WiFiClientSecure.h>

#define _TASK_SLEEP_ON_IDLE_RUN // Enable 1 ms SLEEP_IDLE powerdowns between tasks if no callback methods were invoked during the pass
#define _TASK_STATUS_REQUEST    // Compile with support for StatusRequest functionality - triggering tasks on status change events in addition to time only
#include <TaskScheduler.h>

// Scheduler
Scheduler ts;
#define DURATION 10000

// Debug and Test options
#define _DEBUG_
//#define _TEST_

#ifdef _DEBUG_
#define _PP(a) Serial.print(a);
#define _PL(a) Serial.println(a);
#else
#define _PP(a)
#define _PL(a)
#endif


#define PERIOD1 500
#define DURATION 10000
void blink1CB();
Task tBlink1 ( PERIOD1 * TASK_MILLISECOND, DURATION / PERIOD1, &blink1CB, &ts, true );

inline void LEDOn() {
  digitalWrite( LED_BUILTIN, HIGH );
}

inline void LEDOff() {
  digitalWrite( LED_BUILTIN, LOW );
}

// === 1 =======================================
bool LED_state = false;
void blink1CB() {
  if ( tBlink1.isFirstIteration() ) {
    _PP(millis());
    _PL(": Blink1 - simple flag driven");
    LED_state = false;
  }

  if ( LED_state ) {
    LEDOff();
    LED_state = false;
  }
  else {
    LEDOn();
    LED_state = true;
  }

  if ( tBlink1.isLastIteration() ) {
    tBlink1.restartDelayed( 2 * TASK_SECOND );
    LEDOff();
  }
}


const char *cricclubs_server = "cricclubs.com";
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

extern unsigned long timer0_overflow_count;
#define MAX_POS 9620 // equivalent to 360 degree rotation
#define NUM_ITEMS 10 // Number of things to show. Not tested with number of items that are not clean divisor of MAX_POS
#define MIN_POS_INCR MAX_POS / NUM_ITEMS

int clockA = 3; // Set these to the pin numbers you have attached the clock wires
int clockB = 4; // to.  Order is not important.

int tickPin = clockA; // This keeps track of which clock pin should be fired next

unsigned long currPos = 0;
unsigned long prevPos = 0;
unsigned long desPos = 0;
volatile int sv = currPos; // set value (set variable)
volatile int d = 0;

void pwm_digitalWrite(int pin, int value)
{
  if (value == LOW)
  {
    pwm.setPWM(pin, 0, 4096);
  }
  else
  {
    pwm.setPWM(pin, 4096, 0);
  }
}

#include <IotWebConf.h>
#include <IotWebConfUsing.h> // This loads aliases for easier class names.

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "testThing";

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "smrtTHNG8266";

#define STRING_LEN 128
#define NUMBER_LEN 32

#define D2 39
#define LED_BUILTIN 10

// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "sb1"

// -- When CONFIG_PIN is pulled to ground on startup, the Thing will use the initial
//      password to buld an AP. (E.g. in case of lost password)
#define CONFIG_PIN D2

// -- Status indicator pin.
//      First it will light up (kept LOW), on Wifi connection it will blink,
//      when connected to the Wifi it will turn off (kept HIGH).
#define STATUS_PIN LED_BUILTIN

// -- Method declarations.
void handleRoot();
// -- Callback methods.
void configSaved();
bool formValidator(iotwebconf::WebRequestWrapper *webRequestWrapper);

DNSServer dnsServer;
WebServer server(80);

char internalClockPosValue[NUMBER_LEN];
char tournamentIdValue[NUMBER_LEN];
char clubIdValue[NUMBER_LEN];
char matchIdValue[NUMBER_LEN];

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
// -- You can also use namespace formats e.g.: iotwebconf::TextParameter
IotWebConfParameterGroup sbSettings = iotwebconf::ParameterGroup("sbSettings", "Scoreboard Settings");
IotWebConfNumberParameter internalClockPos = IotWebConfNumberParameter("Internal Clock Position", "internalClockPos", internalClockPosValue, NUMBER_LEN, "7", "1..100", "min='0' max='100' step='1'");
// -- We can add a legend to the separator
IotWebConfNumberParameter clubId = IotWebConfNumberParameter("Club ID", "clubId", clubIdValue, NUMBER_LEN, "0", "1..1000000", "min='0' max='1000000' step='1'");
IotWebConfNumberParameter matchId = IotWebConfNumberParameter("Match ID", "matchId", matchIdValue, NUMBER_LEN, "0", "1..1000000", "min='0' max='1000000' step='1'");
IotWebConfTextParameter tournamentId = iotwebconf::TextParameter("Tournament ID", "tournamentId", tournamentIdValue, NUMBER_LEN, "NACL");

// Initialize the IO ports
void setup()
{
  M5.begin();
  M5.Lcd.println("hello world.");
  pinMode(clockA, OUTPUT);
  pinMode(clockB, OUTPUT);

  Serial.begin(115200);
  pwm.begin();
  pwm.setPWMFreq(1000); // Set to whatever you like, we don't use it in this demo!

  // if you want to really speed stuff up, you can go into 'fast 400khz I2C' mode
  // some i2c devices dont like this so much so if you're sharing the bus, watch
  // out for this!
  Wire.setClock(400000);
  Serial.println();
  Serial.println("Starting up...");

  sbSettings.addItem(&internalClockPos);
  sbSettings.addItem(&tournamentId);
  sbSettings.addItem(&clubId);
  sbSettings.addItem(&matchId);

  iotWebConf.setStatusPin(STATUS_PIN);
  iotWebConf.setConfigPin(CONFIG_PIN);
  iotWebConf.addParameterGroup(&sbSettings);
  iotWebConf.setConfigSavedCallback(&configSaved);
  iotWebConf.setFormValidator(&formValidator);
  iotWebConf.getApTimeoutParameter()->visible = true;

  // -- Initializing the configuration.
  iotWebConf.init();
  prevPos = atoi(internalClockPosValue);
  Serial.printf("Prev Position =  %ld \n", prevPos);

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", []
            { iotWebConf.handleConfig(); });
  server.onNotFound([]()
                    { iotWebConf.handleNotFound(); });

  Serial.println("Ready.");
}

// Move the second hand forward one position (one second on the clock face).
void doTick()
{

  // Energize the electromagnet in the correct direction.
  pwm_digitalWrite(tickPin, HIGH);
  delay(10);
  pwm_digitalWrite(tickPin, LOW);

  // Switch the direction so it will fire in the opposite way next time.
  if (tickPin == clockA)
  {
    tickPin = clockB;
  }
  else
  {
    tickPin = clockA;
  }
}

int des_pos_to_val(int desPos, int prevPos)
{
  int scaled_Val = desPos * MIN_POS_INCR;
  int prev_scaled_Val = prevPos * MIN_POS_INCR;

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

void set_des_pos(int d)
{
  cli(); // Interrupt disabled for indivisible processing
  prevPos = desPos;
  currPos = 0;
  sv = d; // Set to the target position of the pulse motor
  sei();  // Interrupt enabled because the setting is completed
}

char rx_byte = 0;

// Main loop
void loop()
{
  // goto the end position and then sleep
  if (currPos < sv)
  {
    delay(40);
    doTick();
    currPos += 10;
  }
  else
  {
    if (Serial.available() > 0)
    {                          // is a character available?
      rx_byte = Serial.read(); // get the character

      // check if a number was received
      if ((rx_byte >= '0') && (rx_byte <= '9'))
      {
        Serial.print("Number received:");
        Serial.println(rx_byte);
        desPos = rx_byte - '0';
        d = des_pos_to_val(desPos, prevPos);
        set_des_pos(d);
      }
      else
      {
        Serial.println("Not a number.");
      }

    } // end: if (Serial.available ()> 0)

    // Serial.print(sv); // Flow the value serially for state observation (easy to understand by looking at the serial plotter)
    // Serial.print(",");
    // Serial.println(currPos);

    if (WiFi.status() == WL_CONNECTED)
    {
      M5.Lcd.println(WiFi.localIP());
      M5.Lcd.println(clubIdValue);
      M5.Lcd.println(matchIdValue);
      // Serial.println("Executing scheduled task.");
      ts.execute();

      if (strcmp("0", clubIdValue) != 0 && strcmp("0", matchIdValue) != 0 && 1 == 10)
      {
        WiFiClientSecure client;
        client.setInsecure();

        Serial.println("\nStarting connection to server...");
        M5.Lcd.println("Starting connection to server...");
        if (!client.connect(cricclubs_server, 443))
          Serial.println("Connection failed!");
        else
        {
          Serial.println("Connected to server!");
          // Make a HTTP request:
          String req("GET https://cricclubs.com/");
          req += tournamentIdValue;
          req += "/viewScorecard.do?matchId=";
          req += atoi(matchIdValue);
          req += "&clubId=";
          req += atoi(clubIdValue);
          req += " HTTP/1.0";
          client.println(req);
          client.println("Host: cricclubs.com");
          client.println("Connection: close");
          client.println();

          while (client.connected())
          {
            String line = client.readStringUntil('\n');
            if (line == "\r")
            {
              Serial.println("headers received");
              break;
            }
          }
          // if there are incoming bytes available
          // from the server, read them and print them:
          bool titleNotFound = true;
          while (client.available() && titleNotFound)
          {
            String line = client.readStringUntil('\n');
            // skip till title is found
            if (line.startsWith("<title"))
            {
              String message("Title found for ");
              String clubIDMessage("Club ID:");
              clubIDMessage += atoi(clubIdValue);
              message += clubIDMessage;
              message += " ";
              String matchIDMessage("Match ID:");
              matchIDMessage += atoi(matchIdValue);
              message += matchIDMessage;
              Serial.println(message);
              M5.Lcd.setCursor(0, 0);
              M5.Lcd.println(tournamentIdValue);
              M5.Lcd.println(clubIDMessage);
              M5.Lcd.println(matchIDMessage);
              titleNotFound = false;
            }
          }

          client.stop();
          delay(4000000);
        }
      }
    }
    else
    {
      Serial.println("Wifi not connected yet.");
    }
  }
  iotWebConf.doLoop();
}

/**
 * Handle web requests to "/" path.
 */
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>Scoreboard Settings</title></head><body>Scoreboard Settings";
  s += "<ul>";
  s += "<li>Club ID: ";
  s += atoi(clubIdValue);
  s += "<li>Match ID: ";
  s += atoi(matchIdValue);
  s += "<li>Internal Clock Position (internal use only): ";
  s += "Go to <a href='config'>configure page</a> to change values.";
  s += "</body></html>\n";
  prevPos = atoi(internalClockPosValue);
  Serial.printf("Prev Position =  %ld \n", prevPos);
  server.send(200, "text/html", s);
}

void configSaved()
{
  sscanf(internalClockPosValue, "%ld", &desPos);
  Serial.printf("New Desired Position =  %ld \n", desPos);
  d = des_pos_to_val(desPos, prevPos);
  set_des_pos(d);
  Serial.println("Configuration was updated.");
}

bool formValidator(iotwebconf::WebRequestWrapper *webRequestWrapper)
{
  Serial.println("Validating form.");
  bool valid = true;

  /*
  int l = webRequestWrapper->arg(stringParam.getId()).length();
  if (l < 3)
  {
    stringParam.errorMessage = "Please provide at least 3 characters for this test!";
    valid = false;
  }
*/
  return valid;
}