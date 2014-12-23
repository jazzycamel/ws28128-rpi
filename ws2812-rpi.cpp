/*
###############################################################################
#                                                                             #
# WS2812-RPi                                                                  #
# ==========                                                                  #
# A C++ library for driving WS2812 RGB LED's (known as 'NeoPixels' by         #
#     Adafruit) directly from a Raspberry Pi with accompanying Python wrapper #
# Copyright (C) 2014 Rob Kent                                                 #
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
*/
#include "ws2812-rpi.h"

struct control_data_s* NeoPixel::ctl=0;
uint8_t* NeoPixel::virtbase=0;
volatile unsigned int* NeoPixel::pwm_reg=0;
volatile unsigned int* NeoPixel::clk_reg=0;
volatile unsigned int* NeoPixel::dma_reg=0;
volatile unsigned int* NeoPixel::gpio_reg=0;

// PUBLIC

NeoPixel::NeoPixel(unsigned int n)
    : numLEDs(n)
{
    LEDBuffer.resize(n);
    brightness=DEFAULT_BRIGHTNESS;

    initHardware();
    clearLEDBuffer();
}

NeoPixel::~NeoPixel(){
    terminate(0);
    //delete LEDBuffer;
}

void NeoPixel::begin(){};

void NeoPixel::show(){
    int i, j;
    unsigned int LEDBuffeWordPos = 0;
    unsigned int PWMWaveformBitPos = 0;
    unsigned int colorBits = 0;
    unsigned char colorBit = 0;
    unsigned int wireBit = 0;
    Color_t color;

    for(i=0; i<numLEDs; i++) {
        LEDBuffer[i].r *= brightness;
        LEDBuffer[i].g *= brightness;
        LEDBuffer[i].b *= brightness;
        colorBits = ((unsigned int)LEDBuffer[i].r << 8) | ((unsigned int)LEDBuffer[i].g << 16) | LEDBuffer[i].b;

        for(j=23; j>=0; j--) {
            colorBit = (colorBits & (1 << j)) ? 1 : 0;
            switch(colorBit) {
                case 1:
                    setPWMBit(wireBit++, 1);
                    setPWMBit(wireBit++, 1);
                    setPWMBit(wireBit++, 0);
                    break;
                case 0:
                    setPWMBit(wireBit++, 1);
                    setPWMBit(wireBit++, 0);
                    setPWMBit(wireBit++, 0);
                    break;
            }
        }
    }

    ctl = (struct control_data_s *)virtbase;
    dma_cb_t *cbp = ctl->cb;

    for(i = 0; i < (cbp->length / 4); i++) {
        ctl->sample[i] = PWMWaveform[i];
    }

    startTransfer();

    float bitTimeUSec = (float)(NUM_DATA_WORDS * 32) * 0.4;
    usleep((int)bitTimeUSec);
};

unsigned char NeoPixel::setPixelColor(unsigned int pixel, unsigned char r, unsigned char g, unsigned char b){
    if(pixel < 0) {
        printf("Unable to set pixel %d (less than zero?)\n", pixel);
        return false;
    }
    if(pixel > numLEDs - 1) {
        printf("Unable to set pixel %d (LED buffer is %d pixels long)\n", pixel, numLEDs);
        return false;
    }
    LEDBuffer[pixel] = RGB2Color(r, g, b);
    return true;
}

unsigned char NeoPixel::setPixelColor(unsigned int pixel, Color_t c){
    if(pixel < 0) {
        printf("Unable to set pixel %d (less than zero?)\n", pixel);
        return false;
    }
    if(pixel > numLEDs - 1) {
        printf("Unable to set pixel %d (LED buffer is %d pixels long)\n", pixel, numLEDs);
        return false;
    }
    LEDBuffer[pixel] = c;
    return true;
}

bool NeoPixel::setBrightness(float b){
    if(b < 0) {
        printf("Brightness can't be set below 0.\n");
        return false;
    }
    if(b > 1) {
        printf("Brightness can't be set above 1.\n");
        return false;
    }
    brightness = b;
    return true;
}

//Color_t* NeoPixel::getPixels(){ return &LEDBuffer[0]; }
std::vector<Color_t> NeoPixel::getPixels(){ return LEDBuffer; }

float NeoPixel::getBrightness(){ return brightness; }

