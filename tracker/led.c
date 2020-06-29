#include "Configuration.h"


void *LEDLoop(void *some_void_ptr)
	{
	struct TGPS *GPS;
	int Flash=0;

	GPS = (struct TGPS *)some_void_ptr;

	// We have 2 LED outputs
	pinMode (Config.LED_Warn, OUTPUT);
	pinMode (Config.LED_OK, OUTPUT);
	
	while (1)
		{
		digitalWrite (Config.LED_Warn, (GPS->Satellites < 4) && (GPS->Altitude < 2000) && (GPS->MessageCount & 1));	
		digitalWrite (Config.LED_OK, (GPS->Satellites >= 4) && (GPS->Altitude < 2000) && (Flash ^= 1));	

		usleep(750000);
		}

	return 0;
	}
