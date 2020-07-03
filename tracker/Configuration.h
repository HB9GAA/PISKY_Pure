#include <stdio.h> 
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <termios.h>
#include <stdint.h>
#include <dirent.h>
#include <math.h>
#include <pthread.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <sys/statvfs.h>
#include <stdbool.h>
#include <pigpio.h> 
#include <inttypes.h>
#include <linux/i2c-dev.h>
#include <stdarg.h>
#include <time.h>


//LoRa Definitions
//-----------------------------------------------------------------------------
#define REG_FIFO                    0x00
#define REG_BITRATE_MSB    					0x02
#define REG_BITRATE_LSB    					0x03
#define REG_FDEV_MSB                0x04
#define REG_FDEV_LSB                0x05
#define REG_FRF_MSB                 0x06
#define REG_FRF_MID                 0x07
#define REG_FRF_LSB                 0x08
#define REG_FIFO_ADDR_PTR           0x0D 
#define REG_FIFO_TX_BASE_AD         0x0E
#define REG_FIFO_RX_BASE_AD         0x0F
#define REG_RX_NB_BYTES             0x13
#define REG_OPMODE                  0x01
#define REG_FIFO_RX_CURRENT_ADDR    0x10
#define REG_IRQ_FLAGS_MASK          0x11
#define REG_IRQ_FLAGS               0x12
#define REG_IRQ_FLAGS1              0x3E
#define REG_IRQ_FLAGS2              0x3F
#define REG_PACKET_SNR							0x19
#define REG_PACKET_RSSI							0x1A
#define REG_CURRENT_RSSI						0x1B
#define REG_DIO_MAPPING_1           0x40
#define REG_DIO_MAPPING_2           0x41
#define REG_MODEM_CONFIG            0x1D
#define REG_MODEM_CONFIG2           0x1E
#define REG_MODEM_CONFIG3           0x26
#define REG_PAYLOAD_LENGTH          0x22
#define REG_HOP_PERIOD              0x24
#define REG_FREQ_ERROR							0x28
#define REG_DETECT_OPT							0x31
#define	REG_DETECTION_THRESHOLD			0x37
#define REG_PREAMBLE_MSB_FSK 				0x25
#define REG_PREAMBLE_LSB_FSK 				0x26
#define REG_SYNC_CONFIG             0x27
#define REG_PACKET_CONFIG1	  			0x30
#define REG_PAYLOAD_LENGTH_FSK      0x32
#define REG_FIFO_THRESH             0x35

// MODES
#define RF98_MODE_RX_CONTINUOUS     0x85
#define RF98_MODE_TX                0x83
#define RF98_MODE_SLEEP             0x80
#define RF98_MODE_STANDBY           0x81

#define PAYLOAD_LENGTH              255

// Modem Config 1
#define EXPLICIT_MODE               0x00
#define IMPLICIT_MODE               0x01

#define ERROR_CODING_4_5            0x02
#define ERROR_CODING_4_6            0x04
#define ERROR_CODING_4_7            0x06
#define ERROR_CODING_4_8            0x08

#define BANDWIDTH_7K8               0x00
#define BANDWIDTH_10K4              0x10
#define BANDWIDTH_15K6              0x20
#define BANDWIDTH_20K8              0x30
#define BANDWIDTH_31K25             0x40
#define BANDWIDTH_41K7              0x50
#define BANDWIDTH_62K5              0x60
#define BANDWIDTH_125K              0x70
#define BANDWIDTH_250K              0x80
#define BANDWIDTH_500K              0x90

// Modem Config 2

#define SPREADING_6                 0x60
#define SPREADING_7                 0x70
#define SPREADING_8                 0x80
#define SPREADING_9                 0x90
#define SPREADING_10                0xA0
#define SPREADING_11                0xB0
#define SPREADING_12                0xC0

#define CRC_OFF                     0x00
#define CRC_ON                      0x04

// POWER AMPLIFIER CONFIG
#define REG_PA_CONFIG               0x09
#define PA_MAX_BOOST                0x8F
#define PA_LOW_BOOST                0x81
#define PA_MED_BOOST                0x8A
#define PA_MAX_UK                   0x88
#define PA_OFF_BOOST                0x00
#define RFO_MIN                     0x00

// LOW NOISE AMPLIFIER
#define REG_LNA                     0x0C
#define LNA_MAX_GAIN                0x23  // 0010 0011
#define LNA_OFF_GAIN                0x00
#define LNA_LOW_GAIN                0xC0  // 1100 0000

