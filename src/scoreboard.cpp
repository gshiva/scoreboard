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
#undef min
#include <Adafruit_PWMServoDriver.h>
#include "Dial.h"

#include <WiFiClientSecure.h>

#include <IotWebConf.h>
#include <IotWebConfUsing.h> // This loads aliases for easier class names.

#define _TASK_SLEEP_ON_IDLE_RUN // Enable 1 ms SLEEP_IDLE powerdowns between tasks if no callback methods were invoked during the pass
#define _TASK_STATUS_REQUEST    // Compile with support for StatusRequest functionality - triggering tasks on status change events in addition to time only
#include <TaskScheduler.h>

#include <regex>
#include <string>

// Scheduler
Scheduler ts;
#define DURATION 10000

// Debug and Test options
#define _DEBUG_
//#define _TEST_

#include "MatchDetails.h"

#define PERIOD1 500
#define DURATION 10000
#define SCORE_PERIOD 60000        // 1 minute
#define CONFIG_SAVE_PERIOD 600000 // 10 minutes

void blink1CB();
void getScoreCB();
void saveConfigCB();

Task tBlink1(PERIOD1 *TASK_MILLISECOND, DURATION / PERIOD1, &blink1CB, &ts, true);
Task tGetScore(SCORE_PERIOD *TASK_MILLISECOND, TASK_FOREVER, &getScoreCB, &ts, true);
Task tSaveConfig(CONFIG_SAVE_PERIOD *TASK_MILLISECOND, TASK_FOREVER, &saveConfigCB, &ts, true);

unsigned long prevMillis = millis();
const char *cricclubs_server = "cricclubs.com";
#define STRING_LEN 128
#define NUMBER_LEN 32

#define D2 39
#define LED_BUILTIN 10

// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "sb2"

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

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

using namespace std;

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "testThing";

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "smrtTHNG8266";

const int NUM_DIALS = 8;
Dial dials[NUM_DIALS];
// Dials are assumed to be setup as follows:
//  1.  The first dial (dials[0]) is the least significant digit of Runs.
//  2.  The second dial (dials[1]) is the second least significant digit of Runs.
//  3.  The third dial (dials[2]) is the most significant digit of Runs.
//  4.  The fourth dial (dials[3]) is the least significant digit of Overs.
//  5.  The fifth dial (dials[4]) is the second least significant digit of Overs.
//  6.  The sixth dial (dials[5]) is the most significant digit of Overs.
//  7.  The seventh dial (dials[6]) is the least significant digit of Wickets.
//  8.  The eighth dial (dials[7]) is the most significant digit of Wickets.

int prevPos[NUM_DIALS];
char internalClockPosValue[NUM_DIALS][NUMBER_LEN];
String labels[NUM_DIALS];
String ids[NUM_DIALS];
IotWebConfNumberParameter *internalClockPos[NUM_DIALS];
char tournamentIdValue[NUMBER_LEN];
char clubIdValue[NUMBER_LEN];
char matchIdValue[NUMBER_LEN];
int prev_runs = 0;
int prev_overs = 0;
int prev_wickets = 0;
bool config_updated = false;

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
// -- You can also use namespace formats e.g.: iotwebconf::TextParameter
IotWebConfParameterGroup sbSettings = iotwebconf::ParameterGroup("sbSettings", "Scoreboard Settings");

// -- We can add a legend to the separator
IotWebConfNumberParameter clubId = IotWebConfNumberParameter("Club ID", "clubId", clubIdValue, NUMBER_LEN, "0", "1..1000000", "min='0' max='1000000' step='1'");
IotWebConfNumberParameter matchId = IotWebConfNumberParameter("Match ID", "matchId", matchIdValue, NUMBER_LEN, "0", "1..1000000", "min='0' max='1000000' step='1'");
IotWebConfTextParameter tournamentId = iotwebconf::TextParameter("Tournament ID", "tournamentId", tournamentIdValue, NUMBER_LEN, "NACL");

