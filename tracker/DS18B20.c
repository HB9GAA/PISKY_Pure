#include "Configuration.h"

void *DS18B20Loop(void *some_void_ptr)
	{
	DIR *dir;
	struct dirent *dp;
	char *folder = "/sys/bus/w1/devices";
	FILE *fp;
	char line[100], filename[100];
	char *value;
	float Temperature;
	struct TGPS *GPS;
	int SensorCount;

	GPS = (struct TGPS *)some_void_ptr;
	GPS->DS18B20Count = 0;

	while (1)
		{
		if ((dir = opendir(folder)) != NULL)
			{
			SensorCount = 0;
			while (((dp = readdir(dir)) != NULL) && (SensorCount < 2))
				{
				if (strlen(dp->d_name) > 3)
					{
					if ((dp->d_name[0] != 'W') && (dp->d_name[2] == '-'))
						{
						sprintf(filename, "%.40s/%.30s/w1_slave", folder, dp->d_name);
						if ((fp = fopen(filename, "r")) != NULL)
							{
							// 44 02 4b 46 7f ff 0c 10 ee : crc=ee YES
							// 44 02 4b 46 7f ff 0c 10 ee t=36250
							if (fgets(line, sizeof(line), fp) != NULL)
								{
								if (strstr(line, "YES") != NULL)
									{
									if (fgets(line, sizeof(line), fp) != NULL)
										{
										strtok(line, "=");
										value = strtok(NULL, "\n");
										Temperature = atof(value) / 1000;
										// printf("%d: %5.3fC\n", SensorCount, Temperature);
										GPS->DS18B20Temperature[SensorCount++] = Temperature;
										}
									}
								}
							fclose(fp);
							}
						}
					}
				}
			if (SensorCount > GPS->DS18B20Count) GPS->DS18B20Count = SensorCount;
			// printf("%d DS18B20-Sensoren gefunden\n", SensorCount);
			
			closedir(dir);
			}
		
		if (GPS->DS18B20Count == 0)
			{
			//Use the GPU sensor instead
			FILE *fp;
			double T;
			
			fp = fopen ("/sys/class/thermal/thermal_zone0/temp", "r");
			if (fp != NULL)
				{
				fscanf (fp, "%lf", &T);
				GPS->DS18B20Temperature[0] = T / 1000;
				// printf ("GPU-Temperatur: %6.3f C.\n", GPS->DS18B20Temperature[0]);
				GPS->DS18B20Count = 1;
				fclose (fp);
				}
			}		
		sleep(5);
		}
	}
