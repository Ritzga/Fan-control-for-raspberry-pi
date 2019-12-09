// g++ test.cpp -Wall -o test -lwiringPi
//#include <wiringPi.h>
#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <utility>
#include <chrono>
#include <thread>

using namespace std;
using namespace std::this_thread;     // sleep_for, sleep_until
using namespace std::chrono_literals; // ns, us, ms, s, h, etc.
using std::chrono::system_clock;

#define MaxPWM 256
#define PWM_pin = 1;   /* GPIO 1 as per WiringPi, GPIO18 as per BCM */
#define TachoPin 3;

float temp = 20.00; //current temperature
int pwmIntensity = 20;
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

int readTemp()
{
    return random(15, 45);
}

int main ()
{
    speedList = readConfig();
    while(true)
    {
        temp = readTemp();
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
            //std::cout << "at temperature " << speed.first << " with speed " << speed.second << '\n';
        }
        std::cout << "current temperature " << temp << " with setted speed " << pwmIntensity << '\n';
        sleep_for(5s);
    }
    /*
    int intensity ;

    if (wiringPiSetup() == -1)
    {
        cout<<"Hello"<<endl;
        exit(1);
    }
    pinMode (PWM_pin, PWM_OUTPUT);

    pwmSetRange(MaxPWM);
    pwmSetClock(4);

    while (1)
    {
        for (intensity = 0; intensity < MaxPWM; intensity++){
        pwmWrite(PWM_pin, intensity);
        delay(10);
        cout<<intensity<<endl;
        }
        for (intensity = MaxPWM; intensity > 0; intensity --){
        pwmWrite(PWM_pin, intensity);
        delay(10);
        cout<<intensity<<endl;
        }
        //pwmWrite (PWM_pin, 128) ;
    }*/
}