void setDials(MatchDetails &matchDetails, Dial dials[NUM_DIALS])
{
  if (matchDetails.isInitialized())
  {
    int *runDigits = matchDetails.getRunDigits();
    int *wicketDigits = matchDetails.getWicketDigits();
    int *overDigits = matchDetails.getOverDigits();
    int values[NUM_DIALS] = {runDigits[0], runDigits[1], runDigits[2],
                             overDigits[0], overDigits[1], overDigits[2],
                             wicketDigits[0], wicketDigits[1]};
    for (int i = 0; i < NUM_DIALS; i++)
    {
      dials[i].setPos(values[i]);
    }
  }
}

void getScoreCB()
{
  unsigned long currentMillis = millis();
  _PP(currentMillis);
  _PP(": Getting Score after: ");
  _PP((currentMillis - prevMillis) / 1000);
  _PL(" seconds");
  prevMillis = millis();
  WiFiClientSecure client;
  client.setInsecure();

  Serial.println("\nStarting connection to cricclubs server...");
  Serial.print("Match ID:");
  Serial.println(matchIdValue);
  Serial.print("Club ID:");
  Serial.println(clubIdValue);

  M5.Lcd.println("Starting connection to cricclubs server...");
  if (!client.connect(cricclubs_server, 443))
  {
    Serial.println("Connection to cricclubs failed!");
  }
  else
  {
    Serial.println("Connected to cricclubs server!");
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
    while (client.available())
    {
      MatchDetails matchDetails;
      matchDetails.getMatchDetails(client);
      if (matchDetails.isInitialized())
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
        String runsMessage = "Runs: ";
        runsMessage += matchDetails.getRuns();
        M5.Lcd.println(runsMessage);
        String wicketsMessage = "Wickets: ";
        wicketsMessage += matchDetails.getWickets();
        M5.Lcd.println(wicketsMessage);
        String oversMessage = "Overs: ";
        oversMessage += matchDetails.getOvers();
        M5.Lcd.println(oversMessage);
        if (prev_runs != matchDetails.getRuns() || prev_overs != matchDetails.getOvers() || prev_wickets != matchDetails.getWickets())
        {
          prev_runs = matchDetails.getRuns();
          prev_overs = matchDetails.getOvers();
          prev_wickets = matchDetails.getWickets();
          setDials(matchDetails, dials);
          config_updated = true;
        } else {
          config_updated = false;
          Serial.println("No update required as previous values are same");
        }
      }
      else
      {
        Serial.println("No Title found for ");
        Serial.println(clubIdValue);
        Serial.println(matchIdValue);
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.println("No Title found for ");
        M5.Lcd.println(clubIdValue);
        M5.Lcd.println(matchIdValue);
      }
      break;
    }
    client.stop();
    Serial.println("Connection to cricclubs server closed.");
  }
}

void saveConfigCB()
{
  Serial.println("\nChecking whether config was updated...");

  if (config_updated)
  {
    Serial.println("\nSaving config...");
    M5.Lcd.println("Saving config...");
    for (int i = 0; i < NUM_DIALS; i++)
    {
      dials[i].print();
      itoa(dials[i].getPos(), internalClockPosValue[i], 10);
    }
    iotWebConf.saveConfig();
    config_updated = false;
    Serial.println("Config saved.");
    M5.Lcd.println("Config saved.");
  }
  else
  {
    Serial.println("No config changes to save.");
    M5.Lcd.println("No config changes to save.");
  }
}

inline void LEDOn()
{
  digitalWrite(LED_BUILTIN, HIGH);
}

inline void LEDOff()
{
  digitalWrite(LED_BUILTIN, LOW);
}

// === 1 =======================================
bool LED_state = false;
void blink1CB()
{
  if (tBlink1.isFirstIteration())
  {
    _PP(millis());
    _PL(": Blink1 - simple flag driven");
    LED_state = false;
  }

  if (LED_state)
  {
    LEDOff();
    LED_state = false;
  }
  else
  {
    LEDOn();
    LED_state = true;
  }

  if (tBlink1.isLastIteration())
  {
    tBlink1.restartDelayed(2 * TASK_SECOND);
    LEDOff();
  }
}

