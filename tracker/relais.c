#include "Configuration.h"

uint8_t Relais[4] = {1, 4, 26, 29};			//Initial pin numbers of relays 0 to 3
int RelaisStatusTemp[4] = {0, 0, 0, 0};	
	
	
//
//--------------------------------------------------------------------------------------------
void WriteRelaisLog(struct TGPS *GPS)
	{
	if (Config.EnableTelemetryLogging)
		{
		FILE *fp = NULL;
		
		if ((fp = fopen("relais.txt", "at")) != NULL)
			{
			fprintf(fp, "%li, Alt = %dm,  Vspeed = %5.2fm/s, R0 = %i, R1 = %i, R2 = %i, R3 = %i \n", time(NULL), GPS->Altitude, GPS->AscentRate, RelaisStatusTemp[0], RelaisStatusTemp[1], RelaisStatusTemp[2], RelaisStatusTemp[3]);
			//fputs(Buffer, fp);
			fclose(fp);
			}
		}
	}


//
//--------------------------------------------------------------------------------------------
void RelaisStatus(struct TGPS *GPS)
	{
	printf("\nRelais-Status at GPS-Alt = %5dm and Vspeed = %5.2fm/s\n", GPS->Altitude, GPS->AscentRate);		
	printf("|------|------|--------|--------|--------|--------|\n");	
	printf("|Relais|Status|  Asc-ON| Asc-OFF|  Des-ON| Des-OFF|\n");	
	for (int Rel = 0; Rel < 4; Rel++)
		{
		printf("|------|------|--------|--------|--------|--------|\n");		
		printf("|  %d   |  %d   |  %5dm|  %5dm|  %5dm|  %5dm|\n", 
						Rel, RelaisStatusTemp[Rel], 
						Config.RelaisConfig[Rel].AscendON,
						Config.RelaisConfig[Rel].AscendOFF,
						Config.RelaisConfig[Rel].DescendON,
						Config.RelaisConfig[Rel].DescendOFF);
		}
	printf("|------|------|--------|--------|--------|--------|\n");	
	}	


//
//--------------------------------------------------------------------------------------------
void *RelaisLoop(void *some_void_ptr)
	{
	struct TGPS *GPS;
	GPS = (struct TGPS *)some_void_ptr;

	for (int i = 0; i < 4; i++)
		{
		Config.RelaisConfig[i].RelaisPin = Relais[i];
		pinMode (Config.RelaisConfig[i].RelaisPin, OUTPUT);
		}

	RelaisStatus(GPS);
		
	while (1)
		{
		for (int i = 0; i < 4; i++)
			{
			if(GPS->AscentRate > 0.02)
				{
				if((Config.RelaisConfig[i].AscendON >= 0) || (Config.RelaisConfig[i].AscendOFF >= 0))
					{
					if((GPS->Altitude > Config.RelaisConfig[i].AscendON) && ((Config.RelaisConfig[i].AscendON >= 0) && (Config.RelaisConfig[i].AscendOFF < 0)))
						RelaisStatusTemp[i] = 1;

					else if((GPS->Altitude > Config.RelaisConfig[i].AscendON) && (GPS->Altitude < Config.RelaisConfig[i].AscendOFF))
						RelaisStatusTemp[i] = 1;
					
					else
						RelaisStatusTemp[i] = 0;					
					}
				}
			else if(GPS->AscentRate < -0.02)
				{
				if((Config.RelaisConfig[i].DescendON >= 0) || (Config.RelaisConfig[i].DescendOFF >= 0))
					{
					if((GPS->Altitude > Config.RelaisConfig[i].DescendOFF) && (Config.RelaisConfig[i].DescendOFF >= 0) && (Config.RelaisConfig[i].DescendON < 0))
						RelaisStatusTemp[i] = 1;

					else if((GPS->Altitude < Config.RelaisConfig[i].DescendON) && (GPS->Altitude > Config.RelaisConfig[i].DescendOFF))	
						RelaisStatusTemp[i] = 1;
					
					else
						RelaisStatusTemp[i] = 0;				
					}
				}
				
			digitalWrite(Config.RelaisConfig[i].RelaisPin, RelaisStatusTemp[i]);
			}

		RelaisStatus(GPS);
		WriteRelaisLog(GPS);
		sleep(Config.RelaisUpdateRate);
		}

	return 0;
	}
	