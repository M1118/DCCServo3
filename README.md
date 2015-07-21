# DCCServo3
A 3 Servo DCC decoder based on an Arduino.

This decoder can be used to control 3 servos, the choice of servo
is determined by the state of the function buttons, F0, F1 and F2.
If the function is off then the servo will not move, if the function
is turned on then the corresponding servo can be control by the
speed knob of the controller.

Each servo has a set of CV values that are used to define how the
servo behaves, a lower and upper limit on the travel of the servo
and the maximum speed the servo will travel between the lower and
upper limits.

Two modes of operation are supported, relative or absolute mode.
In relative mode the speed knob and direct control the direction
of movement of the servo and the percentage of the maximum speed
at which the servo moves. In absolute mode the servo ignores the
speed setting in the CVs and simply moves to a position that matches
the current speed knob setting. E.g. if the speed know is set to
40% then the servo will move to a position 40% of the way between
the lower and upper limits set in the CVs for that servo.

## DCC connection

The DCC input signal should be connected to pin 2 of the Arduino
and the acknowledge signal to pin 3 of the Arduino.

## Servo connections

The servos should have the control input of the servo connected to
pins 9, 10 and 11 respectively.

## Startup

Pin 4 of the Arduino may be used for a "soft start" approach. If
this is used to control the supply of power to the servos then it
will stop servo jitter on startup. Pin 4 will be high on startup
and taken low after the decoder has receved power for 5 seconds.
This output can be used to control the voltage supplied to the servo
power, but must not be used as the direct power to the servo as it
is not capable of supplying the required current.

The onboard LED (pin 13) will be lit when pin 4 is set low.

## CV Values

The decoder supports the following CV values:

| CV | Description | Default |
| -----: | --- | ---: |
| 1 | Short address of the decoder | 3 |
| 7 | Decoder version (read-only) | 2 |
| 8 | Manufacturer ID | DIY |A
| 17 | Extended address uppper 6 bits | 0 |
| 18 | Extended address lower 8 bits | 3 |
| 29 | Configuration options | 0 |
| 30 | First servo lower limit angle (degrees) | 0 |
| 31 | First servo upper limit angle (degrees) | 180 |
| 32 | First servo travel time (seconds) | 5 |
| 33 | First servo configuration flags (see below) | 3 |
| 35 | First servo lower limit angle (degrees) | 0 |
| 36 | First servo upper limit angle (degrees) | 180 |
| 37 | First servo travel time (seconds) | 5 |
| 38 | First servo configuration flags (see below) | 3 |
| 40 | First servo lower limit angle (degrees) | 0 |
| 41 | First servo upper limit angle (degrees) | 180 |
| 42 | First servo travel time (seconds) | 5 |
| 43 | First servo configuration flags (see below) | 3 |

The servo configuration values (CV33, CV38 and CV 40) are defined as follows

| 7 | 6  | 5  | 4  | 3 | 2 | 1 & 0 |
| --- | --- | --- | --- | --- | --- | --- |
| Unused | Unused | Unused | Unused | Unused | Absolute Mode | Initial Position |

Absolute mode controls the effect the speed knob has on the servo
position. A value of 0 in this bit field results in the speed setting
controllign the speed of the servo movement, relative the minimum
travel time set in the relavant CV fo rthe servo. A value of 1 means
the speed knob poisition sets the absolute position of the servo.
In absolute mode the end positions are still taken itno account,
so a 10% speed setting means 10% of travel from lower limit to upper
limit.

Initial position bits control the movement of the servo at startup. 

| Value | Description |
| :---: | --- |
| 0 | No movement of the servo at startup |
| 1 | The servo moves to the lower limit position at startup |
| 2 | The servo moves to the upper limit position at startup |
| 3 | The servo moves to the mid point between the lower and upper limit position at startup |
