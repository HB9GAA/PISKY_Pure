/* ========================================================================== */
/*   gps.c                                                                    */
/*                                                                            */
/*   i2c bit-banging code for ublox on Pi A/A+/B/B+                           */
/*                                                                            */
/* ========================================================================== */

#define _GNU_SOURCE 1

#include "Configuration.h"

struct gps_info 
	{
	uint8_t address; 								//7 bit address
	uint8_t sda; 										//pin used for sda coresponds to gpio
	uint8_t scl; 										//clock
	uint32_t clock_delay; 					//proportional to bus speed
	uint32_t timeout;
	int Failed;
	int fd;													//For serial port GPS
	enum {cmNone, cmI2C, cmSerial} ConnectionMode;
	};


//offener Bus, legt die Struktur fest und initialisiert GPIO
//Die scl- und sda-Zeile sind so eingestellt, dass sie immer 0 (low) Ausgang sind, 
//wenn ein high benötigt wird, sind sie als Eingang eingestellt.
//--------------------------------------------------------------------------------------------
int OpenGPSPort(struct gps_info *bb,
				char *SerialDevice,
				uint8_t adr, 									//7 bit address
				uint8_t data,   							//GPIO Pin für Daten 
				uint8_t clock,  							//GPIO Stift für Clock
				uint32_t delay, 							//Clock delay us
				uint32_t timeout) 						//Clock stretch & timeout
	{
	bb->fd = -1;
	bb->Failed = 0;
	bb->ConnectionMode = cmNone;
	
	if (*SerialDevice)
		{
		bb->fd = open(SerialDevice, O_RDWR | O_NOCTTY | O_NDELAY);
		
		if (bb->fd >= 0)
			{
			struct termios options;
			
			fcntl(bb->fd, F_SETFL, 0);

			tcgetattr(bb->fd, &options);

			options.c_lflag &= ~ECHO;
			options.c_cc[VMIN]  = 0;
			options.c_cc[VTIME] = 10;

			options.c_cflag &= ~CSTOPB;
			cfsetispeed(&options, B9600);
			cfsetospeed(&options, B9600);
			options.c_cflag |= CS8;

			tcsetattr(bb->fd, TCSANOW, &options);
			
			printf("Opened serial GPS Port\n");
			bb->ConnectionMode = cmSerial;
			}
		else
			{
			printf("*** Öffnen der seriellen GNSS-Schnittstelle fehlgeschlagen ***\n");
			bb->Failed = 1;
			}
		}
	else
		{
		bb->address = adr;
		bb->sda = data;
		bb->scl = clock;
		bb->clock_delay = delay;
		bb->timeout = timeout;
		
		//sie sollten niedrig eingestellt werden, Eingang - Ausgang bestimmt das Niveau
		pinMode(bb->sda, INPUT);
		pinMode(bb->scl, INPUT);

		digitalWrite(bb->sda, LOW);
		digitalWrite(bb->scl, LOW);

		pullUpDnControl(bb->sda, PUD_UP);
		pullUpDnControl(bb->scl, PUD_UP);
		
		printf("Offener I2C-GNSS-Port\n");
		bb->ConnectionMode = cmI2C;
		}

	return bb->Failed;
	}


void BitDelay(uint32_t delay)
	{
  struct timespec sleeper, dummy ;

  sleeper.tv_sec  = 0;
  sleeper.tv_nsec = delay;
  nanosleep (&sleeper, &dummy) ;
	}


void CloseGPSPort(struct gps_info *bb)
	{
	if (bb->ConnectionMode == cmSerial)
		{
		close(bb->fd);
		}
	else if (bb->ConnectionMode == cmI2C)
		{
		int i;
		
		pinMode(bb->sda, INPUT);
		digitalWrite(bb->scl, LOW);
		
		for (i=0; i<16; i++)
			{
			pinMode(bb->scl, OUTPUT);
			BitDelay(bb->clock_delay);
			pinMode(bb->scl, INPUT);
			BitDelay(bb->clock_delay);
			}
		}
	
	bb->ConnectionMode = cmNone;
	}


