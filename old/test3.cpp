#include <unistd.h>				//Needed for I2C port
#include <sys/ioctl.h>			//Needed for I2C port
#include <linux/i2c-dev.h>		//Needed for I2C port
#include <cstdio>
#include <fstream>
#include <fcntl.h>
#include <iostream>




using namespace std;


	
int main(){
	
	int fd = wiringPiI2CSetup(0x76);
	if(fd < 0) {
		printf("Device not found");
		return -1;
	}
	
	  wiringPiI2CWriteReg8(fd, 0xf4, 0x25);   // pressure and temperature oversampling x 1, mode normal

	
	bme280_calib_data cal;
	readCalibrationData(fd, &cal);
	
	bme280_raw_data raw;
	getRawData(fd, &raw);
	
	int32_t t_fine = getTemperatureCalibration(&cal, raw.temperature);
	float t = compensateTemperature(t_fine); 
	
	cout << t << endl;

	return 0;
}
