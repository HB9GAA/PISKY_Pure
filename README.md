# PISKY_Pure - is a small HAB Tracker software for the <br>GNSS/LoRa boards from ELcon Consulting & Engineering #

Created by Roland Elmiger (<a href="mainlto:hb9gaa@arrl.net">hb9gaa@arrl.net</a>). 
Some code sequences were taken from the "Pi in the Sky" by Dave Ackerman.

This software is specially written for the GNSS/LoRa board and Raspberry Pi Zero W. <br>
The GNSS/LoRa board can be purchased from <a href="http://https://shop.elcon.ch">ELcon-shop</a> <br>

![#f03c15](https://placehold.it/15/f03c15/000000?text=+) ***The software does not work for all other boards!***

<g-emoji class="g-emoji" alias="blush" fallback-src="https://github.githubassets.com/images/icons/emoji/unicode/1f60a.png">😊</g-emoji>

## OS installation ##
Follow the instructions:
- Download the latest version of <a href="https://www.raspberrypi.org/downloads/">Raspberry Pi Imagerand</a> install it. <br>
- Connect an SD card reader with the SD card inside. <br>
- Open Raspberry Pi Imager and choose the OS RASBIAN LITE from the list presented. <br>
- Choose the SD card you wish to write your image to. <br>
- Review your selections and click 'WRITE' to begin writing data to the SD card.<br><br>
- Open the "boot" directory on the SD Card, and create an empty file named "ssh".<br><br>
- In order to connect the Raspberry Pi to a WLAN router (hotspot), it has to know the SSID and the passphrase (password)<br>
 of your router. These WLAN log-in information can be entered by starting the configuration tool <a href="http://elcon.ch/download/RaspberryPi/RaspiBrickConfig.jar">RaspiBrickConfig.jar</a><br>
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


