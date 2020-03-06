# PISKY_Pure - a very small HAB Tracker software for the GNSS/LoRa boards from ELcon #

Created by Roland Elmiger (hb9gaa@arrl.net). Some program parts have been taken over by Dave Ackerman.

This software is written for the GNSS/LoRa board with Pi Zero W board.
The GNSS/LoRa board can be purchased from board from http://https://shop.elcon.ch
Software support is provided for customers who have purchased a GNSS/LoRa board, for use with that board only.

## Installation ##
Follow the instructions at http://www.pi-in-the-sky.com/index.php?id=sd-card-image-from-scratch

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

