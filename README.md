# PISKY_Pure - is a small HAB Tracker software for the <br>GNSS/LoRa, dual Camera-Multiplexer and Relay boards from ELcon Consulting & Engineering #

Created by Roland Elmiger (<a href="mainlto:hb9gaa@arrl.net">hb9gaa@arrl.net</a>). 
Some code sequences were taken from the "Pi in the Sky" by Dave Ackerman.

This software is specially written for the GNSS/LoRa board and Raspberry Pi Zero W. <br>
The GNSS/LoRa board EL-50 contains an ublox GNSS receiver and the LoRa transceiver for the 70cm band.<br>
The EL-51 Relay Board is now integrated in the software. The four optically isolated solid-state relays can be switched on and off height-controlled.<br>
The EL-52 dual camera multiplexer board is supported by the software. In the configuration it can be defined whether one or two cameras are installed.<br>
In addition, the current GNSS data, longitude, latitude and altitude are stored in the Exchangeable Image File in all acquired images.<br>
<g-emoji class="g-emoji" alias="exclamation" fallback-src="https://github.githubassets.com/images/icons/emoji/unicode/2757.png">❗️</g-emoji> ***The software does not work for all other boards!***

## OS installation ##
Follow the instructions:
- Download the latest version of <a href="https://www.raspberrypi.org/downloads/">Raspberry Pi Imager</a> and install it. <br>
- Connect an SD card reader with the SD card inside. <br>
- Open Raspberry Pi Imager and choose the OS RASBIAN LITE from the list presented. <br>
- Choose the SD card you wish to write your image to. <br>
- Review your selections and click 'WRITE' to begin writing data to the SD card.<br><br>
- Open the "boot" directory on the SD Card, and create an empty file named "ssh".<br><br>
- In order to connect the Raspberry Pi to a WLAN router (hotspot), it has to know the SSID and the passphrase (password)<br>
 of your router. These WLAN log-in information can be entered by starting the configuration tool <a href="https://github.com/HB9GAA/PISKY_Pure/raw/master/misc/RaspiBrickConfig.jar">RaspiBrickConfig.jar</a><br>
 You must copy this tool into the "boot" directory before you start it.<br>
 In the dialog box enter your router SSID and the passphrase. Then click Create SSID and Save.<br>
<img src="/misc/raspibrick10.png" align="middle"><br>
 
The SD-card is now ready to be used in your Raspberry Pi Zero W (incl. GNSS/LoRa card mounted).<br>
Eject the SD card from your computer and insert it into your Pi. <br>
Power up your Pi (we recommend an official power supply) and wait a few moments for the initial boot to complete.<br>
On your computer, open up <a href="https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html">PuTTY</a>.

We know that the default hostname (the name of the machine) for a fresh Raspbian install is <b>raspberrypi</b>, 
so in the Host Name (or IP address) field enter raspberrypi. This searches for machines on that name on the local network. 
Once a connection is established, you'll be prompted by PuTTY to accept the connection to the new machine. You should see a black terminal prompting for a login. Enter the default username: <b>pi</b> and the default password: <b>raspberry</b>

If it's been a while since the image was downloaded, we can update the software on the Raspi again:

	sudo apt-get update
	sudo apt-get upgrade -y

To prepare the main user software we still have to install the following libraries.

	sudo apt-get install git -y
	sudo apt-get install python3-distutils -y


## PISKY_Pure installation ##
Use the following command to load the PISKY_Pure software onto your Raspberry Pi Zero W.<br>

	git clone https://github.com/HB9GAA/PISKY_Pure.git
	cd ~/PISKY_Pure
	./install

Follow the information that will guide you through the various installations.
At the end of the installation you should not forget to fill in your callsign in the file pisky.txt.<br>
Finally, the Raspberry Pi Zero W must be restarted with the following command

	sudo reboot


