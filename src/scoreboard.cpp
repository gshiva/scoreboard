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
#include "meter.h"

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

#ifdef _DEBUG_
#define _PP(a) Serial.print(a);
#define _PL(a) Serial.println(a);
#else
#define _PP(a)
#define _PL(a)
#endif

#define PERIOD1 500
#define DURATION 10000
#define SCORE_PERIOD 60000
void blink1CB();
void getScoreCB();
Task tBlink1(PERIOD1 *TASK_MILLISECOND, DURATION / PERIOD1, &blink1CB, &ts, true);
Task tGetScore(SCORE_PERIOD *TASK_MILLISECOND, TASK_FOREVER, &getScoreCB, &ts, true);

unsigned long prevMillis = millis();
const char *cricclubs_server = "cricclubs.com";
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

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

using namespace std;
class MatchDetails
{
  int runs;
  int wickets;
  int overs;
  bool initialized;

  int runDigits[3];
  int wicketDigits[2];
  int overDigits[3];

public:
  MatchDetails()
  {
    runs = 0;
    wickets = 0;
    overs = 0;
    for (int i = 0; i < 3; i++)
    {
      runDigits[i] = 0;
      if (i < 2)
      {
        wicketDigits[i] = 0;
      }
      overDigits[i] = 0;
    }
    initialized = false;
  }

  void setRuns(int runs)
  {
    this->runs = runs;
    int temp = runs;
    for (int i = 0; i < 3; i++)
    {
      runDigits[i] = temp % 10;
      temp /= 10;
    }
  }

  void setWickets(int wickets)
  {
    this->wickets = wickets;
    int temp = wickets;
    for (int i = 0; i < 2; i++)
    {
      wicketDigits[i] = temp % 10;
      temp /= 10;
    }
  }

  void setOvers(int overs)
  {
    this->overs = overs;
    int temp = overs;
    for (int i = 0; i < 3; i++)
    {
      overDigits[i] = temp % 10;
      temp /= 10;
    }
  }

  void setInitialized(bool initialized)
  {
    this->initialized = initialized;
  }

  int getRuns()
  {
    return runs;
  }

  int getWickets()
  {
    return wickets;
  }

  int getOvers()
  {
    return overs;
  }

  int getRunDigit(int i)
  {
    return runDigits[i];
  }

  int getWicketDigit(int i)
  {
    return wicketDigits[i];
  }

  int getOverDigit(int i)
  {
    return overDigits[i];
  }

  bool isInitialized()
  {
    return initialized;
  }

  void print()
  {
    _PP("runs: ");
    _PL(runs);
    _PP("wickets: ");
    _PL(wickets);
    _PP("overs: ");
    _PL(overs);
  }
};

MatchDetails getMatchDetails(WiFiClientSecure &client)
{
  String sline("not empty");
  MatchDetails matchDetails;

  Serial.println("Getting match details...");

  while (!matchDetails.isInitialized() && sline.length() > 0)
  {
    sline = client.readStringUntil('\n');
    std::string line = sline.c_str();
    Serial.println(sline);
    if (line.find("description") != string::npos)
    {
      // sample line
      // <span id="line9"></span></span><span>&lt;<span class="start-tag">meta</span>
      // <span class="attribute-name">name</span>='<a class="attribute-value">description</a>'
      // <span class="attribute-name">content</span>='<a class="attribute-value">
      // INDIA won by 73 Run(s);INDIA 184/7(20.0 overs) NEW ZEALAND 111/10(17.2 overs)</a>'
      // <span>/</span>&gt;</span><span>

      // first team playing
      // SRI LANKA 267/3(88.0 overs)

      // second team playing
      // WEST INDIES 184/7(20.0 overs) SRI LANKA 267/3(88.0 overs)

      // get the Runs/Wickets/(Overs
      // using this regex /(\d+)\/(\d+)\((\d+)/gm

      std::regex regex_pattern("(\\d+)\\/(\\d+)\\((\\d+)");
      std::smatch match;
      string::const_iterator searchStart(line.cbegin());
      while (regex_search(searchStart, line.cend(), match, regex_pattern))
      {
        searchStart = match.suffix().first;
        std::string sruns = match[1];
        String runs(sruns.c_str());
        std::string swickets = match[2];
        String wickets(swickets.c_str());
        std::string sovers = match[3];
        String overs(sovers.c_str());
        matchDetails.setRuns(runs.toInt());
        matchDetails.setWickets(wickets.toInt());
        matchDetails.setOvers(overs.toInt());
        matchDetails.setInitialized(true);
      }
      matchDetails.print();
    }
  }

  return matchDetails;
}

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "testThing";

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "smrtTHNG8266";

const int NUM_METERS = 1;
Meter meters[NUM_METERS];
int prevPos[NUM_METERS];
char internalClockPosValue[NUM_METERS][NUMBER_LEN];
IotWebConfNumberParameter *internalClockPos[NUM_METERS];
char tournamentIdValue[NUMBER_LEN];
char clubIdValue[NUMBER_LEN];
char matchIdValue[NUMBER_LEN];

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
// -- You can also use namespace formats e.g.: iotwebconf::TextParameter
IotWebConfParameterGroup sbSettings = iotwebconf::ParameterGroup("sbSettings", "Scoreboard Settings");

// -- We can add a legend to the separator
IotWebConfNumberParameter clubId = IotWebConfNumberParameter("Club ID", "clubId", clubIdValue, NUMBER_LEN, "0", "1..1000000", "min='0' max='1000000' step='1'");
IotWebConfNumberParameter matchId = IotWebConfNumberParameter("Match ID", "matchId", matchIdValue, NUMBER_LEN, "0", "1..1000000", "min='0' max='1000000' step='1'");
IotWebConfTextParameter tournamentId = iotwebconf::TextParameter("Tournament ID", "tournamentId", tournamentIdValue, NUMBER_LEN, "NACL");

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
      MatchDetails matchDetails = getMatchDetails(client);
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

  for (int i = 0; i < NUM_METERS; i++)
  {
    internalClockPos[i] = new IotWebConfNumberParameter("Internal Clock Position", "internalClockPos", internalClockPosValue[i], NUMBER_LEN, "7", "1..100", "min='0' max='100' step='1'");
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

  int clockA = 3;
  int clockB = 4;
  // Serial.printf("Prev Position =  %d \n", prevPos);

  Serial.println("initializing meters...");

  for (int i = 0; i < NUM_METERS; i++)
  {
    meters[i].init(clockA + i * 2, clockB + i * 2, &pwm, atoi(internalClockPosValue[i]));
  }

  Serial.println("meters initialized...");

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
  bool move_needed = true;
  for (int i = 0; i < NUM_METERS; i++)
  {
    if (!meters[i].moveOneStep())
    {
      move_needed = false;
    }
  }
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
  // int prevPos = atoi(internalClockPosValue);
  // Serial.printf("Prev Position =  %d \n", prevPos);
  server.send(200, "text/html", s);
}

void configSaved()
{
  int desPos = 0;
  for (int i = 0; i < NUM_METERS; i++)
  {
    const char *clockPosValue = &internalClockPosValue[i][0];
    sscanf(clockPosValue, "%d", &desPos);
    Serial.printf("New Desired Position =  %d \n", desPos);
    meters[i].setPos(desPos);
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
