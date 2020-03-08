// PI IN THE SKY TRACKER PROGRAM
//----------------------------------------------------------------------------
// This program is written for the Pi GNSS/LoRa Board
// produced by ELcon Consulting & Engineering.  
// No support is provided for use on other hardware. 
// It does the following:                 |
// 1 - Sets up the hardware including putting the GPS in flight mode
// 2 - Reads the current temperature
// 3 - Reads the current GPS position
// 5 - Builds a telemetry sentence to transmit
// 6 - repeats steps 2-6

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>   	// Standard input/output definitions
#include <string.h>  	// String function definitions
#include <unistd.h>  	// UNIX standard function definitions
#include <fcntl.h>   	// File control definitions
#include <errno.h>   	// Error number definitions
#include <termios.h> 	// POSIX terminal control definitions
#include <stdint.h>
#include <stdlib.h>
#include <dirent.h>
#include <math.h>
#include <pthread.h>
#include <wiringPi.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <sys/statvfs.h>
#include <pigpio.h> 
#include <inttypes.h>

#include "gps.h"
#include "DS18B20.h"
#include "misc.h"
#include "snapper.h"
#include "led.h"
#include "lora.h"
#include "log.h"

struct TConfig Config;

// Pin allocations.  Do not change unless you're using your own hardware
// WIRING PI PINS
#define UBLOX_ENABLE	2

int Records, FileNumber;
struct termios options;
char *SSDVFolder="/home/pi/PISKY_Pure/tracker/images";
 

