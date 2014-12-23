<h1>ws28128-rpi</h1>

A C++ library for driving WS2812 RGB LED's (known as 'NeoPixels' by Adafruit) directly from a Raspberry Pi with accompanying Python wrapper.

This solution uses the Raspberry Pi's DMA controller to drive the LED's directly and therefore requires no microcontroller or other intermediary circuit unless the LED's are powered at 5V rather 3.3V in which case a level converter will be necessary.

Before using WS2812 LED's with this or any other solution you should definitely read the Adafruit best practice guide (https://learn.adafruit.com/adafruit-neopixel-uberguide/best-practices). I cannot and will not be held responsible for any damage or injury caused by using this code. Use at your own risk, no guarantees or warranties offered.

<h2>Hardware</h2>

I do not recommend powering any LED's directly from the Raspberry Pi as they can be quite power hungry. Also, I prefer to power them from a 5V source rather than 3.3V which requires a level converter (74AHCT125 or equivalent). A big capacitor (1000uF, 6.3V) across the supply rails is a good idea irrespective of what voltage you are using. Data is output from the Raspberry Pi on GPIO18 (physical pin 12) and a current limiting resistor (300-500 ohms) should be placed between this and the first LED.

<h2>Software</h2>
<h3>C++</h3>
The C++ interface is a simple class named 'NeoPixel' with an API very similar to the Adafruit Arduino NeoPixel library. To use this in your own project you simply need to place the 'ws2812-rpi.h', 'ws2812-rpi-defines.h' and 'ws2812-rpi.cpp' files in your projects source directly and include the 'ws2812-rpi.h' header file. This library requires has no dependencies that require explicit declaration at compile time.

A simple test/example program is included in the form of the 'ws2812-rpi-test' executable, the source for which can be found in 'ws2812-rpi-test.cpp' and reads as follows:

```
#include "ws2812-rpi.h"

int main(int argc, char **argv){
    NeoPixel *n=new NeoPixel(24);

    while(true) n->effectsDemo();
    delete n;

    return 0;
}
```

This can be built by running the 'build_test.sh' script from the command line as follows:

```
$ ./build_test.sh
```

Any code using this library (including the test) must be run with super user privileges to allow access to the necessary registers. For examples, run the test program as follows:

```
$ sudo ./ws2812-rpi-test
```

<h3>Python</h3>
The accompanying Python module is created using the Boost Python library to wrap the C++ code. To use this module simply place the 'NeoPixel.so' shared object file in your project directory and import it as follows:

```
import NeoPixel
```

A simple test/example program is included in the form of the 'ws2812-rpi-test.py' script which reads as follows:

```
#!/usr/bin/python
from NeoPixel import NeoPixel

if __name__=="__main__":
    n=NeoPixel(60)
    n.setBrightness(1.)
    n.effectsDemo()
    n.clear()
    n.show()
    del n
    exit(0)
```

This script is run as follows:

```
$ sudo ./ws2812-rpi-test.py
```

The source for the Python wrapper can be found in the 'ws2812-rpi-python.cpp' file and it can be built by running the 'build_python.sh' script as follows:

```
$ ./build_python.sh
```

To do this you will need to have the Boost Python and Python development libraries installed as follows:

```
$ sudo apt-get update
$ sudo apt-get install libboost-python-dev python-dev
```
