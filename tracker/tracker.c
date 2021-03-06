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

#include "Configuration.h"

struct TConfig Config;

// Pin allocations.  Do not change unless you're using your own hardware
// WIRING PI PINS
#define UBLOX_ENABLE	2

int Records, FileNumber;
struct termios options;
char *SSDVFolder="/home/pi/PISKY_Pure/tracker/images";
 

void LoadConfigFile(struct TConfig *Config)
	{
	const char* CameraTypes[6] = {"None", "one CSI Pi Camera - raspistill", "two CSI Pi Camera - raspistill", "USB webcam - fswebcam", "USB camera - gphoto2", "Python Script"};
	FILE *fp;
	char *filename = "/boot/pisky.txt";

	if ((fp = fopen(filename, "r")) == NULL)
		{
		printf("\nFailed to open config file %s (error %d - %s).\nPlease check that it exists and has read permission.\n", filename, errno, strerror(errno));
		exit(1);
		}

	ReadBoolean(fp, "UNIXformat", -1, 0, &(Config->UNIXformat));			//UNIXformat = 0 -> hh:mm:ss or UNIXformat = 1 -> UNIX-Time 
	if (Config->UNIXformat)
			{
			printf("Format: UNIX-Time\n");
			}
		else
			{
			printf("Format: Default\n");
			}

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
	
	Config->EnableRelaisLogging = ReadBooleanFromString(fp, "logging", "Relais");
	if (Config->EnableRelaisLogging) printf("Relais Logging enabled\n");
	
	Config->RelaisUpdateRate = ReadInteger(fp, "relais_update_rate", -1, 0, 120);
	if (Config->RelaisUpdateRate > 0)
		{
		printf("Relais Status are stored every %ds in relais.txt (default 120s)\n", Config->RelaisUpdateRate);
		}
	
	for (int Rel = 0; Rel < 4; Rel++)
		{
		printf("Relais %d settings\n", Rel);
		Config->RelaisConfig[Rel].AscendON = ReadInteger(fp, "ASC_ON_Relais", Rel, 0, -1);
		Config->RelaisConfig[Rel].AscendOFF = ReadInteger(fp, "ASC_OFF_Relais", Rel, 0, -1);
		printf("      - ASC_ON = %5d m,\tASC_OFF = %5d m\n", Config->RelaisConfig[Rel].AscendON, Config->RelaisConfig[Rel].AscendOFF);
		Config->RelaisConfig[Rel].DescendON = ReadInteger(fp, "DES_ON_Relais", Rel, 0, -1);
		Config->RelaisConfig[Rel].DescendOFF = ReadInteger(fp, "DES_OFF_Relais", Rel, 0, -1);
		printf("      - DES_ON = %5d m,\tDES_OFF = %5d m\n", Config->RelaisConfig[Rel].DescendON, Config->RelaisConfig[Rel].DescendOFF);
		}
	
	
	Config->ExternalDS18B20 = ReadInteger(fp, "external_temperature", -1, 0, 1);
	if (Config->ExternalDS18B20)
		{
		printf("External DS18B20 Enabled\n");
		}
	
	Config->Camera = ReadCameraType(fp, "camera");			//0 = no camera, 1 = one camera, 2 = two cameras (requires the dual camera multiplexer board)
	printf ("Camera (%s) %s\n", CameraTypes[Config->Camera], Config->Camera ? "Enabled" : "no Camera");
	
	if (Config->Camera == 1 || Config->Camera == 2 )
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
	
		// Set up full-size image parameters for CAM0 and CAM1 are the same		
		Config->Channels[CAM0_CHANNEL].ImageWidthWhenLow = ReadInteger(fp, "full_low_width", -1, 0, 640);
		Config->Channels[CAM0_CHANNEL].ImageHeightWhenLow = ReadInteger(fp, "full_low_height", -1, 0, 480);
		Config->Channels[CAM1_CHANNEL].ImageWidthWhenLow = ReadInteger(fp, "full_low_width", -1, 0, 640);
		Config->Channels[CAM1_CHANNEL].ImageHeightWhenLow = ReadInteger(fp, "full_low_height", -1, 0, 480);
		printf ("Full Low image size %d x %d pixels\n", Config->Channels[CAM0_CHANNEL].ImageWidthWhenLow, Config->Channels[CAM0_CHANNEL].ImageHeightWhenLow);
		
		Config->Channels[CAM0_CHANNEL].ImageWidthWhenHigh = ReadInteger(fp, "full_high_width", -1, 0, 2592);
		Config->Channels[CAM0_CHANNEL].ImageHeightWhenHigh = ReadInteger(fp, "full_high_height", -1, 0, 1944);
		Config->Channels[CAM1_CHANNEL].ImageWidthWhenHigh = ReadInteger(fp, "full_high_width", -1, 0, 2592);
		Config->Channels[CAM1_CHANNEL].ImageHeightWhenHigh = ReadInteger(fp, "full_high_height", -1, 0, 1944);
		printf ("Full High image size %d x %d pixels\n", Config->Channels[CAM0_CHANNEL].ImageWidthWhenHigh, Config->Channels[CAM0_CHANNEL].ImageHeightWhenHigh);

		Config->Channels[CAM0_CHANNEL].ImagePeriod = ReadInteger(fp, "full_image_period", -1, 0, 60);
		Config->Channels[CAM1_CHANNEL].ImagePeriod = ReadInteger(fp, "full_image_period", -1, 0, 60);
		printf ("Full size: %d seconds between photographs\n", Config->Channels[CAM0_CHANNEL].ImagePeriod);
		
		Config->Channels[CAM0_CHANNEL].ImagePackets = Config->Channels[CAM0_CHANNEL].ImagePeriod > 0;
		Config->Channels[CAM0_CHANNEL].Enabled = Config->Channels[CAM0_CHANNEL].ImagePackets;
		Config->Channels[CAM1_CHANNEL].ImagePackets = Config->Channels[CAM1_CHANNEL].ImagePeriod > 0;
		Config->Channels[CAM1_CHANNEL].Enabled = Config->Channels[CAM1_CHANNEL].ImagePackets;
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
	if (GPS->Satellites > 0) 									// && (GPS->Altitude > Config.SSDVHigh))
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
	pthread_t LoRaThread, GPSThread, RelaisThread, DS18B20Thread, CameraThread, LEDThread;
	
	if (prog_count("tracker") > 1)
		{
		printf("\nThe tracker program is already running!\n");
		printf("It is automatically started with the camera script when the Pi boots.\n\n");
		printf("If you just want the tracker software to run, it already is,\n");
		printf("and its output can be viewed on a monitor attached to a Pi video socket.\n\n");
		printf("If you want to view the tracker output via ssh,\n");
		printf("then stop the program with the command:\n");
		printf("	sudo killall tracker\n\n");
		printf("and start it manually with\n");
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
			printf("RPi Zero W or RPi Zero\n");
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
		printf("Remove existing image files\n");
		remove("gps.txt");
		remove("telemetry.txt");
		remove("relais.txt");
		remove("/boot/clear.txt");
		system("rm -rf /home/pi/PISKY_Pure/tracker/images/*");
		}
		
	// Remove any old SSDV files
	system("rm -f ssdv*.bin");

	memset(&GPS, 0, sizeof(struct TGPS));
	
	// Set up I/O
	if (wiringPiSetup() == -1)
		{
		printf("Cannot initialize WiringPi\n");
		exit (1);
		}
	
	if (gpioInitialise() < 0)
		{
		printf("Pigpio initialization failed.\n");
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
	sprintf(Config.Channels[4].SSDVFolder, "%s/CAM0", SSDVFolder);
	sprintf(Config.Channels[5].SSDVFolder, "%s/CAM1", SSDVFolder);
		
	if (Config.Camera)
		{
		// Create SSDV Folders
		if(stat(SSDVFolder, &st) == -1)
			{
			mkdir(SSDVFolder, 0777);
			}	
	
		for(i = 0; i < 6; i++)
			{
			if(*Config.Channels[i].SSDVFolder)
				{
				if (stat(Config.Channels[i].SSDVFolder, &st) == -1)
					{
					mkdir(Config.Channels[i].SSDVFolder, 0777);
					}
				}
			}

		// Filenames for SSDV
		for(i = 0; i < 6; i++)
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
		fprintf(stderr, "Error when creating a GPS thread\n");
		return 1;
		}
	
	if (Config.LoRaDevices[0].InUse || Config.LoRaDevices[1].InUse)
		{
		if (pthread_create(&LoRaThread, NULL, LoRaLoop, &GPS))
			{
			fprintf(stderr, "Error when creating a LoRa thread\n");
			}
		}
	
	if (pthread_create(&RelaisThread, NULL, RelaisLoop, &GPS))
		{
		fprintf(stderr, "Error while creating the relay thread\n");
		return 1;
		}
		
	if (pthread_create(&DS18B20Thread, NULL, DS18B20Loop, &GPS))
		{
		fprintf(stderr, "Error creating the DS18B20s thread\n");
		return 1;
		}

	if (Config.Camera)
		{
		if (pthread_create(&CameraThread, NULL, CameraLoop, &GPS))
			{
			fprintf(stderr, "Error while creating the camera thread.\n");
			return 1;
			}
		}

	if (pthread_create(&LEDThread, NULL, LEDLoop, &GPS))
		{
		fprintf(stderr, "Error when creating the LED thread.\n");
		return 1;
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