// clock with stretch - bit level
// setzt die Taktleitung hoch und prüft, ob sie hoch geht. Wenn die Bit-Pegel-Verlängerung verwendet wird, 
// muss der Takt bei jedem Übergang überprüft werden.
//--------------------------------------------------------------------------------------------
void I2CClockHigh(struct gps_info *bb)
	{
    uint32_t to = bb->timeout;
	
	pinMode(bb->scl, INPUT);
	
    // check that it is high
	while (!digitalRead(bb->scl))
		{
		BitDelay(1000);
		if(!to--)
			{
			fprintf(stderr, "gps_info: Clock line held by slave\n");
			bb->Failed = 1;
			return;
			}
    }
	}

//
//--------------------------------------------------------------------------------------------
void I2CClockLow(struct gps_info *bb)
	{
	pinMode(bb->scl, OUTPUT);
	}


//
//--------------------------------------------------------------------------------------------
void I2CDataLow(struct gps_info *bb)
	{
	pinMode(bb->sda, OUTPUT);
	}


//
//--------------------------------------------------------------------------------------------
void I2CDataHigh(struct gps_info *bb)
	{
	pinMode(bb->sda, INPUT);
	}

	
// Returns 1 if bus is free, i.e. both sda and scl high
//--------------------------------------------------------------------------------------------
int BusIsFree(struct gps_info *bb)
	{
	return digitalRead(bb->sda) && digitalRead(bb->scl);
	}


// Startbedingung
// This is when sda is pulled low when clock is high. This also puls the clock
// low ready to send or receive data so both sda and scl end up low after this.
//--------------------------------------------------------------------------------------------
void I2CStart(struct gps_info *bb)
	{
	uint32_t to = bb->timeout;
	//der Bus muss für die Startbedingung frei sein
	while(to-- && !BusIsFree(bb))
		{
		BitDelay(5000);
		}

	if (!BusIsFree(bb))
		{
		fprintf(stderr, "gps_info: Cannot set start condition\n");
		bb->Failed = 1;
		return;
    }

  // start condition is when data linegoes low when clock is high
	I2CDataLow(bb);
	BitDelay((bb->clock_delay)/2);
	I2CClockLow(bb);
	BitDelay(bb->clock_delay);
	}


// stop condition
// when the clock is high, sda goes from low to high
//--------------------------------------------------------------------------------------------
void I2CStop(struct gps_info *bb)
	{
	I2CDataLow(bb);
	BitDelay(bb->clock_delay);
	I2CClockHigh(bb); // clock will be low from read/write, put high
	BitDelay(bb->clock_delay);
	I2CDataHigh(bb);
	}


// sends a byte to the bus, this is an 8 bit unit so could be address or data
// msb first
// returns 1 for NACK and 0 for ACK (0 is good)
//--------------------------------------------------------------------------------------------
int I2CSend(struct gps_info *bb, uint8_t value)
	{
	uint32_t rv;
	uint8_t j, mask=0x80;

	// clock is already low from start condition
	for(j=0;j<8;j++)
		{
		BitDelay(bb->clock_delay);
		if (value & mask)
			{
			I2CDataHigh(bb);
			}
		else
			{
			I2CDataLow(bb);
			}
		// clock out data
		I2CClockHigh(bb);  // clock it out
		BitDelay(bb->clock_delay);
		I2CClockLow(bb);      // back to low so data can change
		mask>>= 1;      // next bit along
    }
    // release bus for slave ack or nack
	I2CDataHigh(bb);
	BitDelay(bb->clock_delay);
	I2CClockHigh(bb);     // and clock high tels slave to NACK/ACK
	BitDelay(bb->clock_delay); // delay for slave to act
	rv = digitalRead(bb->sda);     // get ACK, NACK from slave

	I2CClockLow(bb);
	BitDelay(bb->clock_delay);
	return rv;
	}


// receive 1 char from bus
// Input
// send: 1=nack, (last byte) 0 = ack (get another)
//--------------------------------------------------------------------------------------------
uint8_t I2CRead(struct gps_info *bb, uint8_t ack)
	{
	uint8_t j, data=0;

	for (j=0;j<8;j++)
		{
		data<<= 1;      // shift in
		BitDelay(bb->clock_delay);
		I2CClockHigh(bb);      // set clock high to get data
		BitDelay(bb->clock_delay); // delay for slave
		
		if (digitalRead(bb->sda)) 
			data++;   // get data
		
		I2CClockLow(bb);
		}

	// clock has been left low at this point
	// send ack or nack
	BitDelay(bb->clock_delay);
   
	if (ack)
		{
		I2CDataHigh(bb);
		}
	else
		{
		I2CDataLow(bb);
		}
   
	BitDelay(bb->clock_delay);
	I2CClockHigh(bb);    // clock it in
	BitDelay(bb->clock_delay);
	I2CClockLow(bb);
	I2CDataHigh(bb);

	return data;
	}
	

