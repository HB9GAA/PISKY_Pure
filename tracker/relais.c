#include "Configuration.h"

uint8_t Relais[4] = {1, 4, 26, 29};			//Initial pin numbers of relays 0 to 3
int temp = 0;
int RelaisStatusTemp[4] = {0, 0, 0, 0};	
	
void *RelaisLoop(void *some_void_ptr)
	{
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
	
	
	for (int Rel = 0; Rel < 4; Rel++)
		{
		printf("Relais %d settings\n", Rel);
		printf("..    - ASC_ON = %5d m,\tASC_OFF = %5d m\n", Config.RelaisConfig[Rel].AscendON, Config.RelaisConfig[Rel].AscendOFF);
		printf("..    - DES_ON = %5d m,\tDES_OFF = %5d m\n", Config.RelaisConfig[Rel].DescendON, Config.RelaisConfig[Rel].DescendOFF);
		}
	
	while (1)
		{
		if(GPS->AscentRate > 0.1)
			{
			for (int i = 0; i < 4; i++)
				{		
				if((GPS->Altitude > Config.RelaisConfig[i].AscendON) && (GPS->Altitude < Config.RelaisConfig[i].AscendOFF))	
					digitalWrite (Config.RelaisConfig[i].RelaisPin, 1);
				else
					digitalWrite (Config.RelaisConfig[i].RelaisPin, 0);
				}
			}
		else if(GPS->AscentRate < 0.1)
			{
			for (int i = 0; i < 4; i++)
				{		
				if((GPS->Altitude < Config.RelaisConfig[i].AscendON) && (GPS->Altitude > Config.RelaisConfig[i].AscendOFF))	
					digitalWrite (Config.RelaisConfig[i].RelaisPin, 1);
				else
					digitalWrite (Config.RelaisConfig[i].RelaisPin, 0);
				}
			}
		

				printf("ASC_Rate = %5.1f m/s,\tASC_ON = %5d m,\tGPS-Alt = %5d m\n", GPS->AscentRate, Config.RelaisConfig[0].AscendON, GPS->Altitude);
		temp++;
		
		usleep(3000000);
		
		}

	return 0;
	}
	