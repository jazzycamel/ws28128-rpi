#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
using namespace boost::python;

#include "ws2812-rpi.h"

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(
    setPixelColor1, NeoPixel::setPixelColor, 4, 4
)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(
    setPixelColor2, NeoPixel::setPixelColor, 2, 2
)

BOOST_PYTHON_MODULE(NeoPixel){
    class_<Color_t>("Color")
        .def_readwrite("r", &Color_t::r)
        .def_readwrite("g", &Color_t::g)
        .def_readwrite("b", &Color_t::b);

    class_<std::vector<Color_t> >("Color_t_vector")
        .def(vector_indexing_suite<std::vector<Color_t> >());

    class_<NeoPixel>("NeoPixel", init<unsigned int>())
        .def("begin", &NeoPixel::begin)
        .def("show", &NeoPixel::show)
        .def("setPixelColor",
             static_cast<unsigned char(NeoPixel::*)(unsigned int, unsigned char, unsigned char, unsigned char)>(&NeoPixel::setPixelColor),
             setPixelColor1())
        .def("setPixelColor",
             static_cast<unsigned char(NeoPixel::*)(unsigned int, Color_t)>(&NeoPixel::setPixelColor), 
             setPixelColor2())
        .def("setBrightness", &NeoPixel::setBrightness)
        .def("getPixels", &NeoPixel::getPixels)
        .def("getBrightness", &NeoPixel::getBrightness)
        .def("getPixelColor", &NeoPixel::getPixelColor)
        .def("numPixels", &NeoPixel::numPixels)
        .def("clear", &NeoPixel::clear)
        .def("colorWipe", &NeoPixel::colorWipe)
        .def("rainbow", &NeoPixel::rainbow)
        .def("rainbowCycle", &NeoPixel::rainbowCycle)
        .def("theaterChase", &NeoPixel::theaterChase)
        .def("theaterChaseRainbow", &NeoPixel::theaterChaseRainbow)
        .def("gradient", &NeoPixel::gradient)
        .def("bars", &NeoPixel::bars)
        .def("effectsDemo", &NeoPixel::effectsDemo)
	;
}