Color_t NeoPixel::getPixelColor(unsigned int pixel){
    if(pixel < 0) {
        printf("Unable to get pixel %d (less than zero?)\n", pixel);
        return RGB2Color(0, 0, 0);
    }
    if(pixel > numLEDs - 1) {
        printf("Unable to get pixel %d (LED buffer is %d pixels long)\n", pixel, numLEDs);
        return RGB2Color(0, 0, 0);
    }
    return LEDBuffer[pixel];
}

unsigned int NeoPixel::numPixels(){ return numLEDs; }

void NeoPixel::clear(){ clearLEDBuffer(); }

// PRIVATE
void NeoPixel::printBinary(unsigned int i, unsigned int bits){
    int x;
    for(x=bits-1; x>=0; x--) {
        printf("%d", (i & (1 << x)) ? 1 : 0);
        if(x % 16 == 0 && x > 0) {
            printf(" ");
        } else if(x % 4 == 0 && x > 0) {
            printf(":");
        }
    }    
}

unsigned int NeoPixel::reverseWord(unsigned int word){
    unsigned int output = 0;
    unsigned char bit;
    int i;
    for(i=0; i<32; i++) {
        bit = word & (1 << i) ? 1 : 0;
        output |= word & (1 << i) ? 1 : 0;
        if(i<31) {
            output <<= 1;
        }
    }
    return output;    
}

void NeoPixel::terminate(int dummy){
    if(dma_reg) {
        CLRBIT(dma_reg[DMA_CS], DMA_CS_ACTIVE);
        usleep(100);
        SETBIT(dma_reg[DMA_CS], DMA_CS_RESET);
        usleep(100);
    }

    // Shut down PWM
    if(pwm_reg) {
        CLRBIT(pwm_reg[PWM_CTL], PWM_CTL_PWEN1);
        usleep(100);
        pwm_reg[PWM_CTL] = (1 << PWM_CTL_CLRF1);
    }
    
    // Free the allocated memory
    if(page_map != 0) {
        free(page_map);
    }
}

void NeoPixel::fatal(char *fmt, ...){
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    terminate(0);
}

unsigned int NeoPixel::mem_virt_to_phys(void *virt){
    unsigned int offset = (uint8_t *)virt - virtbase;
    return page_map[offset >> PAGE_SHIFT].physaddr + (offset % PAGE_SIZE);    
}

unsigned int NeoPixel::mem_phys_to_virt(uint32_t phys){
    unsigned int pg_offset = phys & (PAGE_SIZE - 1);
    unsigned int pg_addr = phys - pg_offset;
    int i;

    for (i = 0; i < NUM_PAGES; i++) {
        if (page_map[i].physaddr == pg_addr) {
            return (uint32_t)virtbase + i * PAGE_SIZE + pg_offset;
        }
    }
    fatal("Failed to reverse map phys addr %08x\n", phys);

    return 0;    
}

void* NeoPixel::map_peripheral(uint32_t base, uint32_t len){
    int fd = open("/dev/mem", O_RDWR);
    void * vaddr;

    if (fd < 0)
        fatal("Failed to open /dev/mem: %m\n");
    vaddr = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, base);
    if (vaddr == MAP_FAILED)
        fatal("Failed to map peripheral at 0x%08x: %m\n", base);
    close(fd);

    return vaddr;    
}

void NeoPixel::clearPWMBuffer(){
    memset(PWMWaveform, 0, NUM_DATA_WORDS * 4);
}

void NeoPixel::clearLEDBuffer(){
    int i;
    for(i=0; i<numLEDs; i++) {
        LEDBuffer[i].r = 0;
        LEDBuffer[i].g = 0;
        LEDBuffer[i].b = 0;
    }    
}

Color_t NeoPixel::RGB2Color(unsigned char r, unsigned char g, unsigned char b){
    //Color_t color = { r, g, b };
    Color_t color(r, g, b);
    return color;
}

Color_t NeoPixel::Color(unsigned char r, unsigned char g, unsigned char b){
    return RGB2Color(r, g, b);
}

void NeoPixel::setPWMBit(unsigned int bitPos, unsigned char bit){
    unsigned int wordOffset = (int)(bitPos / 32);
    unsigned int bitIdx = bitPos - (wordOffset * 32);

    switch(bit) {
        case 1:
            PWMWaveform[wordOffset] |= (1 << (31 - bitIdx));
            break;
        case 0:
            PWMWaveform[wordOffset] &= ~(1 << (31 - bitIdx));
            break;
    }
}