void LoadConfigFile(struct TConfig *Config)
	{
	const char* CameraTypes[5] = {"None", "CSI Pi Camera - raspistill", "USB webcam - fswebcam", "USB camera - gphoto2", "Python Script"};
	FILE *fp;
	char *filename = "/boot/pisky.txt";

	if ((fp = fopen(filename, "r")) == NULL)
		{
		printf("\nFailed to open config file %s (error %d - %s).\nPlease check that it exists and has read permission.\n", filename, errno, strerror(errno));
		exit(1);
		}

 	//added by HB9GAA
	//-----------------------------------------
	ReadBoolean(fp, "UNIXformat", -1, 0, &(Config->UNIXformat));			//UNIXformat = 0 -> hh:mm:ss or UNIXformat = 1 -> UNIX-Time 
	if (Config->UNIXformat)
			{
			printf("Format: UNIX-Time\n");
			}
		else
			{
			printf("Format: Default\n");
			}
	//-----------------------------------------

	ReadBoolean(fp, "disable_monitor", -1, 0, &(Config->DisableMonitor));
	if (Config->DisableMonitor)
		{
		printf("HDMI/Composite outputs will be disabled\n");
		}
	
	// Logging
	Config->EnableGPSLogging = ReadBooleanFromString(fp, "logging", "GPS");
	if (Config->EnableGPSLogging) printf("GPS Logging enabled\n");

	Config->EnableTelemetryLogging = ReadBooleanFromString(fp, "logging", "Telemetry");
	if (Config->EnableTelemetryLogging) printf("Telemetry Logging enabled\n");
	
	Config->TelemetryFileUpdate = ReadInteger(fp, "telemetry_file_update", -1, 0, 0);
	if (Config->TelemetryFileUpdate > 0)
		{
		printf("Telemetry file 'latest.txt' will be created every %d seconds\n", Config->TelemetryFileUpdate);
		}
	
	Config->ExternalDS18B20 = ReadInteger(fp, "external_temperature", -1, 0, 1);
	if (Config->ExternalDS18B20)
		{
		printf("External DS18B20 Enabled\n");
		}
	
	Config->Camera = ReadCameraType(fp, "camera");
	printf ("Camera (%s) %s\n", CameraTypes[Config->Camera], Config->Camera ? "Enabled" : "Disabled");
	
	if (Config->Camera)
		{
		ReadString(fp, "camera_settings", -1, Config->CameraSettings, sizeof(Config->CameraSettings), 0);
		if (*Config->CameraSettings)
			{
			printf ("Adding custom camera parameters '%s' to raspistill calls\n", Config->CameraSettings);
			}
		
		Config->SSDVSettings[0] = '\0';
		ReadString(fp, "SSDV_settings", -1, Config->SSDVSettings, sizeof(Config->SSDVSettings), 0);
		if (*Config->SSDVSettings)
			{
			printf ("Adding custom SSDV parameters '%s'\n", Config->SSDVSettings);
			}

		Config->SSDVHigh = ReadInteger(fp, "high", -1, 0, 2000);
		printf ("Image size changes at %dm\n", Config->SSDVHigh);
	
		// Set up full-size image parameters		
		Config->Channels[FULL_CHANNEL].ImageWidthWhenLow = ReadInteger(fp, "full_low_width", -1, 0, 640);
		Config->Channels[FULL_CHANNEL].ImageHeightWhenLow = ReadInteger(fp, "full_low_height", -1, 0, 480);
		printf ("Full Low image size %d x %d pixels\n", Config->Channels[FULL_CHANNEL].ImageWidthWhenLow, Config->Channels[FULL_CHANNEL].ImageHeightWhenLow);
		
		Config->Channels[FULL_CHANNEL].ImageWidthWhenHigh = ReadInteger(fp, "full_high_width", -1, 0, 2592);
		Config->Channels[FULL_CHANNEL].ImageHeightWhenHigh = ReadInteger(fp, "full_high_height", -1, 0, 1944);
		printf ("Full High image size %d x %d pixels\n", Config->Channels[FULL_CHANNEL].ImageWidthWhenHigh, Config->Channels[FULL_CHANNEL].ImageHeightWhenHigh);

		Config->Channels[FULL_CHANNEL].ImagePeriod = ReadInteger(fp, "full_image_period", -1, 0, 60);
		printf ("Full size: %d seconds between photographs\n", Config->Channels[FULL_CHANNEL].ImagePeriod);
		
		Config->Channels[FULL_CHANNEL].ImagePackets = Config->Channels[FULL_CHANNEL].ImagePeriod > 0;
		Config->Channels[FULL_CHANNEL].Enabled = Config->Channels[FULL_CHANNEL].ImagePackets;
		}

	// GPS
	Config->GPSSource[0] = '\0';
	ReadString(fp, "gps_source", -1, Config->GPSSource, sizeof(Config->GPSSource), 0);
	ReadBoolean(fp, "Power_Saving", -1, 0, &(Config->Power_Saving));
	printf("GPS Power Saving = %s\n", Config->Power_Saving ? "ON" : "OFF");
	Config->Flight_Mode_Altitude = ReadInteger(fp, "Flight_Mode_Altitude", -1, 0, 1000);
	if (Config->Flight_Mode_Altitude) printf("Switching GPS to flight mode above %d metres\n", Config->Flight_Mode_Altitude);
	
	// External data file
	Config->ExternalDataFileName[0] = '\0';
	ReadString(fp, "external_data", -1, Config->ExternalDataFileName, sizeof(Config->ExternalDataFileName), 0);

	// Show GPS NMEA ?
	Config->ShowGPS = 0;
	ReadBoolean(fp, "show_gps", -1, 0, &(Config->ShowGPS));
	
	// Serial GPS
	Config->GPSDevice[0] = '\0';
	ReadString(fp, "gps_device", -1, Config->GPSDevice, sizeof(Config->GPSDevice), 0);
	
	if (!Config->GPSDevice[0])
		{
		// I2C overrides.  Only needed for users own boards, or for some of our prototypes
		if (ReadInteger(fp, "SDA", -1, 0, 0))
			{
			Config->SDA = ReadInteger(fp, "SDA", -1, 0, 0);
			printf ("I2C SDA overridden to %d\n", Config->SDA);
			}

		if (ReadInteger(fp, "SCL", -1, 0, 0))
			{
			Config->SCL = ReadInteger(fp, "SCL", -1, 0, 0);
			printf ("I2C SCL overridden to %d\n", Config->SCL);
			}
		}
	
	Config->InfoMessageCount = ReadInteger(fp, "info_messages", -1, 0, -1);
		
	LoadLoRaConfig(fp, Config);
	
	fclose(fp);
	}

char *SerialPortName(void)
	{
	// Put this here in case the serial port name changes sometime
	
	return "/dev/ttyAMA0";
	}