// writes buffer
//--------------------------------------------------------------------------------------------
void I2Cputs(struct gps_info *bb, uint8_t *s, uint32_t len)
	{
	I2CStart(bb);
	I2CSend(bb, bb->address * 2); // address
	while(len) 
		{
		I2CSend(bb, *(s++));
		len--;
		}
	I2CStop(bb); // stop    
	}


// read one byte from GPS
//--------------------------------------------------------------------------------------------
uint8_t GPSGetc(struct gps_info *bb)
	{
	uint8_t Character;
	
	Character = 0xFF;
	
	if (bb->ConnectionMode == cmSerial)
		{
		if (read(bb->fd, &Character, 1) < 0)
			{
			bb->Failed = 1;
			bb->ConnectionMode = cmNone;
			}
		}
	else if (bb->ConnectionMode == cmI2C)
		{
		I2CStart(bb);
		I2CSend(bb, (bb->address * 2)+1); // address
		Character = I2CRead(bb, 1);
		I2CStop(bb); // stop
		}
	
	return Character;
	}


//
//--------------------------------------------------------------------------------------------
int GPSChecksumOK(char *Buffer, int Count)
	{
	unsigned char XOR, i, c, Star;

	// Find star
	for (Star=Count; (Star > 1) && (Buffer[Star] != '*'); Star--);
	
	XOR = 0;
	for (i = 1; i < Star; i++)
		{
		c = Buffer[i];
		XOR ^= c;
		}

	return (Buffer[Star] == '*') && (Buffer[Star+1] == Hex(XOR >> 4)) && (Buffer[Star+2] == Hex(XOR & 15));
	}


//
//--------------------------------------------------------------------------------------------
void FixUBXChecksum(unsigned char *Message, int Length)
	{ 
	int i;
	unsigned char CK_A, CK_B;

	CK_A = 0;
	CK_B = 0;

	for (i=2; i<(Length-2); i++)
		{
		CK_A = CK_A + Message[i];
		CK_B = CK_B + CK_A;
		}

	Message[Length-2] = CK_A;
	Message[Length-1] = CK_B;
	}


//
//--------------------------------------------------------------------------------------------
void SendToGPS(struct gps_info *bb, unsigned char *MSG, int len)
	{
	if (bb->ConnectionMode == cmSerial)
		{
		write(bb->fd, MSG, len);
		}
	else if (bb->ConnectionMode == cmI2C)
		{
		I2Cputs(bb, MSG, len);
		}
	}


//
//--------------------------------------------------------------------------------------------
void SetFlightMode(struct gps_info *bb, int High)
	{
	// Send navigation configuration command
	unsigned char FlightMode[] = {0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0xDC};
	unsigned char PedestrianMode[] = {0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00, 0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x76};
	
	if (High)
		{
		SendToGPS(bb, FlightMode, sizeof(FlightMode));
		printf ("Setting UBlox flight mode\n");
		}
	else
		{
		SendToGPS(bb, PedestrianMode, sizeof(PedestrianMode));
		printf ("Setting UBlox pedestrian mode\n");
		}
	}


//
//--------------------------------------------------------------------------------------------
void SetPowerMode(struct gps_info *bb, int SavePower)
	{
	unsigned char setPSM[] = {0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x01, 0x22, 0x92 };
	
	setPSM[7] = SavePower ? 1 : 0;
	printf ("Setting power-saving %s\n", SavePower ? "ON" : "OFF");
	FixUBXChecksum(setPSM, sizeof(setPSM));
	SendToGPS(bb, setPSM, sizeof(setPSM));
	}


