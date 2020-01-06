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

using namespace std;
using namespace std::this_thread; // sleep_for, sleep_until
using namespace std::chrono_literals; // ns, us, ms, s, h, etc.
using std::chrono::system_clock;
/* --------------------------------------------------------------------
    pin configuration
*/
#define MaxPWM 256 //max speed of fan
#define PWM_pin 1 // GPIO 1 as per WiringPi, GPIO18 as per BCM
#define TachoPin 3 //tacho pin of fan
#define BME280_ADDRESS                0x76 //bus address of bme280

#define BME280_REGISTER_DIG_T1        0x88 //temperature 1 address
#define BME280_REGISTER_DIG_T2        0x8A //temperature 2 address
#define BME280_REGISTER_DIG_T3        0x8C //temperature 3 address

// struct for bme280 data
typedef struct
{
  uint16_t dig_T1;
  int16_t  dig_T2;
  int16_t  dig_T3;
} bme280_calib_data;

// struct for bme280 temperature
typedef struct
{
  uint8_t tmsb;
  uint8_t tlsb;
  uint8_t txsb;

  uint32_t temperature;

} bme280_raw_data;

/*  --------------------------------------------------------------------
    temperature
*/
int fd; //readed raw tempe
float temp = 20.0f; //current temperature
float cpuTemp = 20.0f; //current cpu temperature
/*  --------------------------------------------------------------------
    fan speed
*/
int pwmIntensity = 20; //current intensity
/*  --------------------------------------------------------------------
    fan tacho
*/
float tacho = 0.0f; //current tacho value
bool ps_tachoPin = false; //if current readed
int rcount = 0; //how often readed
float pTimer; //time for read interval
/*  --------------------------------------------------------------------
    settings
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

/*  --------------------------------------------------------------------
    code
*/
/**
 * Read values out of config
 *
 * @return vector of pairs of two integer (temp => speed)
 */
vector<pair<int, int>> readConfig()
{
    vector<pair<int, int>> list;
    string line;
    ifstream myfile;
    myfile.open("config.fennec"); //open the config file
    if(!myfile.is_open())
    {
        perror("Error open");
        exit(EXIT_FAILURE);
    }
    string delimiter = "="; //element between values
    string s;
    while(getline(myfile, line))
    {
        std::string delimiter = "=";
        size_t pos = 0;
        std::string token;
        pos = line.find(delimiter); //find position of element between values
        token = line.substr(0, pos); //get the first value
        line.erase(0, pos + delimiter.length()); //delete the first value out of string to get the second value
        pair<int, int> p1 = {stoi(token), stoi(line)}; //save both values to pait
        list.push_back(p1); //push the pait to the vector
    }
    return list;
}

/**
 * Set up the wiringPi for the bme280
 *
 * @return int for error log
 */
int setUpTemp()
{
    fd = wiringPiI2CSetup(0x76); //give wiringPi the bus address of bme280
	if(fd < 0)
    {
        printf("Device not found");
        return -1;
	}
    return 1;
}

/**
 * Read the raw data of bme280
 *
 * @return integer of raw readed data
 */
void readCalibrationData(int fd, bme280_calib_data *data)
{
  data->dig_T1 = (uint16_t)wiringPiI2CReadReg16(fd, BME280_REGISTER_DIG_T1);
  data->dig_T2 = (int16_t)wiringPiI2CReadReg16(fd, BME280_REGISTER_DIG_T2);
  data->dig_T3 = (int16_t)wiringPiI2CReadReg16(fd, BME280_REGISTER_DIG_T3);
}

/**
 * Read the raw data of bme280
 *
 * @return integer of raw readed data
 */
void getRawData(int fd, bme280_raw_data *raw)
{
  wiringPiI2CWrite(fd, 0xfa); //set bme280 to read mode

  //set bits of the readed data
  raw->tmsb = wiringPiI2CRead(fd);
  raw->tlsb = wiringPiI2CRead(fd);
  raw->txsb = wiringPiI2CRead(fd);

  raw->temperature = 0;
  raw->temperature = (raw->temperature | raw->tmsb) << 8;
  raw->temperature = (raw->temperature | raw->tlsb) << 8;
  raw->temperature = (raw->temperature | raw->txsb) >> 4;
}

/**
 * Calculate the temperature with bosch equation
 *
 * @return temperature value
 */