unsigned char NeoPixel::getPWMBit(unsigned int bitPos){
    unsigned int wordOffset = (int)(bitPos / 32);
    unsigned int bitIdx = bitPos - (wordOffset * 32);

    if(PWMWaveform[wordOffset] & (1 << bitIdx)) {
        return true;
    } else {
        return false;
    }    
}

void NeoPixel::initHardware(){
    int i = 0;
    int pid;
    int fd;
    char pagemap_fn[64];

    // Clear the PWM buffer
    clearPWMBuffer();

    // Set up peripheral access
    dma_reg = (unsigned int*) map_peripheral(DMA_BASE, DMA_LEN);
    dma_reg += 0x000;
    pwm_reg = (unsigned int*)map_peripheral(PWM_BASE, PWM_LEN);
    clk_reg = (unsigned int*)map_peripheral(CLK_BASE, CLK_LEN);
    gpio_reg = (unsigned int*)map_peripheral(GPIO_BASE, GPIO_LEN);


    // Set PWM alternate function for GPIO18
    SET_GPIO_ALT(18, 5);

    // Allocate memory for the DMA control block & data to be sent
    virtbase = (uint8_t*) mmap(
        NULL,
        NUM_PAGES * PAGE_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED |
        MAP_ANONYMOUS |
        MAP_NORESERVE |
        MAP_LOCKED,
        -1,
        0);

    if (virtbase == MAP_FAILED) {
        fatal("Failed to mmap physical pages: %m\n");
    }

    if ((unsigned long)virtbase & (PAGE_SIZE-1)) {
        fatal("Virtual address is not page aligned\n");
    }

    // Allocate page map (pointers to the control block(s) and data for each CB
    page_map =(page_map_t*) malloc(NUM_PAGES * sizeof(*page_map));
    if (page_map == 0)
        fatal("Failed to malloc page_map: %m\n");

    pid = getpid();
    sprintf(pagemap_fn, "/proc/%d/pagemap", pid);
    fd = open(pagemap_fn, O_RDONLY);

    if (fd < 0) {
        fatal("Failed to open %s: %m\n", pagemap_fn);
    }

    if (lseek(fd, (unsigned long)virtbase >> 9, SEEK_SET) != (unsigned long)virtbase >> 9) {
        fatal("Failed to seek on %s: %m\n", pagemap_fn);
    }

    for (i = 0; i < NUM_PAGES; i++) {
        uint64_t pfn;
        page_map[i].virtaddr = virtbase + i * PAGE_SIZE;

        page_map[i].virtaddr[0] = 0;

        if (read(fd, &pfn, sizeof(pfn)) != sizeof(pfn)) {
            fatal("Failed to read %s: %m\n", pagemap_fn);
        }

        if ((pfn >> 55)&0xfbf != 0x10c) {
            fatal("Page %d not present (pfn 0x%016llx)\n", i, pfn);
        }

        page_map[i].physaddr = (unsigned int)pfn << PAGE_SHIFT | 0x40000000;
    }

    // Set up control block
    ctl = (struct control_data_s *)virtbase;
    dma_cb_t *cbp = ctl->cb;
    unsigned int phys_pwm_fifo_addr = 0x7e20c000 + 0x18;

    cbp->info = DMA_TI_CONFIGWORD;

    cbp->src = mem_virt_to_phys(ctl->sample);   

    cbp->dst = phys_pwm_fifo_addr;

    cbp->length = ((numLEDs * 2.25) + 1) * 4;
    if(cbp->length > NUM_DATA_WORDS * 4) {
        cbp->length = NUM_DATA_WORDS * 4;
    }

    cbp->stride = 0;  
    cbp->pad[0] = 0;
    cbp->pad[1] = 0;   
    cbp->next = 0;

    dma_reg[DMA_CS] |= (1 << DMA_CS_ABORT);
    usleep(100);
    dma_reg[DMA_CS] = (1 << DMA_CS_RESET);
    usleep(100);


    // PWM Clock
    clk_reg[PWM_CLK_CNTL] = 0x5A000000 | (1 << 5);
    usleep(100);

    CLRBIT(pwm_reg[PWM_DMAC], PWM_DMAC_ENAB);
    usleep(100);

    unsigned int idiv = 400;
    unsigned short fdiv = 0;
    clk_reg[PWM_CLK_DIV] = 0x5A000000 | (idiv << 12) | fdiv;
    usleep(100);

    clk_reg[PWM_CLK_CNTL] = 0x5A000015;
    usleep(100);


    // PWM
    pwm_reg[PWM_CTL] = 0;

    pwm_reg[PWM_RNG1] = 32;
    usleep(100);
    
    pwm_reg[PWM_DMAC] =
        (1 << PWM_DMAC_ENAB) |
        (8 << PWM_DMAC_PANIC) |
        (8 << PWM_DMAC_DREQ);
    usleep(1000);
    
    SETBIT(pwm_reg[PWM_CTL], PWM_CTL_CLRF1);
    usleep(100);
    
    CLRBIT(pwm_reg[PWM_CTL], PWM_CTL_RPTL1);
    usleep(100);
    
    CLRBIT(pwm_reg[PWM_CTL], PWM_CTL_SBIT1);
    usleep(100);
    
    CLRBIT(pwm_reg[PWM_CTL], PWM_CTL_POLA1);
    usleep(100);
    
    SETBIT(pwm_reg[PWM_CTL], PWM_CTL_MODE1);
    usleep(100);
    
    SETBIT(pwm_reg[PWM_CTL], PWM_CTL_USEF1);
    usleep(100);

    CLRBIT(pwm_reg[PWM_CTL], PWM_CTL_MSEN1);
    usleep(100);   

    SETBIT(dma_reg[DMA_CS], DMA_CS_INT);
    usleep(100);
    
    SETBIT(dma_reg[DMA_CS], DMA_CS_END);
    usleep(100);
    
    dma_reg[DMA_CONBLK_AD] = mem_virt_to_phys(ctl->cb);
    usleep(100);

    dma_reg[DMA_DEBUG] = 7;
    usleep(100);
}

