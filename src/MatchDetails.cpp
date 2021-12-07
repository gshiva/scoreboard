#include <regex>
#include <string>
#include "MatchDetails.h"

using namespace std;

MatchDetails::MatchDetails()
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

void MatchDetails::setRuns(int runs)
{
    this->runs = runs;
    int temp = runs;
    for (int i = 0; i < 3; i++)
    {
        runDigits[i] = temp % 10;
        temp /= 10;
    }
}

void MatchDetails::setWickets(int wickets)
{
    this->wickets = wickets;
    int temp = wickets;
    for (int i = 0; i < 2; i++)
    {
        wicketDigits[i] = temp % 10;
        temp /= 10;
    }
}

void MatchDetails::setOvers(int overs)
{
    this->overs = overs;
    int temp = overs;
    for (int i = 0; i < 3; i++)
    {
        overDigits[i] = temp % 10;
        temp /= 10;
    }
}

void MatchDetails::setInitialized(bool initialized)
{
    this->initialized = initialized;
}

int MatchDetails::getRuns()
{
    return runs;
}

int MatchDetails::getWickets()
{
    return wickets;
}

int MatchDetails::getOvers()
{
    return overs;
}

int *MatchDetails::getRunDigits()
{
    return runDigits;
}

int *MatchDetails::getWicketDigits()
{
    return wicketDigits;
}

int *MatchDetails::getOverDigits()
{
    return overDigits;
}

bool MatchDetails::isInitialized()
{
    return initialized;
}

void MatchDetails::print()
{
    _PP("runs: ");
    _PL(runs);
    _PP("wickets: ");
    _PL(wickets);
    _PP("overs: ");
    _PL(overs);
}

MatchDetails *MatchDetails::getMatchDetails(WiFiClientSecure &client)
{
    String sline("not empty");

    Serial.println("Getting match details...");

    while (!this->isInitialized() && sline.length() > 0)
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
                this->setRuns(runs.toInt());
                this->setWickets(wickets.toInt());
                this->setOvers(overs.toInt());
                this->setInitialized(true);
            }
            this->print();
        }
    }

    return this;
}