int32_t getTemperatureCalibration(bme280_calib_data *cal, int32_t adc_T)
{
    //I can't explain what happend here
  int32_t var1  = ((((adc_T>>3) - ((int32_t)cal->dig_T1 <<1))) *
     ((int32_t)cal->dig_T2)) >> 11;

  int32_t var2  = (((((adc_T>>4) - ((int32_t)cal->dig_T1)) *
       ((adc_T>>4) - ((int32_t)cal->dig_T1))) >> 12) *
     ((int32_t)cal->dig_T3)) >> 14;
  return var1 + var2;
}

/**
 * compensate the temperature
 *
 * @return temperature value
 */
float compensateTemperature(int32_t t_fine)
{
  float T  = (t_fine * 5 + 128) >> 8;
  return T/100;
}

/**
 * the main function for read a temperature
 *
 * @return temperature value
 */
void readRealTemp()
{
  	wiringPiI2CWriteReg8(fd, 0xf4, 0x25);   // pressure and temperature oversampling x 1, mode normal
	bme280_calib_data cal;
	readCalibrationData(fd, &cal);

	bme280_raw_data raw;
	getRawData(fd, &raw);

	int32_t t_fine = getTemperatureCalibration(&cal, raw.temperature);
	temp = compensateTemperature(t_fine);
	cout << temp << endl;
}

/**
 * read cpu temperature in standard linux file for cpu temperature
 *
 * @return cpu temperature
 */
void readCPUTemperature()
{
    FILE *tempFile;
    tempFile = fopen("sys/class/thermal/thermal_zone0/temp", "r");
    fscanf(tempFile, "%f", &cpuTemp);
    cpu /= 1000;
    fclose(tempFile);
}

/**
 * Calculate the linear value of the pwm intensity
 *
 * @return delta value of the linear function
 */
int calcPWM (int x1, int y1, int x2, int y2)
{
    return ((y2-y1)/(x2-x1)*(temp-x1)+y1);
}

/**
 * find the hihger pwm speed out of the settings with the current temperature
 *
 * @return pwm intensity
 */
void getPWMSpeed()
{
    pair<int, int> oldVar;
    oldVar.first = 0;
    oldVar.second = 0;
    for (auto &speed : speedList) // access by reference to avoid copying
    {
        if(temp < speed.first)
        {
            continue;
        }
        else
        {
            if(oldVar.first > 0)
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
                    pwmIntensity = calcPWM(oldVar.first, oldVar.second, speed.first, speed.second)
                }
            }
            else
            {
                pwmIntensity = speed.second;
            }
        }
        oldVar = speed;
    }
}

/**
 * set the pwm intensity to the kernal space - module parameter
 *
 */
void setPWM()
{
    using namespace std::string_literals;
    ofstream f("/sys/module/testpwm/parameters/duty"); //path of the parameter
    getPWMSpeed(); //get the current intensity
    f << pwmIntensity << endl; //write to the parameter
    //cout << cmd; //debug value
}

/**
 * set up the tacho
 *
 */
void setUpTacho()
{
    pinMode(TachoPin, INPUT);
    pTimer = (float)clock()/CLOCKS_PER_SEC;
}
/**
 * read the hights of the tacho value in a time sequence
 *
 * @return tacho speed of the fan
 */
void readTacho()
{
    if ((pTimer + 1)<((float)clock()/CLOCKS_PER_SEC)) //if one second is gone
    {
        tacho  = rcount * 60;
        cout << tacho <<endl; //debug
        rcount = 0;
        pTimer = (float)clock()/CLOCKS_PER_SEC;
    }
    if (digitalRead(TachoPin)==HIGH) //count the high
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

/**
 * main function
 *
 */
int main ()
{
    if (wiringPiSetup() ==-1) exit(1); //setup the wiringPi
    speedList = readConfig(); //load the config
    setUpTacho(); //setup the tacho reading
    setUpTemp(); //setup the temperature reading
    int interval = 0;
    while(true) //endless
    {
        readTacho(); //read tacho speed
        if(interval > 500000)
        {
            readRealTemp(); //read temperature
            readCPUTemperature();
            setPWM(); //set pwm intensity
            std::cout << "current outer temperature " << temp << '\n';
            std::cout << "current cpu temperature " << cpuTemp << '\n';
            std::cout << "current speed fan speed " << tacho << "rpm \n";
            std::cout << "setted fan PWM " << pwmIntensity << '\n';
            test = 0;
        }
    }
}
