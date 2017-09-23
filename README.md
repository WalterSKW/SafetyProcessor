# SafetyProcessor
A safety processor for a 3D printer. 
Goal is to early detect fire risks by detecting abnormal high environmental temperatures and high levels of CO2.
The project should be easily accessible for most people and is therefor based on an Arduino platform and components from either the Arduino world or the 3D printing world.
Initial setup uses an Arduino Nano, a MQ 135 based smoke detector and as high temperature detector an standard 3D printing 100k NTC.

The setup allows 2 detection units to be connected. 2 because most printers have one sort of electronics box containing high power switching equipment for a heated BED and for the extruders and may have also an enclosed printing chamber (to keep te print protected from draft and eventually also heated to prevent warping)
One unit contains one MQ-135 gas detector, one 100k NTC and one LED for signalling.

The processor reads the values from the detectors installed and switches of the power electronics using a relay in the case of abnormal values.
Another function is that the processor can be used to switch the printing unit on and off using a pushbutton switch.
