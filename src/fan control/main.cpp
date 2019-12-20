// g++ main.cpp -Wall -o test -lwiringPi
#include <wiringPi.h>
#include <wiringPiI2C.h>
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
#include <time.h>

using namespace std;
using namespace std::this_thread;     // sleep_for, sleep_until
using namespace std::chrono_literals; // ns, us, ms, s, h, etc.
using std::chrono::system_clock;
/* pin configuration ------------------------------------------------

*/
#define MaxPWM 256
#define PWM_pin 1   /* GPIO 1 as per WiringPi, GPIO18 as per BCM */
#define TachoPin 3
#define BME280_ADDRESS                0x76

#define BME280_REGISTER_DIG_T1        0x88
#define BME280_REGISTER_DIG_T2        0x8A
#define BME280_REGISTER_DIG_T3        0x8C

typedef struct
{
  uint16_t dig_T1;
  int16_t  dig_T2;
  int16_t  dig_T3;
} bme280_calib_data;

typedef struct
{
  uint8_t tmsb;
  uint8_t tlsb;
  uint8_t txsb;

  uint32_t temperature;

} bme280_raw_data;

/* temperature ------------------------------------------------------

*/
int fd;
float temp = 20.0f; //current temperature
float cpuTemp = 20.0f;
/* fan speed --------------------------------------------------------

*/
int pwmIntensity = 20;
/* fan tacho --------------------------------------------------------

*/
int tacho = 0;
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

int setUpTemp() {
    fd = wiringPiI2CSetup(0x76);
	if(fd < 0) 
    {
        printf("Device not found");
        return -1;
	}
    return 1;
}

void readCalibrationData(int fd, bme280_calib_data *data) {
  data->dig_T1 = (uint16_t)wiringPiI2CReadReg16(fd, BME280_REGISTER_DIG_T1);
  data->dig_T2 = (int16_t)wiringPiI2CReadReg16(fd, BME280_REGISTER_DIG_T2);
  data->dig_T3 = (int16_t)wiringPiI2CReadReg16(fd, BME280_REGISTER_DIG_T3);
}

void getRawData(int fd, bme280_raw_data *raw) {
  wiringPiI2CWrite(fd, 0xfa);

  raw->tmsb = wiringPiI2CRead(fd);
  raw->tlsb = wiringPiI2CRead(fd);
  raw->txsb = wiringPiI2CRead(fd);

  raw->temperature = 0;
  raw->temperature = (raw->temperature | raw->tmsb) << 8;
  raw->temperature = (raw->temperature | raw->tlsb) << 8;
  raw->temperature = (raw->temperature | raw->txsb) >> 4;
}

int32_t getTemperatureCalibration(bme280_calib_data *cal, int32_t adc_T) {
  int32_t var1  = ((((adc_T>>3) - ((int32_t)cal->dig_T1 <<1))) *
     ((int32_t)cal->dig_T2)) >> 11;

  int32_t var2  = (((((adc_T>>4) - ((int32_t)cal->dig_T1)) *
       ((adc_T>>4) - ((int32_t)cal->dig_T1))) >> 12) *
     ((int32_t)cal->dig_T3)) >> 14;

  return var1 + var2;
}

float compensateTemperature(int32_t t_fine) {
  float T  = (t_fine * 5 + 128) >> 8;
  return T/100;
}

void readRealTemp()
{
  	wiringPiI2CWriteReg8(fd, 0xf4, 0x25);   // temperature oversampling x 1, mode normal
	bme280_calib_data cal;
	readCalibrationData(fd, &cal);

	bme280_raw_data raw;
	getRawData(fd, &raw);

	int32_t t_fine = getTemperatureCalibration(&cal, raw.temperature);
	temp = compensateTemperature(t_fine);
}

float readTemp()
{
    return random(15, 45);
}

void readCPUTemperature()
{
    FILE *temperatureFile;
    temperatureFile = fopen ("/sys/class/thermal/thermal_zone0/temp", "r");
    fscanf (temperatureFile, "%f", &cpuTemp);
    cpuTemp /= 1000;
    fclose (temperatureFile);
}

void setUpPWM() {
    pinMode (PWM_pin, PWM_OUTPUT);
    pwmSetRange(MaxPWM);
    pwmSetClock(4);
}

int calcPWM(int x1, int y1, int x2, int y2)
{
    return ((y2-y1)/(x2-x1)*(temp-x1)+ y1);
}

void getPWMSpeed()
{
    pair<int, int> oldVal;
    oldVal.first = 0;
    oldVal.second = 0;
    for (auto &speed : speedList) // access by reference to avoid copying
    {
        if(temp < speed.first)
        {
            continue;
        }
        else
        {
            if(oldVal.first > 0)
            {
                if(pwmIntensity > MaxPWM)
                {
                    pwmIntensity = MaxPWM;
                }
                else if(pwmIntensity < 1)
                {
                    pwmIntensity = 1;
                }
                else
                {
                    pwmIntensity = calcPWM(oldVal.first, oldVal.second, speed.first, speed.second);
                }
            }
            else
            {
                pwmIntensity = speed.second;
            }
            
        }
        oldVal = speed;
    }
}

void setPWM()
{
    using namespace std::string_literals;
    ofstream f("/sys/module/testpwm/parameters/duty");
    getPWMSpeed();
    //pwmWrite(PWM_pin, pwmIntensity);
    //string cmd = "echo '90' | sudo tee /sys/module/testpwm/parameters/duty";
    //system(cmd.data());
    f << pwmIntensity << endl; 
    //cout << cmd;
}

void setUpTacho() {
    pinMode(TachoPin, INPUT);
	pTimer = (float)clock()/CLOCKS_PER_SEC;
}

void readTacho()
{
    if((pTimer +1)<((float)clock()/CLOCKS_PER_SEC))
    {
        tacho  = rcount * 60;
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
    if (wiringPiSetup() == -1) exit(1);
    speedList = readConfig();
    setUpTemp();
    setUpTacho();
    //setUpPWM();
    int test = 0;
    while(true)
    {
        readTacho();
        if(test > 500000)
        {
            readRealTemp();
            readCPUTemperature();
            setPWM();
            
            std::cout << "current outer temperature " << temp << '\n';
            std::cout << "current cpu temperature " << cpuTemp << '\n';
            std::cout << "current speed fan speed " << tacho << "rpm \n";
            std::cout << "setted fan PWM " << pwmIntensity << '\n';
            test = 0;
        }
        test++;
    }
}
