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