void NeoPixel::startTransfer(){
    dma_reg[DMA_CONBLK_AD] = mem_virt_to_phys(ctl->cb);
    dma_reg[DMA_CS] = DMA_CS_CONFIGWORD | (1 << DMA_CS_ACTIVE);
    usleep(100);

    SETBIT(pwm_reg[PWM_CTL], PWM_CTL_PWEN1);    
}

Color_t NeoPixel::wheel(uint8_t wheelPos) {
    if(wheelPos < 85) {
        return Color(wheelPos * 3, 255 - wheelPos * 3, 0);
    } else if(wheelPos < 170) {
        wheelPos -= 85;
        return Color(255 - wheelPos * 3, 0, wheelPos * 3);
    } else {
        wheelPos -= 170;
        return Color(0, wheelPos * 3, 255 - wheelPos * 3);
    }
}

void NeoPixel::colorWipe(Color_t c, uint8_t wait) {
    uint16_t i;
    for(i=0; i<numPixels(); i++) {
        setPixelColor(i, c);
        show();
        usleep(wait * 1000);
    }
}

void NeoPixel::rainbow(uint8_t wait) {
    uint16_t i, j;

    for(j=0; j<256; j++) {
        for(i=0; i<numPixels(); i++) {
            setPixelColor(i, wheel((i+j) & 255));
        }
        show();
        usleep(wait * 1000);
    }
}

void NeoPixel::rainbowCycle(uint8_t wait) {
    uint16_t i, j;

    for(j=0; j<256*5; j++) {
        for(i=0; i<numPixels(); i++) {
            setPixelColor(i, wheel(((i * 256 / numPixels()) + j) & 255));
        }
        show();
        usleep(wait * 1000);
    }
}

void NeoPixel::theaterChase(Color_t c, uint8_t wait) {
    unsigned int j, q, i;
    for (j=0; j<15; j++) {
        for (q=0; q < 3; q++) {
            for (i=0; i < numPixels(); i=i+3) {
                setPixelColor(i+q, c);
            }
            show();
     
            usleep(wait * 1000);

            for (i=0; i < numPixels(); i=i+3) {
                setPixelColor(i+q, 0, 0, 0);
            }
        }
    }
}

void NeoPixel::theaterChaseRainbow(uint8_t wait) {
    int j, q, i;
    for (j=0; j < 256; j+=4) {
        for (q=0; q < 3; q++) {
            for (i=0; i < numPixels(); i=i+3) {
                setPixelColor(i+q, wheel((i+j) % 255));
            }
            show();

            usleep(wait * 1000);
       
            for (i=0; i < numPixels(); i=i+3) {
                setPixelColor(i+q, 0, 0, 0);
            }
        }
    }
}

long NeoPixel::map(long x, long in_min, long in_max, long out_min, long out_max){
    return (x-in_min)*(out_max-out_min)/(in_max-in_min)+out_min;
}

