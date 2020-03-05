// Types

typedef enum {fmIdle, fmLaunched, fmDescending, fmHoming, fmDirect, fmDownwind, fmUpwind, fmLanding, fmLanded} TFlightMode;

struct TGPS
	{
	// GPS
	long SecondsInDay;					// Time in seconds since midnight
	int Hours, Minutes, Seconds;
	double Longitude, Latitude;
	int32_t Altitude;
	unsigned int Satellites;
	int Speed;
	int Direction;
	unsigned long SecondsSinceLaunch;					// Time in seconds since midnight
	
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


// functions
void *GPSLoop(void *some_void_ptr);