int OpenSerialPort(void)
	{
	int fd;

	fd = open(SerialPortName(), O_WRONLY | O_NOCTTY);	// O_NDELAY);
	if (fd >= 0)
		{
		/* get the current options */
		tcgetattr(fd, &options);

		/* set raw input */
		options.c_lflag &= ~ECHO;
		options.c_cc[VMIN]  = 0;
		options.c_cc[VTIME] = 10;

		cfsetispeed(&options, Config.TxSpeed);
		cfsetospeed(&options, Config.TxSpeed);
		options.c_cflag |= CSTOPB;
		options.c_cflag &= ~CSIZE;
		if (Config.TxSpeed == B50)
			{
			options.c_cflag |= CS7;
			}
		else
			{
			options.c_cflag |= CS8;
			}
		options.c_oflag &= ~ONLCR;
		options.c_oflag &= ~OPOST;
		options.c_iflag &= ~IXON;
		options.c_iflag &= ~IXOFF;
	
		tcsetattr(fd, TCSANOW, &options);
		}

	return fd;
	}

void SendSentence(int fd, char *TxLine)
	{
	write(fd, TxLine, strlen(TxLine));

	// Log now while we're waiting for the serial port, to eliminate or at least reduce downtime whilst logging
	WriteTelemetryLog(TxLine);
	
	// Wait till those characters get sent
	tcsetattr(fd, TCSAFLUSH, &options);
	}

void SendFreeSpace(int fd)
	{
	struct statvfs vfs;

	if (statvfs("/home", &vfs) == 0)
		{
		char Sentence[200];
		
		sprintf(Sentence, "Free SD space = %.1fMB\n", (float)vfs.f_bsize * (float)vfs.f_bfree / (1024 * 1024));
		printf(Sentence);
		SendSentence(fd, Sentence);
		}
	}

int LoRaChannelUploadNow(int LoRaChannel, struct TGPS *GPS, int PacketTime)
	{
	// Can't use time till we have it
	if ((Config.LoRaDevices[LoRaChannel].UplinkCycle > 0) && (Config.LoRaDevices[LoRaChannel].UplinkPeriod > 0))
		{
		int i;
		long CycleSeconds;
		
		for (i=0; i<=PacketTime; i++)
			{
			CycleSeconds = (GPS->SecondsInDay+i) % Config.LoRaDevices[LoRaChannel].UplinkCycle;
	
			if (CycleSeconds < Config.LoRaDevices[LoRaChannel].UplinkPeriod)
				{
				return 1;
				}
			}
		}
	
	return 0;
	}

int LoRaUploadNow(struct TGPS *GPS, int PacketTime)
	{
	// Can't use time till we have it
	if (Config.QuietRTTYDuringLoRaUplink && (GPS->Satellites > 0)) // && (GPS->Altitude > Config.SSDVHigh))
		{
		return (LoRaChannelUploadNow(0, GPS, PacketTime) || LoRaChannelUploadNow(1, GPS, PacketTime));
		}
	
	return 0;
	}