Color_t NeoPixel::gradientColor(std::vector<Color_t>& scheme, int range, int gradRange, int i){
    int curRange=i/range;
    int rangeIndex=i%range;
    int colorIndex=rangeIndex/gradRange;
    int start=colorIndex;
    int end=colorIndex+1;
    if(curRange%2!=0){
        start=(scheme.size()-1)-start;
        end=(scheme.size()-1)-end;
    }
    return Color(
        map(rangeIndex%gradRange, 0, gradRange, scheme[start].r, scheme[end].r),
        map(rangeIndex%gradRange, 0, gradRange, scheme[start].g, scheme[end].g),
        map(rangeIndex%gradRange, 0, gradRange, scheme[start].b, scheme[end].b)
    );
}

void NeoPixel::gradient(std::vector<Color_t>& scheme, int repeat, int speedMS){
    if(scheme.size()<2) return;

    int range=(int)ceil((float)numLEDs/(float)repeat);
    int gradRange=(int)ceil((float)range/(float)(scheme.size()-1));
    unsigned long time=millis();
    int offset=speedMS>0?time/speedMS:0;

    Color_t oldColor=gradientColor(scheme, range, gradRange, numLEDs-1+offset);
    for(int i=0; i<numLEDs; ++i){
        Color_t currentColor=gradientColor(scheme, range, gradRange, i+offset);
        if(speedMS>0){
            setPixelColor(
                i,
                map(time%speedMS, 0, speedMS, oldColor.r, currentColor.r),
                map(time%speedMS, 0, speedMS, oldColor.g, currentColor.g),
                map(time%speedMS, 0, speedMS, oldColor.b, currentColor.b)
            );
        } else {
            setPixelColor(i, currentColor.r, currentColor.g, currentColor.b);
        }
        oldColor=currentColor;
    }
    show();
}

void NeoPixel::bars(std::vector<Color_t>& scheme, int width, int speedMS){
    int maxSize=numLEDs/scheme.size();
    if(width>maxSize) return;

    int offset=speedMS>0?millis()/speedMS:0;

    for(int i=0; i<numLEDs; ++i){
        int colorIndex=((i+offset)%(scheme.size()*width))/width;
        setPixelColor(i, scheme[colorIndex].r, scheme[colorIndex].g, scheme[colorIndex].b);
    }
    show();
}

void NeoPixel::effectsDemo() {
    int i, j, ptr;
    float k;

    // Default effects from the Arduino lib
    colorWipe(Color(255, 0, 0), 50); // Red
    colorWipe(Color(0, 255, 0), 50); // Green
    colorWipe(Color(0, 0, 255), 50); // Blue
    theaterChase(Color(127, 127, 127), 50); // White
    theaterChase(Color(127,   0,   0), 50); // Red
    theaterChase(Color(  0,   0, 127), 50); // Blue
    rainbow(5);
    rainbowCycle(5);
    theaterChaseRainbow(50);

    // Watermelon fade :)
    for(k=0; k<0.5; k+=.01) {
        ptr=0;
        setBrightness(k);
        for(i=0; i<numLEDs; i++) {
            setPixelColor(i, i*5, 64, i*2);
        }
        show();
    }
    for(k=0.5; k>=0; k-=.01) {
        ptr=0;
        setBrightness(k);
        for(i=0; i<numLEDs; i++) {
            setPixelColor(i, i*5, 64, i*2);
        }
        show();
    }
    usleep(1000);

    // Random color fade
    srand(time(NULL));
    uint8_t lastRed = 0;
    uint8_t lastGreen = 0;
    uint8_t lastBlue = 0;
    uint8_t red, green, blue;
    Color_t curPixel;
    setBrightness(DEFAULT_BRIGHTNESS);
    for(j=1; j<16; j++) {
        ptr = 0;
        if(j % 3) {
            red = 120;
            green = 64;
            blue = 48;
        } else if(j % 7) {
            red = 255;
            green = 255;
            blue = 255;
        } else {
            red = rand();
            green = rand();
            blue = rand();
        }
        for(k=0; k<1; k+=.01) {
            for(i=0; i<numLEDs; i++) {
                setPixelColor(
                    i,
                    (red * k) + (lastRed * (1-k)),
                    i * (255 / numLEDs), //(green * k) + (lastGreen * (1-k)),
                    (blue * k) + (lastBlue * (1-k))
                    );
                curPixel = getPixelColor(i);
            }
            show();
        }
        lastRed = red;
        lastGreen = green;
        lastBlue = blue;
    }
}
