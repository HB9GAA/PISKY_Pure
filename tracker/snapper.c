#include "Configuration.h"

int SSDVPacketsToSend(int Channel)
		{
	int i, j, Count;
	
	Count = 0;
	for (i = 0; i < 2; i++)
		{
		for (j=0; j<Config.Channels[Channel].SSDVPackets[i].NumberOfPackets; j++)
			{
			if (Config.Channels[Channel].SSDVPackets[i].Packets[j])
				{
				Count++;
				}
			}
		}
	// printf("Channel %d Count %d\n", Channel, Count);
	return Count;
	}


int TimeTillImageCompleted(int Channel)
	{
	//Schneller Check für den "CAM0" oder "CAM1"-Kanal - dafür senden wir nicht, also müssen wir eine grossen Zeit-Wert zurücksenden, damit wir nicht sofort ein Foto machen
	if ((Channel == CAM0_CHANNEL) || (Channel == CAM1_CHANNEL))
		{
		return 9999;
		}
	
	return SSDVPacketsToSend(Channel) * 256 * 10 / Config.Channels[Channel].BaudRate;
	}


void FindBestImageAndRequestConversion(int Channel, int width, int height)
	{
	size_t LargestFileSize;
	char LargestFileName[100], FileName[100];
	DIR *dp;
	struct dirent *ep;
	struct stat st;
	char *SSDVFolder;
	
	LargestFileSize = 0;
	SSDVFolder = Config.Channels[Channel].SSDVFolder;
	
	dp = opendir(SSDVFolder);
	if (dp != NULL)
		{
		while ((ep = readdir (dp)) != NULL)
			{
			if (strstr(ep->d_name, ".JPG") != NULL)
				{
				if (strchr(ep->d_name, '~') == NULL)
					{
					sprintf(FileName, "%.60s/%.30s", SSDVFolder, ep->d_name);
					stat(FileName, &st);
					if (st.st_size > LargestFileSize)
						{
						LargestFileSize = st.st_size;
						strcpy(LargestFileName, FileName);
						}
					}
				}
			}
		(void) closedir (dp);
		}

	if (LargestFileSize > 0)
		{
		FILE *fp;
		
		printf("Found file %s to convert\n", LargestFileName);
		
		// Create SSDV script file
		if ((fp = fopen(Config.Channels[Channel].convert_file, "wt")) != NULL)
			{
			Config.Channels[Channel].SSDVFileNumber++;
			sprintf(Config.Channels[Channel].ssdv_filename, "ssdv_%d_%d.bin", Channel, Config.Channels[Channel].SSDVFileNumber);
			
			if ((Config.Camera == CAM0_CHANNEL) || (Config.Camera == CAM1_CHANNEL))
				{
				//Schreiben Sie einfach Parameter in eine Datei und überlassen Sie dem externen Skript den Rest
				fprintf(fp, "%s\n%.6s\n%d\n%s\n%d\n%d\n%s\n", Config.Channels[Channel].PayloadID, Config.SSDVSettings, Config.Channels[Channel].SSDVFileNumber, LargestFileName, width, height, Config.Channels[Channel].ssdv_filename);
				}
			else
				{
				//Externes Skript für ImageMagic etc.
				fprintf(fp, "rm -f ssdv.jpg\n");
				fprintf(fp, "if [ -e process_image ]\n");
				fprintf(fp, "then\n");
				fprintf(fp, "	./process_image %d %s %d %d\n", Channel, LargestFileName, width, height);
				fprintf(fp, "else\n");
				//Kopieren Sie einfach die Datei
				fprintf(fp, "	cp %s ssdv.jpg\n", LargestFileName);
				fprintf(fp, "fi\n");
				fprintf(fp, "ssdv %s -e -c %.6s -i %d %s %s\n", Config.SSDVSettings, Config.Channels[Channel].PayloadID, Config.Channels[Channel].SSDVFileNumber, "ssdv.jpg", Config.Channels[Channel].ssdv_filename);
				fprintf(fp, "mkdir -p %s/$1\n", SSDVFolder);
				fprintf(fp, "mv %s/*.JPG %s/$1\n", SSDVFolder, SSDVFolder);
				fprintf(fp, "echo DONE > %s\n", Config.Channels[Channel].ssdv_done);
				}
			fclose(fp);
			chmod(Config.Channels[Channel].convert_file, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH); 
			}
		}
	}


void GetWidthAndHeightForChannel(struct TGPS *GPS, int Channel, int *width, int *height)
	{
	if (GPS->Altitude >= Config.SSDVHigh)
		{
		*width = Config.Channels[Channel].ImageWidthWhenHigh;
		*height = Config.Channels[Channel].ImageHeightWhenHigh;
		}
	else
		{
		*width = Config.Channels[Channel].ImageWidthWhenLow;
		*height = Config.Channels[Channel].ImageHeightWhenLow;
		}
	
	//SSDV erfordert die Dimensionen eines Vielfachen von 16 Pixeln
	*width = (*width / 16) * 16;
	*height = (*height / 16) * 16;
	}


