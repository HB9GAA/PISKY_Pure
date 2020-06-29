#include "Configuration.h"

uint8_t Relais[4] = {1, 4, 26, 29};			//Initial pin numbers of relays 0 to 3
int temp = 0;

void *RelaisLoop(void *some_void_ptr)
	{
	struct TConfig Config;
	struct TGPS *GPS;
	GPS = (struct TGPS *)some_void_ptr;		

	for (int i = 0; i < 4; i++)
		{
		Config.RelaisConfig[i].RelaisPin = Relais[i];
		}


	// We have 2 LED outputs
	pinMode (Config.RelaisConfig[0].RelaisPin, OUTPUT);
	pinMode (Config.RelaisConfig[1].RelaisPin, OUTPUT);
	pinMode (Config.RelaisConfig[2].RelaisPin, OUTPUT);
	pinMode (Config.RelaisConfig[3].RelaisPin, OUTPUT);
	
	
	while (1)
		{
		digitalWrite (Config.RelaisConfig[0].RelaisPin, temp & 0x01);
		digitalWrite (Config.RelaisConfig[1].RelaisPin, temp & 0x02);
		digitalWrite (Config.RelaisConfig[2].RelaisPin, temp & 0x04);
		digitalWrite (Config.RelaisConfig[3].RelaisPin, temp & 0x08);
		temp++;
		
		usleep(1200000);
		
		}

	return 0;
	}
	