void setGPS_GNSS(struct gps_info *bb)
	{
	// Sets CFG-GNSS to disable everything other than GPS GNSS
	// solution. Failure to do this means GPS power saving 
	// doesn't work. Not needed for MAX7, needed for MAX8's
	unsigned char setgnss[] = {
	0xB5, 0x62, 0x06, 0x3E, 0x2C, 0x00, 0x00, 0x00,
	0x20, 0x05, 0x00, 0x08, 0x10, 0x00, 0x01, 0x00,
	0x01, 0x01, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00,
	0x01, 0x01, 0x03, 0x08, 0x10, 0x00, 0x00, 0x00,
	0x01, 0x01, 0x05, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x01, 0x01, 0x06, 0x08, 0x0E, 0x00, 0x00, 0x00,
	0x01, 0x01, 0xFC, 0x11   };

	printf ("Disabling GNSS\n");
	SendToGPS(bb, setgnss, sizeof(setgnss));
	}


//
//--------------------------------------------------------------------------------------------
void setGPS_DynamicModel6(struct gps_info *bb)
	{
	uint8_t setdm6[] = {
	0xB5, 0x62, 0x06, 0x24, 0x24, 0x00, 0xFF, 0xFF, 0x06,
	0x03, 0x00, 0x00, 0x00, 0x00, 0x10, 0x27, 0x00, 0x00,
	0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00, 0x64, 0x00, 0x2C,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0xDC};

	printf ("Setting dynamic model 6\n");
	SendToGPS(bb, setdm6, sizeof(setdm6));
	}


//
//--------------------------------------------------------------------------------------------
float FixPosition(float Position)
	{
	float Minutes, Seconds;

	Position = Position / 100;
	Minutes = trunc(Position);
	Seconds = fmod(Position, 1);
	return Minutes + Seconds * 5 / 3;
	}


//
//--------------------------------------------------------------------------------------------
time_t day_seconds()
	{
	time_t t1, t2;
	struct tm tms;
	time(&t1);
	localtime_r(&t1, &tms);
	tms.tm_hour = 0;
	tms.tm_min = 0;
	tms.tm_sec = 0;
	t2 = mktime(&tms);
	return t1 - t2;
	}


