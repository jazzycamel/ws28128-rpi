g++ -c ws2812-rpi.cpp
g++ -c -I/usr/include/python2.7 -I/usr/include -fPIC  ws2812-rpi-python.cpp
g++ -shared -Wl,--export-dynamic ws2812-rpi.o ws2812-rpi-python.o -L/usr/lib -lboost_python-py27 -L/usr/lib/python2.7/config -lpython2.7 -o NeoPixel.so
