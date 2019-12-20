//g++ test2.cpp -o test -Wall -lwiringPi

#include <wiringPi.h>
#include <iostream>
#include <stdio.h>
#include <time.h>

#define TachoPin 3

using namespace std;

void loop();
void setup();


bool ps_tachoPin = false;
int rcount = 0;
float pTimer;


int main()
{
	if (wiringPiSetup() == -1) exit(1);

	setup();

	while(true)
	{
		loop();
	}
	return 0;
}



void setup()
{
	pinMode(TachoPin, INPUT);
	pTimer = (float)clock()/CLOCKS_PER_SEC;
	//cout<<pTimer<<endl;
}

void loop()
{
	if ((pTimer +1 )<((float)clock()/CLOCKS_PER_SEC)){
		int rpm  = rcount * 60;
		cout <<rpm <<endl;
		rcount = 0;
		pTimer = (float)clock()/CLOCKS_PER_SEC;
	}
	//cout<<pTimer<<endl;
	if (digitalRead(TachoPin)==HIGH)
	{
		if (!ps_tachoPin)
			{
				ps_tachoPin=true;
				rcount++;
				//cout<<rcount<<endl;

			}
	}
	else
	{
		ps_tachoPin = false;
	}
}