// Initialize the IO ports
void setup()
{
  M5.begin();
  M5.Lcd.println("hello world.");

  Serial.begin(115200);
  pwm.begin();
  pwm.setPWMFreq(1000); // Set to whatever you like, we don't use it in this demo!

  // if you want to really speed stuff up, you can go into 'fast 400khz I2C' mode
  // some i2c devices dont like this so much so if you're sharing the bus, watch
  // out for this!
  Wire.setClock(400000);
  Serial.println();
  Serial.println("Starting up...");

  for (int i = 0; i < NUM_DIALS; i++)
  {
    labels[i] = "Dial " + String(i + 1) + " ";
    ids[i] = "dial_" + String(i + 1);
    if (i < 3)
    {
      String pos = String(i + 1);
      labels[i] += "Runs Digit " + pos + ":";
      ids[i] += "_runs_" + pos;
    }
    else if ((i >= 3) && (i < 6))
    {
      String pos = String(i - 3);
      labels[i] += "Overs Digit " + pos + ":";
      ids[i] += "_overs_" + pos;
    }
    else if ((i >= 6) && (i < 8))
    {
      String pos = String(i - 6);
      labels[i] += "Wickets Digit " + String(i - 6) + ":";
      ids[i] += "_wickets_" + pos;
    }
    Serial.println("Label: " + labels[i]);
    Serial.println("ID: " + ids[i]);
    internalClockPos[i] = new IotWebConfNumberParameter(labels[i].c_str(), ids[i].c_str(), internalClockPosValue[i], NUMBER_LEN, "0", "0..9", "min='0' max='9' step='1'");
    sbSettings.addItem(internalClockPos[i]);
  }

  Serial.println("internal clock Pos conf items added...");

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
  Serial.println("initializing iotwebconf...");

  iotWebConf.init();
  Serial.println("iotwebconf initialized...");

  // Serial.printf("Prev Position =  %d \n", prevPos);

  Serial.println("initializing dials...");

  // Initialize the dials
  // Each dial requires two pins to connect to the clock. The order does not matter.
  // The first dial should be connected to pins 3, 4, and the second to pins 5, 6, and so on.
  int clockA = 3;
  int clockB = 4;

  for (int i = 0; i < NUM_DIALS; i++)
  {
    int value = atoi(internalClockPosValue[i]);
    dials[i].init(clockA + i * 2, clockB + i * 2, &pwm, value);
    // calculate the number of runs using the first 3 digits of the internal clock position
    if (i < 3)
    {
      prev_runs += value * pow(10, (i));
    }
    // calculate the number of overs using the 4th - 6th digits of the internal clock position
    else if ((i >= 3) && (i < 6))
    {
      prev_overs += value * pow(10, (i - 3));
    }
    // calculate the number of wickets using the 7th - 8th digits of the internal clock position
    else if ((i >= 6) && (i < 8))
    {
      prev_wickets += value * pow(10, (i - 6));
    }
  }

  Serial.println("dials initialized...");

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", []
            { iotWebConf.handleConfig(); });
  server.onNotFound([]()
                    { iotWebConf.handleNotFound(); });

  Serial.println("Setup completed. Ready.");
}

char rx_byte = 0;

bool move_one_step()
{
  bool move_needed = false;
  for (int i = 0; i < NUM_DIALS; i++)
  {
    if (dials[i].moveOneStep())
    {
      move_needed = true;
      break;
    }
  }
  // Serial.println("move_needed: " + String(move_needed));
  return move_needed;
}

// Main loop
void loop()
{
  // goto the end position and then process
  // web commands and
  // query cricinfo every 60 seconds
  if (!move_one_step())
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      M5.Lcd.println(WiFi.localIP());
      M5.Lcd.println(clubIdValue);
      M5.Lcd.println(matchIdValue);
      // Serial.println("Executing scheduled task.");
      ts.execute();
    }
    // else
    // {
    //   Serial.println("Wifi not connected yet.");
    // }
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
  s += "Go to <a href='config'>configure page</a> to change values.";
  s += "</body></html>\n";
  // int prevPos = atoi(internalClockPosValue);
  // Serial.printf("Prev Position =  %d \n", prevPos);
  server.send(200, "text/html", s);
}

void configSaved()
{
  int desPos = 0;
  for (int i = 0; i < NUM_DIALS; i++)
  {
    const char *clockPosValue = &internalClockPosValue[i][0];
    sscanf(clockPosValue, "%d", &desPos);
    Serial.printf("New Desired Position =  %d \n", desPos);
    dials[i].setPos(desPos);
  }

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