//Pin allocations.  Do not change unless you're using your own hardware
#define UBLOX_ENABLE	2


extern struct TConfig Config;
extern struct TGPS *GPS;

extern void LoadLoRaConfig(FILE *fp, struct TConfig *Config);

//
//-----------------------------------------------------------------------------
extern void *LoRaLoop(void *some_void_ptr);

//
//-----------------------------------------------------------------------------
extern void *RelaisLoop(void *some_void_ptr);

struct TRelaisConfig
	{
	int		AscendON;
	int		AscendOFF;
	int		DescendON;
	int		DescendOFF;
	int		RelaisPin;	
	};
	
extern uint8_t Relais[];			//Initial pin numbers of relays 0 to 3, see relais.c
	
//
//-----------------------------------------------------------------------------
extern void *LEDLoop(void *some_void_ptr);

//
//-----------------------------------------------------------------------------
extern void *CameraLoop(void *some_void_ptr);

//
//-----------------------------------------------------------------------------
extern void *DS18B20Loop(void *some_void_ptr);

//
//-----------------------------------------------------------------------------
extern void *GPSLoop(void *some_void_ptr);

//
//-----------------------------------------------------------------------------
#define	MAX_SSDV_PACKETS	4096

typedef enum {lmIdle, lmListening, lmSending} tLoRaMode;
typedef enum {ptNormal, ptCallingMode, ptBalloonRepeat, ptUplinkRepeat} tPacketType;

struct TLoRaDevice
	{
	int InUse;
	int DIO0;
	int DIO5;
	int CS;
	int RST;
	char Frequency[8];
  float PPM;
	int SpeedMode;
	int Power;
	int PayloadLength;
	int ImplicitOrExplicit;
	int ErrorCoding;
	int Bandwidth;
	int SpreadingFactor;
	int LowDataRateOptimize;
	int CycleTime;
	int Slot;
	int RepeatSlot;
	int UplinkSlot;
	int Binary;
	int HABPack;
	int LastTxAt;
	int LastRxAt;
	int AirCount;
	int GroundCount;
	int BadCRCCount;
	char LastCommand[256];
	unsigned char PacketToRepeat[256];
	unsigned char UplinkPacket[256];
	int PacketRepeatLength;
	int UplinkRepeatLength;
	tPacketType SendPacketType;
	tLoRaMode LoRaMode;
	char CallingFrequency[8];
	int CallingCount;
  int CallingSlot;
	int PacketsSinceLastCall;
	int ReturnStateAfterCall;
	
	// For placing a pause between packets (e.g. to allow another payload to repeat our packets)
	int PacketEveryMilliSeconds;
	int MillisSinceLastPacket;

	// Uplink cycle
	int UplinkPeriod;
	int UplinkCycle;
	
	// Uplink settings
	int UplinkMode;
	double UplinkFrequency;

	// Uplink Messaging
	int EnableMessageStatus;
	int EnableRSSIStatus;
	int LastMessageNumber;
	int MessageCount;
	int LastPacketRSSI;
	int LastPacketSNR;
	int PacketCount;
	int ListenOnly;					// True for listen-only payload that waits for an uplink before responding (or times out and sends anyway)
	};

struct TSSDVPackets
	{
	int ImageNumber;
	int NumberOfPackets;
	int InUse;
	unsigned char Packets[MAX_SSDV_PACKETS];
	};

struct TRecentPacket
	{
	int ImageNumber;
	int PacketNumber;
	};

// Structure for all possible radio devices
// 0 is RTTY
// 1 is APRS
// 2 and 3 are for LoRa
// 4 is a pretend channel for full-size images CAM0
// 5 is a pretend channel for full-size images CAM1
struct TChannel
	{
	int Enabled;
	unsigned int SentenceCounter;
	char PayloadID[16];
	int SendTelemetry;						//TRUE to send telemetry on this channel
	char SSDVFolder[200];
	int ImagePackets;							//Image packets per telemetry packet
	
	int ImageWidthWhenLow;
	int ImageHeightWhenLow;
	
	int ImageWidthWhenHigh;
	int ImageHeightWhenHigh;
	
	int ImagePeriod;							//Time in seconds between photographs
	int	TimeSinceLastImage;
	unsigned int BaudRate;
	char take_pic[100];
	char convert_file[100];
	char ssdv_done[100];
	char ssdv_filename[100];
	FILE *ImageFP;
	int ImagesRequested;
	
	// SSDV Variables
	int SSDVImageNumber;					//Image number for last Tx
	int SSDVPacketNumber;					//Packet number for last Tx
	int SSDVNumberOfPackets;			//Number of packets in image currently being sent
	int SSDVFileNumber;						//Number of latest converted image
	
	int SendMode;
	
	// SSDV Packet Log
	struct TSSDVPackets SSDVPackets[3];
	};