//
//--------------------------------------------------------------------------------------------
void ProcessLine(struct gps_info *bb, struct TGPS *GPS, char *Buffer, int Count, int ActionMask)
	{
	static int SystemTimeHasBeenSet=0;
	
	float utc_time, latitude, longitude, hdop, altitude;
	float geoid; 													//added by HB9GAA
	int lock, satellites;
	char active, ns, ew, units, timestring[16], speedstring[16], *course, *date, restofline[80], *ptr;
	
	if (GPSChecksumOK(Buffer, Count))
		{
		satellites = 0;
	
		if (strncmp(Buffer+3, "GGA", 3) == 0)
			{
			GPS->MessageCount++;
			if (GPS->FlightMode >= fmLaunched)
				{
				GPS->SecondsSinceLaunch++;
				}
			
			// $GPGGA,124943.00,5157.01557,N,00232.66381,W,1,09,1.01,149.3,M,48.6,M,,*42
			//"geoid" added by HB9GAA
			if (sscanf(Buffer+7, "%f,%f,%c,%f,%c,%d,%d,%f,%f,%c,%f", &utc_time, &latitude, &ns, &longitude, &ew, &lock, &satellites, &hdop, &altitude, &units, &geoid) >= 1)
				{	
				if (satellites >= 4)
					{
					unsigned long utc_seconds;
					
					if (ActionMask & 1)
						{
						utc_seconds = utc_time;
						GPS->Hours = utc_seconds / 10000;
						GPS->Minutes = (utc_seconds / 100) % 100;
						GPS->Seconds = utc_seconds % 100;
						GPS->SecondsInDay = GPS->Hours * 3600 + GPS->Minutes * 60 + GPS->Seconds;					
						// printf("\nGGA: %ld seconds offset\n\n", GPS->SecondsInDay - day_seconds());
						}
					
					if (ActionMask & 2)
						{
						latitude = FixPosition(latitude);
						if (ns == 'S') latitude = -latitude;
						longitude = FixPosition(longitude);
						if (ew == 'W') longitude = -longitude;
						
						GPS->Latitude = latitude;
						GPS->Longitude = longitude;				
					
						//added by HB9GAA
						//-----------------------------------------
						if (Config.UNIXformat)									//We format for the HaB-Traker from HB9GAA
							{
							altitude += geoid;											//Geoid height calculation
							}
						//-----------------------------------------
						
						if (GPS->Altitude <= 0)
							{
							GPS->AscentRate = 0;
							}
						else
							{
							GPS->AscentRate = GPS->AscentRate * 0.7 + ((int32_t)altitude - GPS->Altitude) * 0.3;
							}
							
						// printf("Altitude=%ld, AscentRate = %.1lf\n", GPS->Altitude, GPS->AscentRate);
						GPS->Altitude = altitude;
						
						if (GPS->Altitude > GPS->MaximumAltitude)
							{
							GPS->MaximumAltitude = GPS->Altitude;
							GPS->BurstLatitude = GPS->Latitude;
							GPS->BurstLongitude = GPS->Longitude;
							}
							
						if ((GPS->Altitude < GPS->MinimumAltitude) || (GPS->MinimumAltitude == 0))
							{
							GPS->MinimumAltitude = GPS->Altitude;						
							}

						// Launched?
						if ((GPS->AscentRate >= 1.0) && (GPS->Altitude > (GPS->MinimumAltitude+150)) && (GPS->FlightMode == fmIdle))
							{
							GPS->FlightMode = fmLaunched;
							printf("*** LAUNCHED ***\n");
							}

						// Burst?
						if ((GPS->AscentRate < -10.0) && (GPS->Altitude < (GPS->MaximumAltitude+50)) && (GPS->MaximumAltitude >= (GPS->MinimumAltitude+2000)) && (GPS->FlightMode == fmLaunched))
							{
							GPS->FlightMode = fmDescending;
							printf("*** DESCENDING ***\n");
							}
						
						// Landed?
						if ((GPS->AscentRate >= -0.1) && (GPS->FlightMode >= fmDescending) && (GPS->FlightMode < fmLanded))
							{
							GPS->FlightMode = fmLanded;
							printf("*** LANDED ***\n");
							}
						}
					}
				if (ActionMask & 2)
					{
					GPS->Satellites = satellites;
					}
				}

			WriteGPSLog(Buffer);
			}
			
//	$GPRMC,220516,A,5133.82,N,00042.24,W,173.8,231.8,130694,004.2,W*70
//	         1    2    3    4    5     6    7    8      9     10  11 12
//	$GPRMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,x.x,a*hh
//	1    = UTC of position fix
//	2    = Data status (V=navigation receiver warning)
//	3    = Latitude of fix
//	4    = N or S
//	5    = Longitude of fix
//	6    = E or W
//	7    = Speed over ground in knots
//	8    = Track made good in degrees True
//	9    = UT date
//	10   = Magnetic variation degrees (Easterly var. subtracts from true course)
//	11   = E or W
//	12   = Checksum			
			
		else if (strncmp(Buffer+3, "RMC", 3) == 0)
			{
			speedstring[0] = '\0';
			if (sscanf(Buffer+7, "%[^,],%c,%f,%c,%f,%c,%[^,],%s", timestring, &active, &latitude, &ns, &longitude, &ew, speedstring, restofline) >= 7)
				{			
				// $GPRMC,124943.00,A,5157.01557,N,00232.66381,W,0.039,,200314,,,A*6C

				ptr = restofline;
				course = strsep(&ptr, ",");
				date = strsep(&ptr, ",");
				GPS->Speed = (int)atof(speedstring);
				GPS->Direction = (int)atof(course);

				if ((atof(timestring) > 0) && !SystemTimeHasBeenSet)
					{
					struct tm tm;
					char timedatestring[32];
					time_t t;

					// Now create a tm structure from our date and time
					memset(&tm, 0, sizeof(struct tm));
					sprintf(timedatestring, "%c%c-%c%c-20%c%c %c%c:%c%c:%c%c",
											date[0], date[1], date[2], date[3], date[4], date[5],
											timestring[0], timestring[1], timestring[2], timestring[3], timestring[4], timestring[5]);
					strptime(timedatestring, "%d-%m-%Y %H:%M:%S", &tm);
				
					t = mktime(&tm);
										
					if (stime(&t) == -1)
						{
						printf("Failed to set system time\n");
						}
					else
						{
						printf("System time set from GPS time\n");
						SystemTimeHasBeenSet = 1;
						}
					}
				}

			WriteGPSLog(Buffer);
			}
		else if (strncmp(Buffer+3, "GSV", 3) == 0)
			{
			printf("Disabling GSV\r\n");
			unsigned char setGSV[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x39 };
			SendToGPS(bb, setGSV, sizeof(setGSV));
			}
		else if (strncmp(Buffer+3, "GLL", 3) == 0)
			{
			printf("Disabling GLL\r\n");
			unsigned char setGLL[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x2B };
			SendToGPS(bb, setGLL, sizeof(setGLL));
			}
		else if (strncmp(Buffer+3, "GSA", 3) == 0)
			{
			printf("Disabling GSA\r\n");
			unsigned char setGSA[] = { 0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x32 };
			SendToGPS(bb, setGSA, sizeof(setGSA));
			}
		else if (strncmp(Buffer+3, "VTG", 3) == 0)
			{
			printf("Disabling VTG\r\n");
			unsigned char setVTG[] = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x05, 0x47};
			SendToGPS(bb, setVTG, sizeof(setVTG));
			}
    }
	else
    {
		printf("Bad checksum %d %d %s\r\n", Count, strlen(Buffer), Buffer);
		}
	}


