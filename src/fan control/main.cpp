// g++ main.cpp -Wall -o test -lwiringPi
#include <wiringPi.h>
#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <utility>
#include <chrono>
#include <thread>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;
using namespace std::this_thread;     // sleep_for, sleep_until
using namespace std::chrono_literals; // ns, us, ms, s, h, etc.
using std::chrono::system_clock;
/* pin configuration ------------------------------------------------

*/
#define MaxPWM 256
#define PWM_pin 1   /* GPIO 1 as per WiringPi, GPIO18 as per BCM */
#define TachoPin 3
/* temperature ------------------------------------------------------

*/
float temp = 20.0f; //current temperature
/* fan speed --------------------------------------------------------

*/
int pwmIntensity = 20;
/* fan tacho --------------------------------------------------------

*/
float tacho = 0.0f;
bool ps_tachoPin = false;
int rcount = 0;
float pTimer;
/* settings ---------------------------------------------------------

*/
vector<pair<int, int>> speedList; //list of all speed steps

int random(int min, int max) //range : [min, max)
{
   static bool first = true;
   if (first)
   {
      srand( time(NULL) ); //seeding for the first time only!
      first = false;
   }
   return min + rand() % (( max + 1 ) - min);
}

vector<pair<int, int>> readConfig()
{
    vector<pair<int, int>> list;
    string line;
    ifstream myfile;
    myfile.open("config.fennec");
    if(!myfile.is_open())
    {
        perror("Error open");
        exit(EXIT_FAILURE);
    }
    string delimiter = "=";
    string s;
    while(getline(myfile, line))
    {
        std::string delimiter = "=";
        size_t pos = 0;
        std::string token;
        pos = line.find(delimiter);
        token = line.substr(0, pos);
        line.erase(0, pos + delimiter.length());
        pair<int, int> p1 = {stoi(token), stoi(line)};
        list.push_back(p1);
    }
    return list;
}

void setUpTemp() {
    pinMode(TachoPin, INPUT);
    pTimer = (float)clock()/CLOCKS_PER_SEC;
}


float readRealTemp()
{
    int fd = open("/dev/file", O_RDWR | O_NONBLOCK);
    return 0;
    
}

float readTemp()
{
    return random(15, 45);
}

void setUpPWM() {
    pinMode (PWM_pin, PWM_OUTPUT);
    pwmSetRange(MaxPWM);
    pwmSetClock(4);
}

void getPWMSpeed()
{
    for (auto &speed : speedList) // access by reference to avoid copying
    {
        if(temp < speed.first)
        {
            continue;
        }
        else
        {
            pwmIntensity = speed.second;
        }
    }
}

void setPWM()
{
    getPWMSpeed();
    pwmWrite(PWM_pin, pwmIntensity);
    //pwmWrite (PWM_pin, 128) ;
}

void setUpTacho() {
    pinMode(TachoPin, INPUT);
    pTimer = (float)clock()/CLOCKS_PER_SEC;
}

void readTacho()
{

    if ((pTimer +1 )<((float)clock()/CLOCKS_PER_SEC)){
        tacho  = rcount * 60;
        cout << tacho <<endl;
        rcount = 0;
        pTimer = (float)clock()/CLOCKS_PER_SEC;
    }
    if (digitalRead(TachoPin)==HIGH)
    {
        if (!ps_tachoPin)
        {
            ps_tachoPin=true;
            rcount++;
        }
    }
    else
    {
        ps_tachoPin = false;
    }
}

int main ()
{
    speedList = readConfig();
    setUpTemp();
    setUpTacho();
    setUpPWM();
    while(true)
    {
        temp = readTemp();
        setPWM();
        std::cout << "current temperature " << temp << " with setted speed " << pwmIntensity << '\n';
        readTacho();
        sleep_for(2s);
    }
}
