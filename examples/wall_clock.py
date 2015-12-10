#!/usr/bin/python

###############################################################################
#                                                                             #
# WS2812-RPi                                                                  #
# ==========                                                                  #
# A C++ library for driving WS2812 RGB LED's (known as 'NeoPixels' by         #
#     Adafruit) directly from a Raspberry Pi with accompanying Python wrapper #
# Copyright (C) 2015 Rob Kent                                                 #
#                                                                             #
# This program is free software: you can redistribute it and/or modify        #
# it under the terms of the GNU General Public License as published by        #
# the Free Software Foundation, either version 3 of the License, or           #
# (at your option) any later version.                                         #
#                                                                             #
# This program is distributed in the hope that it will be useful,             #
# but WITHOUT ANY WARRANTY; without even the implied warranty of              #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               #
# GNU General Public License for more details.                                #
#                                                                             #
# You should have received a copy of the GNU General Public License           #
# along with this program.  If not, see <http://www.gnu.org/licenses/>.       #
#                                                                             #
###############################################################################

###############################################################################
# DISCLAIMER!                                                                 #
# ===========                                                                 #
#                                                                             #
# This example is thoroughly untested! If you try it and it works then please #
# let me know. If you try it and it doesn't work then please let me know what #
# the problem is (and any fixes).                                             #
#                                                                             #
###############################################################################

from threading import Timer
from time import sleep
from datetime import datetime

from NeoPixel import NeoPixel

class WallClock(object):
    def __init__(self):
        # Create a NeoPixel object with 60 LEDs
        # and turn down the brightness
        self._strip=NeoPixel(60)
        self._strip.begin()
        self._strip.setBrightness(.25)
        self._strip.show()

    def tick(self):
        # Schedule the next tick to limit drift
        self._timer=Timer(1., self.tick)
        self._timer.start()

        # Get the current time
        now=datetime.now()
        second=now.second
        minute=now.minute
        hour=now.hour

        # Adjust 24hr time to 12hr time
        if hour>=12: hour-=12

        # The hour is shown at every 5th LED
        hour*=5

        # Clear the display
        self.clear()

        # Set the hour LED to blue
        self._strip.setPixelColor(hour, 0, 0, 255)

        # Set the minute LED to red unless the minute
        # hour LED are the same then alternate the colour
        # between red and blue every second
        if hour==minute and second%2:
            self._strip.setPixelColor(minute, 0, 0, 255)
        else: self._strip.setPixelColor(minute, 255, 0, 0)

        # Set the second LED to green
        self._strip.setPixelColor(second, 0, 255, 0)

        # Update the display
        self._strip.show()

    def clear(self):
        # Set every pixel off
        for i in range(60):
            self._strip.setPixelColor(i, 0, 0, 0)
        self._strip.show()

if __name__=="__main__":
    w=WallClock()
    sleep(3.)
    w.tick()