int main(void)
	{
	int fd=0;
	int i;
	struct stat st = {0};
	struct TGPS GPS;
	pthread_t LoRaThread, GPSThread, DS18B20Thread, CameraThread, LEDThread, LogThread;
	
	if (prog_count("tracker") > 1)
		{
		printf("\nDas Tracker-Programm läuft bereits!\n");
		printf("Es wird automatisch mit dem Kameraskript gestartet, wenn der Pi bootet.\n\n");
		printf("If you just want the tracker software to run, it already is,\n");
		printf("and its output can be viewed on a monitor attached to a Pi video socket.\n\n");
		printf("Wenn Sie die Tracker-Ausgabe über ssh ansehen möchten,\n");
		printf("dann stoppen Sie das Programm mit dem Befehls:\n");
		printf("	sudo killall tracker\n\n");
		printf("und starten es manuell mit\n");
		printf("	sudo ./tracker\n\n");
		exit(1);
		}
	
	printf("\n\nRASPBERRY PI-IN-THE-SKY FLIGHT COMPUTER\n");
	printf(    "=======================================\n\n");

	Config.BoardType = GetBoardType(&Config.i2cChannel);

	if (Config.BoardType)
		{
		if ((Config.BoardType == 4) || (Config.BoardType == 3))
			{
			printf("RPi Zero W oder RPi Zero\n");
			}
		else
			{
			if (Config.BoardType == 2)
				{
				printf("RPi 3B\n");
				}
			else
				{
				printf("RPi Model A+ oder B+ or B V2\n");
				}
			}
				
		Config.LED_OK = 25;
		Config.LED_Warn = 24;
		
		Config.SDA = 2;
		Config.SCL = 3;
		}
	
	LoadConfigFile(&Config);

	if (Config.DisableMonitor)
		{
		system("/opt/vc/bin/tvservice -off");
		}
	
	if (FileExists("/boot/clear.txt"))
		{
		// remove SSDV and other camera images, plus log files
		printf("Entfernen von vorhandenen Fotodateien\n");
		remove("gps.txt");
		remove("telemetry.txt");
		remove("/boot/clear.txt");
		system("rm -rf /home/pi/PISKY_Pure/tracker/images/*");
		}
		
	// Remove any old SSDV files
	system("rm -f ssdv*.bin");

	memset(&GPS, 0, sizeof(struct TGPS));
	
	// Set up I/O
	if (wiringPiSetup() == -1)
		{
		printf("Kann WiringPi nicht initialisieren\n");
		exit (1);
		}
	
	if (gpioInitialise() < 0)
		{
		printf("Pigpio-Initialisierung fehlgeschlagen.\n");
		exit (1);
		}	
		
	// Switch on the GPS
	if (Config.BoardType == 0)
		{
		// Only PITS board had this, not PITS+
		pinMode (UBLOX_ENABLE, OUTPUT);
		digitalWrite (UBLOX_ENABLE, 0);
		}
			
	// SSDV Folders
	*Config.Channels[0].SSDVFolder = '\0';										// No folder for RTTY images
	*Config.Channels[1].SSDVFolder = '\0';										// No folder for APRS images
	sprintf(Config.Channels[2].SSDVFolder, "%s/LORA0", SSDVFolder);
	*Config.Channels[3].SSDVFolder = '\0';										// No folder for LORA1 images
	sprintf(Config.Channels[4].SSDVFolder, "%s/FULL", SSDVFolder);
		
	if (Config.Camera)
		{
		// Create SSDV Folders
		if (stat(SSDVFolder, &st) == -1)
			{
			mkdir(SSDVFolder, 0777);
			}	
	
		for (i = 0; i < 5; i++)
			{
			if (*Config.Channels[i].SSDVFolder)
				{
				if (stat(Config.Channels[i].SSDVFolder, &st) == -1)
					{
					mkdir(Config.Channels[i].SSDVFolder, 0777);
					}
				}
			}

		// Filenames for SSDV
		for (i = 0; i < 5; i++)
			{
			sprintf(Config.Channels[i].take_pic, "take_pic_%d", i);
			sprintf(Config.Channels[i].convert_file, "convert_%d", i);
			sprintf(Config.Channels[i].ssdv_done, "ssdv_done_%d", i);
			
			Config.Channels[i].SSDVImageNumber = -1;
			Config.Channels[i].SSDVPacketNumber = -1;
			
			Config.Channels[i].ImageFP = NULL;
			}
		}
	
	if (pthread_create(&GPSThread, NULL, GPSLoop, &GPS))
		{
		fprintf(stderr, "Fehler beim Erstellen eines GPS-Threads\n");
		return 1;
		}
	
	if (Config.LoRaDevices[0].InUse || Config.LoRaDevices[1].InUse)
		{
		if (pthread_create(&LoRaThread, NULL, LoRaLoop, &GPS))
			{
			fprintf(stderr, "Fehler beim Erstellen eines LoRa-Threads\n");
			}
		}
	
	if (pthread_create(&DS18B20Thread, NULL, DS18B20Loop, &GPS))
		{
		fprintf(stderr, "Fehler beim Erstellen des DS18B20s-Threads\n");
		return 1;
		}

	if (Config.Camera)
		{
		if (pthread_create(&CameraThread, NULL, CameraLoop, &GPS))
			{
			fprintf(stderr, "Fehler beim Erstellen des Kamera-Threads.\n");
			return 1;
			}
		}

	if (pthread_create(&LEDThread, NULL, LEDLoop, &GPS))
		{
		fprintf(stderr, "Fehler bei der Erstellung des LED-Threads.\n");
		return 1;
		}

	if (Config.TelemetryFileUpdate > 0)
		{
		if (pthread_create(&LogThread, NULL, LogLoop, &GPS))
			{
			fprintf(stderr, "Fehler beim Erstellen eines Log-Threads\n");
			return 1;
			}
		}
	
	while (1)
		{		
		if (fd < 0)
			{
			delay(200);
			}
		else if (LoRaUploadNow(&GPS, 10))
			{
			delay(200);
			}
		}
	
	gpioTerminate();
	}