//
//--------------------------------------------------------------------------------------------
void *GPSLoop(void *some_void_ptr)
	{
	char Line[100];
	int Length;
	struct gps_info bb;
	struct TGPS *GPS;
	FILE *fp;
	int SentenceCount;
	
	GPS = (struct TGPS *)some_void_ptr;
	
	fp = NULL;
	if (Config.GPSSource[0])
		{
		fp = fopen(Config.GPSSource, "rt");
		}
	
	Length = 0;
	SentenceCount = 0;
	
	if (fp != NULL)
	{
		printf("NMEA File GPS using %s\n", Config.GPSSource);
	}
	else if (*Config.GPSDevice)
	{
		printf("Serial GPS using %s\n", Config.GPSDevice);
	}
	else
	{
		printf ("I2C GPS using SDA = %d, SCL = %d\n", Config.SDA, Config.SCL);
	}
	
    while (1)
    {
		unsigned char Character;
	
		if (OpenGPSPort(&bb, Config.GPSDevice, 0x42, Config.SDA, Config.SCL, 2000, 100))		// struct, i2c address, SDA, SCL, ns clock delay, timeout ms
		{
			printf("Failed to open GPS\n");
			bb.Failed = 1;
		}
			
        while (!bb.Failed)
        {
            Character = GPSGetc(&bb);
			// if (Character == 0xFF) printf("."); else printf("%c", Character);

			if (Character == 0xFF)
			{
				delay(100);
			}
            else if (Character == '$')
			{
				Line[0] = Character;
				Length = 1;
			}
            else if (Length > 90)
			{
				Length = 0;
            }
            else if ((Length > 0) && (Character != '\r'))
            {
               	Line[Length++] = Character;
               	if (Character == '\n')
               	{
               		Line[Length] = '\0';
					if (Config.ShowGPS)
					{
						printf("%s", Line);
					}
					
					// if ((fp != NULL) && (strstr(Line, "GGA") != NULL))
					if (fp != NULL)
					{
						static int PostFileCount=0;
						static char Buffer[100];
						
						// Get time only from GPS, then get position from NMEA file
						if (fgets(Buffer, sizeof Buffer, fp) == NULL)
						{
							if (++PostFileCount > 20)
							{
								fclose(fp);
								exit(1);
							}
						}
						ProcessLine(&bb, GPS, Line, Length, 1);
						ProcessLine(&bb, GPS, Buffer, strlen(Buffer), 2);
					}
					else
					{				
						ProcessLine(&bb, GPS, Line, Length, 3);
						
						if (++SentenceCount > 100) SentenceCount = 0;
						
						if ((SentenceCount == 10) && Config.Power_Saving)
						{
							setGPS_GNSS(&bb);
						}					
						else if ((SentenceCount == 20) && Config.Power_Saving)
						{
							setGPS_DynamicModel6(&bb);
						}
						else if (SentenceCount == 30)
						{
							SetPowerMode(&bb, Config.Power_Saving && (GPS->Satellites > 4));
						}
						else if (SentenceCount == 40)
						{
							SetFlightMode(&bb, GPS->Altitude > Config.Flight_Mode_Altitude);
						}
					}

               		Length = 0;
					delay(100);
               	}
            }
		}
		
		CloseGPSPort(&bb);
	}
}

