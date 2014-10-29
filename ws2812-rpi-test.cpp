#include "ws2812-rpi.h"

int main(int argc, char **argv){
    NeoPixel *n=new NeoPixel(24);

    while(true) n->effectsDemo();
    delete n;

    return 0;
}