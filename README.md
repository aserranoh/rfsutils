
# rfsutils

`rfsutils` is a bunch of tools that help with simple tasks like blink leds or
listen for events in buttons, things that are used in almost all the robots.

## Contents of `rfsutils`

These are the utilities included:

### `ledblink`

Utility to make LEDs blink. The LED must be connected to a pin of the GPIO
port. For example:

```bash
ledblink -g 22 o200_200o200
```

The parameter `-g` gives the number of the GPIO pin where the LED is connected.
The last argument is a sequence of blinks. The sequence is composed by the
characters 'o' (ON) and '_' (OFF), each followed by a number that represents
a time in milliseconds. The sequence in the example means: turn **ON** the LED
for 200 ms, then turn it **OFF** for another 200 ms and finally turn it **ON**
again for another 200 ms. At the end of the sequence the LED is turned allways
OFF.

### `buttonsd`

`buttonsd` is a daemon that listens for events (clicks) in buttons connected
to the GPIO and triggers some actions (commands) when they happen. See
`buttonsd --help` for instructions on how to start this daemon.

At the moment, `buttonsd` supports two types of events:

* Click (or short click).
* Long click (two seconds or more).

`buttonsd` uses a configuration file (can be given in the command line
arguments but by default will be usually `/etc/buttonsd.conf`) where each line
describes an action triggered by an event. For example:

```
7_click=shutdown -r now
7_long_click=shutdown -h now
```

The first line means that when the button connected at the GPIO pin number 7 is
clicked (short click), the command `shutdown -r now` (reboot) must be executed.
The second line means that when the same button is clicked (longer time), the
command `shutdown -h now` (halt) must be executed.

## Prerequisites

`rfsutils` requires the library `rfsgpio`, also from the project
*Robots From Scratch!*.

## Installing

To install this library, just clone the repository (or download the release
tarball) and execute the three known steps:

```
./configure
make
make install
```

## Authors

**Antonio Serrano Hernandez**.

## License

This project is licensed under the GPLv3 License - see the `COPYING` file for
details.

