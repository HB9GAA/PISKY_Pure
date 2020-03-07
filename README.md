# PISKY_Pure - is a small HAB Tracker software <br> for the GNSS/LoRa boards from ELcon Consulting & Engineering #

Created by Roland Elmiger (<a href="mainlto:hb9gaa@arrl.net">hb9gaa@arrl.net</a>). Some program parts have been taken over by Dave Ackerman.

This software is written for the special GNSS/LoRa board together with a Pi Zero W. <br>
The GNSS/LoRa board can be purchased from <a href="http://https://shop.elcon.ch">ELcon-shop</a> <br>
Software support is provided for customers who have purchased a GNSS/LoRa board, for use with that board only.

## Installation ##
Follow the instructions:
- Download the latest version of <a href="https://www.raspberrypi.org/downloads/">Raspberry Pi Imagerand</a> install it. <br>
- Connect an SD card reader with the SD card inside. <br>
- Open Raspberry Pi Imager and choose the OS RASBIAN LITE from the list presented. <br>
- Choose the SD card you wish to write your image to. <br>
- Review your selections and click 'WRITE' to begin writing data to the SD card.<br><br>
- Open the "boot" directory on the SD Card, and create an empty file named "ssh".<br><br>
- In order to connect the Raspberry Pi to a WLAN router (hotspot), it has to know the SSID and the passphrase (password)<br>
 of your router. These WLAN log-in information can be entered by starting the configuration tool "RaspiBrickConfig.jar"<br>
 You must copy this tool into the "boot" directory before you start it.<br>
- In the dialog box enter your router SSID and the passphrase. Then click Create SSID and Save.
 
The SD-card is now ready to be used in your Raspberry Pi Zero W (incl. GNSS/LoRa card mounted).<br>
Eject the SD card from your computer and insert it into your Pi. <br>
Power up your Pi (we recommend an official power supply) and wait a few moments for the initial boot to complete.<br>
On your computer, open up PuTTY.

We know that the default hostname (the name of the machine) for a fresh Raspbian install is raspberrypi, 
so in the Host Name (or IP address) field enter raspberrypi. This searches for machines on that name on the local network. 
Once a connection is established, you'll be prompted by PuTTY to accept the connection to the new machine. You should see a black terminal prompting for a login. Enter the default username: pi and the default password: raspberry

If it's been a while since the image was downloaded, we can update the software on the Raspi again:

	sudo apt-get update
	sudo apt-get upgrade



## USB Camera ##
To use a USB camera (i.e. a compact or SLR) instead of the Pi camera, you need to install gphoto2 and imagemagick:

	sudo apt-get install gphoto2 imagemagick

and you need to edit /boot/pisky.txt and change the "camera" line to be:

	Camera=G

(G/g are for gphoto2, U/F/u/f for fswebcam; N/n is for no camera, Y/y/1/TC/c is for CSI camera).

The camera must be able to support remote capture as well as image download.
There's a list of cameras at http://www.gphoto.org/doc/remote/.

The image resolution is not controlled so you should set this up in the camera.  The full image files are stored on Pi SD card, so ensure that it has sufficient free capacity.

If you are transmitting images with SSDV, then these need to be resized before transmission.  This is configured in pisky.txt in the usual way. 
The process though is different - the camera takes the full sized image, and this image is downloaded to the Pi unaltered.  Before transmission the selected images are then resized using imagemagick.
So, regardless of the image size settings in pisky.txt, the Pi SD card will contain the full-size images.

PITS does not set the camera exposure.  Normally with the camera set to an automatic exposure mode you will get plenty of good images, but you can of course set it to any mode you wish including manual.
If you wish to control this from the software rather than on the camera, then create an executable script "take_image" (there is a sample supplied as "take_image.sample").
Note that most settings are only available in the more basic modes - e.g. the aperture can only be controlled in aperture-priority or manual modes.

gphoto2 needs more work than the other camera options, and we strongly recommend some experimentation with the camera you intend to use.  See http://gphoto.org/doc/manual/using-gphoto2.html.


## USB Webcam ##

To use a USB webcam instead of the Pi camera, you need to install fswebcam:

	sudo apt-get install fswebcam

and you need to edit /boot/pisky.txt and change the "camera" line to be:

	Camera=U


## IMAGE PROCESSING ##

All images now include telemetry (longitude, latitude, altitude) in the JPEG comment field, **but only if EXIV2 has been installed**.

It is therefore possible to overlay downloaded images with telemetry data, as text or graphics, using (for example) ImageMagick, and EXIV2 to extract the data from the JPEG being processed.  A sample script "process_image.sample" is provided; to use this, rename to be "process_image", make it executable, and edit to your requirements.  Please note that the sample assumes a particular image resolution and you will need to change the pixel positions of the various items.  Imagemagick is quite complex to use but there is plenty of documentation on the web plus many samples. 


## Change Log ##

05/03/2020
==========

- code completely re-written to use GNSS/LoRa board from ELcon, Switzerland (i.e. not using PITS board)

