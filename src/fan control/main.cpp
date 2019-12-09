// g++ test.cpp -Wall -o test -lwiringPi
//#include <wiringPi.h>
#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <utility>

#define MaxPWM 256

using namespace std;

#define PWM_pin = 1;   /* GPIO 1 as per WiringPi, GPIO18 as per BCM */
#define TachoPin 3

int main ()
{
    vector<pair<int, int>> speedList;
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
        speedList.push_back(p1);
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
