# Quadra-VST-editor---Cherry-Audio
A Hardware editor for the Cherry Audio Quadra VST

This is my attempt to make a physical hardware editor for the Cherry Audio Arp Quadra VST. 

![Synth](photos/synth.jpg)

First of all let me say I started building this without really understanding how the Quadra was controlled over MIDI, my knowledge was based on my experience with Arturia and building an editor for their CS80 VST.

So not long after construction started I did some testing and realized that the Cherry Audio VSTs uses toggling of buttons based on sending the same CC message or Note on/off etc. So this gives me a problem straight away in that I cannot change a specific button to on or off without knowing it's current status. I have asked Cherry Audio to implement on/off based on cc value 127 and 0 to make things easier. Let's see if it's implemented in a later release. Also there are a few buttons that toggle through multiple values, so these are impossible to control as you don't know where they are at any given time. And finally there seems to be a bug in the selection of the arp direction when controlling over MIDI.

There is a solution to all of this, the MIDI controller can act as the master controller and update every parameter of the Quadra VST over MIDI, by creating a blank patch with known positions of all the buttons/sliders/trill etc you can over ride these every time you recall this blank template, for example the VCO1 wave is set to SAWTOOTH UP in the blank patch, so I know how many times I need to increment the selection to change it to SINE, same for VCO2 and the POLYWAVE. Trill is always set to +1, note ranges are not important as these can easily be over ridden with a few simple MIDI messages. Only things I cannot display are the flashing tempo LEDs for the LFO, sync etc, but these are compensated for by the onboard touch screen which shows the true positions of all the controls.

So the project is Teensy 4.1 based with 5*16 channel mux chips used to read pots and slide switches and 9 shift register pairs used for push buttons and 10 for the LEDs. I have also used a MAX7219 to control the slider LEDs, but the project could be built without LED sliders if desired. LED note range displays are controlled by TM1637 6 digit display controllers.

I have included some schematics and it can hold 999 programs.