#define LORA_CHANNEL 2					//2 for LoRa CE0
#define CAM0_CHANNEL 4					//4 for FULL CAM0
#define CAM1_CHANNEL 5					//5 for FULL CAM1

struct TConfig
	{
	// Misc settings
	int DisableMonitor;
	int InfoMessageCount;
	int BoardType;
	int i2cChannel;
	
	// Camera
	int Camera;	
	int SSDVHigh;
	char CameraSettings[80];
	char SSDVSettings[16];
	
	// Extra devices
	int ExternalDS18B20;
	
	// Logging
	int EnableGPSLogging;
	int EnableTelemetryLogging;
	int EnableRelaisLogging;
	int RelaisUpdateRate;					//Period in seconds
																//added by HB9GAA
	int UNIXformat;								//UNIXformat = 0 -> hh:mm:ss or UNIXformat = 1 -> UNIX-Time 
	
	// LEDs
	int LED_OK;
	int LED_Warn;
	
	// GPS Settings
	int ShowGPS;
	int SDA;
	int SCL;
	char GPSDevice[64];
		
	// LoRa Settings
	struct TLoRaDevice LoRaDevices[2];

	// Radio channels
	struct TChannel Channels[6];	//2/3 are LoRa, 4 is full-size images CAM0, 5 is full-size images CAM1
	
	// GPS
	char GPSSource[128];
	int Power_Saving;
	int Flight_Mode_Altitude;
		
	// External data file (read into telemetry)
	char ExternalDataFileName[100];
	
	// Relais control
	struct TRelaisConfig RelaisConfig[4];

	};


extern char Hex(unsigned char Character);
extern void WriteGPSLog(char *Buffer);
extern void WriteTelemetryLog(char *Buffer);
extern void WritePredictionLog(char *Buffer);
extern short open_i2c(int address);
extern int FileExists(char *filename);
extern int ReadBooleanFromString(FILE *fp, char *keyword, char *searchword);
extern int ReadBoolean(FILE *fp, char *keyword, int Channel, int NeedValue, int *Result);
extern void ReadString(FILE *fp, char *keyword, int Channel, char *Result, int Length, int NeedValue);
extern int ReadCameraType(FILE *fp, char *keyword);
extern int ReadInteger(FILE *fp, char *keyword, int Channel, int NeedValue, int DefaultValue);
extern char ReadCharacter(FILE *fp, char *keyword);
extern double ReadFloat(FILE *fp, char *keyword, int Channel, int NeedValue, double DefaultValue);
extern void AppendCRC(char *Temp);
extern void LogMessage(const char *format, ...);
extern int devicetree(void);
extern void ProcessSMSUplinkMessage(int LoRaChannel, unsigned char *Message);
extern void ProcessSSDVUplinkMessage(int Channel, unsigned char *Message);
extern void AddImagePacketToRecentList(int Channel, int ImageNumber, int PacketNumber);
extern int ChooseImagePacketToSend(int Channel);
extern void StartNewFileIfNeeded(int Channel);
extern int prog_count(char* name);
extern int GetBoardType(int *i2cChannel);
extern int NoMoreSSDVPacketsToSend(int Channel);
extern int BuildSentence(unsigned char *TxLine, int Channel, struct TGPS *GPS);
extern int FixDirection180(int Angle);

//
//-----------------------------------------------------------------------------
typedef enum {fmIdle, fmLaunched, fmDescending, fmHoming, fmDirect, fmDownwind, fmUpwind, fmLanding, fmLanded} TFlightMode;

struct TGPS
	{
	// GPS
	long SecondsInDay;												//Time in seconds since midnight
	int Hours, Minutes, Seconds;
	double Longitude, Latitude;
	int32_t Altitude;
	unsigned int Satellites;
	int Speed;
	int Direction;
	unsigned long SecondsSinceLaunch;					//Time in seconds since midnight
	
	// Calculated from GPS
	int32_t MaximumAltitude, MinimumAltitude;
  double BurstLatitude, BurstLongitude;
	float AscentRate;
	
	// Sensors
	float DS18B20Temperature[2];
	float BatteryVoltage;
	float Humidity;
	float Pressure;
	float BoardCurrent;
	int DS18B20Count;

	// Flight control
	TFlightMode FlightMode;
		
	unsigned int MessageCount;
	};



