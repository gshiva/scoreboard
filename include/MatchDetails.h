#ifndef _MATCH_DETAILS_H
#define _MATCH_DETAILS_H

#include <WiFiClientSecure.h>

#ifdef _DEBUG_
#define _PP(a) Serial.print(a);
#define _PL(a) Serial.println(a);
#else
#define _PP(a)
#define _PL(a)
#endif

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
    MatchDetails();
    void setRuns(int runs);
    void setWickets(int wickets);
    void setOvers(int overs);
    void setInitialized(bool initialized);
    int getRuns();
    int getWickets();
    int getOvers();
    bool isInitialized();
    int *getRunDigits();
    int *getWicketDigits();
    int *getOverDigits();
    void print();
    MatchDetails *getMatchDetails(WiFiClientSecure &client);

};
#endif