This is a project to implement an USB HID keyboard with 
an Atmel ATTiny2313 microcontroller.

I have also managed to run the code without changes (other than the Makefile)
on an ATMega328P. It should be fairly easy to adapt it to other uCs as well

---- GOAL ---------------------------------------------------------------------
The goal behind the project was to create a custom game panel for flight 
simulators and similar games where you need a lot of buttons which in turn are 
mapped to keyboard keys.

The device is recognized as USB keyboard by windows (I can't test on Mac or 
Linux but I don't see why it wouldn't work) and doesn't need any drivers.

---- ACKNOWLEDGEMENTS ---------------------------------------------------------
The code is based on the USB HID keyboard tutorial by Joonas Pihlajamaa, 
published at the Code and Life blog, http://codeandlife.com

The usbdrv subfolder contain parts of V-USB library available at 
http://www.obdev.at/avrusb/ and its contents are copyrighted by their 
respective authors. 

Without their combined work, this project would probably not have taken 
flight (excuse the pun). 

---- CONFIGURATION ------------------------------------------------------------
For a suggestion on a circuit layout, see the gamekeys.png or the .sch file in 
the eagle subdirectory. Also, please read the "A word on the eagle file" 
section further below.

See the gamekeys.h file for the pin configuration for the shift registers.

In gamekeys.h you will also find the array 'buttonCodes' which holds the
mapping of buttons to USB keycodes. 

The buttonCodes array ordered such that buttonCodes[0] will be mapped to the
bit that is shifted in first. 

For a list of available USB keycodes, see usbkeycodes.h. Note that the list
is not 100% complete. But since I'm remapping keys in my simulators anyway,
I haven't found it lacking.

In case you want / have to establish USB connectivity on ports other than 
PD2 and PD3, you will have to change the values for USB_CFG_IOPORTNAME,
USB_CFG_DMINUS_BIT and USB_CFG_DPLUS_BIT in usbconfig.h inside the usbdrv 
subdirectory.

Make sure you change the values in the Makefile for the uC in use, your 
programmer and the frequency that your uC runs at. Make sure you double 
check with the VUSB website at http://www.obdev.at/avrusb/ which clock rates 
are supported (12.000MHz and 16.000MHz are fine, but there are others).

---- A WORD ON THE EAGLE FILE -------------------------------------------------
The eagle file was made for clarification purposes. I did NOT build my own
circuit from the eagle file. Instead, the eagle file was done after I had 
already made my device and contains a few improvements over my own design
such as the EXTEND pinheader and others.

THIS ESSENTIALLY MEANS THAT THE EAGLE FILE IS UNTESTED

I did my best to verify the schematic and I'm fairly confident that the 
circuitry itself and the values for all parts are correct. But the fact remains
that the schematic hasn't been built as-is. 

Make sure you do sanity checks on everything during your work.

Also, if you find issues with the schematic, don't hesitate to contact me
and I'll update the schematic.

---- DESIGN DECISIONS ---------------------------------------------------------
Hardware Debouncing
Instead of debouncing the buttons in software, I opted for hardware
debouncing based on the SN74HCT14N inverting Schmitt-Trigger ICs and a RC
low-pass filter for each button. The required components are available 
cheaply and perform very reliably. Note that you might want to opt for a 
less drastic filtering if you expect rather short key presses.

If you're running the code on an ATMega238P, a ATTiny4313 or another chip with
more than 2kb flash memory, you can easily implement software debouncing and 
safe yourself a lot of soldering.

Shift Registers
I wanted the software to be flexible and the hardware expandable. Because of 
that I'm reading the button states through 74HC165 shift registers. You can 
chain any number of shift registers in series to extend the number of buttons 
that your device supports.

No spamming of the USB port
A bit of logic was implemented to keep the number of USB messages down to
a minimum. The device will send an USB message only when a button is pressed
or released, not when buttons are held down.