void *CameraLoop(void *some_void_ptr)
	{
	int width, height;
	struct TGPS *GPS;
	char filename[100];
	int Channel;
	FILE *fp;

	GPS = (struct TGPS *)some_void_ptr;
	
	for (Channel = 0; Channel < 5; Channel++)
		{
		Config.Channels[Channel].TimeSinceLastImage = Config.Channels[Channel].ImagePeriod;
		Config.Channels[Channel].SSDVFileNumber = 0;
		}

	while (1)
		{
		for (Channel = 0; Channel < 5; Channel++)
			{
			if (Config.Channels[Channel].Enabled && (Config.Channels[Channel].ImagePackets > 0))
				{
				//Kanal, der SSDV verwendet
				if (++Config.Channels[Channel].TimeSinceLastImage >= Config.Channels[Channel].ImagePeriod)
					{
					//Es ist Zeit, ein Bild für diesen Kanal zu machen
					Config.Channels[Channel].TimeSinceLastImage = 0;							//Zeit zurücksetzen
					GetWidthAndHeightForChannel(GPS, Channel, &width, &height);		//Bildgrösse lesen
					
					if ((width >= 0) && (height >= 0))
						{
						sprintf(filename, "/home/pi/PISKY_Pure/tracker/take_pic_%d", Channel); //Name der Datei erstellen
						
						//Falls das Bild bereits existiert, nicht antasten, dann ist das neue Bild noch nicht erstellt worden
						if (access(filename, F_OK ) == -1)
							{				
							//Das File existiert nicht, also erstellen. Das Skript wird es das nächste Mal ausführen.
							if ((fp = fopen(filename, "wt")) != NULL)
								{
								char FileName[256];
								
								
								if (Channel == CAM0_CHANNEL)
									{
									fprintf(fp, "gpio -g mode 17 out\n");					//GPIO17 -> Kamera-Umschaltung ist ein Ausgang
									
									for (int i = CAM0_CHANNEL; i <= CAM1_CHANNEL; i++)
										{
										Channel = i;
										
										if (i == CAM0_CHANNEL)
											fprintf(fp, "gpio -g write 17 0\n");					//GPIO17 -> Kamera 0 aktivieren
										else
											fprintf(fp, "gpio -g write 17 1\n");					//GPIO17 -> Kamera 0 aktivieren
										
										fprintf(fp, "mkdir -p %s/$2\n", Config.Channels[Channel].SSDVFolder);	//Kamara-Directory erstellen
										sprintf(FileName, "%s/$2/$1.JPG", Config.Channels[Channel].SSDVFolder);	//und Filename definieren

										if ((width == 0) || (height == 0))						//Bild aufnehmen und ev. die Bildgrösse definieren
											{
											fprintf(fp, "raspistill -st -t 2000 -ex auto -mm matrix %s -o %s\n", Config.CameraSettings, FileName);
											}
										else
											{
											fprintf(fp, "raspistill -st -w %d -h %d -t 2000 -ex auto -mm matrix %s -o %s\n", width, height, Config.CameraSettings, FileName);
											}			
										fprintf(fp, "exiv2 -M 'set  Exif.GPSInfo.GPSLatitude %i/100000 0/1 0/1' -M 'set Exif.GPSInfo.GPSLatitudeRef N' %s\n", (int)(100000*GPS->Latitude), FileName);
										fprintf(fp, "exiv2 -M 'set  Exif.GPSInfo.GPSLongitude %i/100000 0/1 0/1' -M 'set Exif.GPSInfo.GPSLongitudeRef E' %s\n", (int)(100000*GPS->Longitude), FileName);
										fprintf(fp, "exiv2 -M 'set  Exif.GPSInfo.GPSAltitude %d/1' %s\n", (int) (GPS->Altitude), FileName);
										fprintf(fp, "exiv2 -M 'set  Exif.Photo.UserComment AscentRate = %.1lf m/s' %s\n", GPS->AscentRate, FileName);
										}
									}
								else
									{
									fprintf(fp, "mkdir -p %s/$2\n", Config.Channels[Channel].SSDVFolder);	//Kamara-Directory erstellen
									sprintf(FileName, "%s/$1.JPG", Config.Channels[Channel].SSDVFolder);//und Filename definieren
									fprintf(fp, "raspistill -st -w %d -h %d -t 2000 -ex auto -mm matrix %s -o %s\n", width, height, Config.CameraSettings, FileName);
									fprintf(fp, "exiv2 -M 'set  Exif.GPSInfo.GPSLatitude %i/100000 0/1 0/1' -M 'set Exif.GPSInfo.GPSLatitudeRef N' %s\n", (int)(100000*GPS->Latitude), FileName);
									fprintf(fp, "exiv2 -M 'set  Exif.GPSInfo.GPSLongitude %i/100000 0/1 0/1' -M 'set Exif.GPSInfo.GPSLongitudeRef E' %s\n", (int)(100000*GPS->Longitude), FileName);
									fprintf(fp, "exiv2 -M 'set  Exif.GPSInfo.GPSAltitude %d/1' %s\n", (int) (GPS->Altitude), FileName);
									fprintf(fp, "exiv2 -M 'set  Exif.Photo.UserComment AscentRate = %.1lf m/s' %s\n", GPS->AscentRate, FileName);
									}

								fclose(fp);
								chmod(filename, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH); 
								Config.Channels[Channel].ImagesRequested++;
								}
							}
						}
					}
				
				// Kontrollieren, ob wir das "beste" Bild konvertieren müssen, bevor die aktuelle SSDV-Datei vollständig gesendet ist.
				if (Channel < CAM0_CHANNEL)
					{
					// Bilder in voller Grösse ausschliessen - bei diesen keine Konvertierung vornehmen.
					if (TimeTillImageCompleted(Channel) < 25)
						{
						// Benötigen konvertierte Datei bald
						if (!FileExists(Config.Channels[Channel].convert_file) && !FileExists(Config.Channels[Channel].ssdv_done))
							{
							// diese holen, falls das Skript die Auflösung grosser Bilder (z.B. von einer SLR- oder Kompaktkamera) ändern muss.
							GetWidthAndHeightForChannel(GPS, Channel, &width, &height);
							
							// Zu verwendendes Bild finden, dann die Konvertierung anfordern (extern durchgeführt)
							FindBestImageAndRequestConversion(Channel, width, height);
							}
						}
					}
				}
			}
		sleep(1);
		}
	}
