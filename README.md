# STEPswiftcode/ exp2swift
exp2swift translator which translates STEP schema files described in ISO 10303-11 STEP EXPRESS language to the Swift programing language.

by Tsutomu Yoshida, Minokamo Japan.


## software category: command line tool
how to use:
exp2swift <input STEP schema file path>

output:
a set of translated STEP-schema swift source files are generated under current working directory. 

The environment variable EXPRESS_PATH can be utilized to specify the search paths for the STEP schema files.

## overview
Based on the [STEPcode](http://stepcode.github.io/docs/home/)/STEP Class Libraries (SCL) by the U.S. National Institute of Standards and Technology (NIST), the exp2swift translates STEP schema files described in ISO 10303-11:1994 EXPRESS language to the Swift programing language. 
Currently it can only handle a single schema file (the ISO 10303-11:1994 longform schema).
The main intention of the development of this translator is to translate ISO 10303-242 (AP242) schema definition into the Swift programing language.




## swift STEP code suite
* [SwiftSDAIcore](https://github.com/tsun7170/SwiftSDAIcore) Swift SDAI runtime environment
* [SwiftSDAIap242](https://github.com/tsun7170/SwiftSDAIap242) Swift translated AP242 schema definition (ed2)
* [SwiftAP242PDMkit](https://github.com/tsun7170/SwiftAP242PDMkit) Swift implementation of PDM schema usage guide
* [simpleP21ReadSample](https://github.com/tsun7170/simpleP21ReadSample) Single P21 file reading and validation sample code
* [multipleP21ReadsSample](https://github.com/tsun7170/multipleP21ReadsSample) Tree of P21 files reading and validation sample code
* **[STEPswiftcode/exp2swift](https://github.com/tsun7170/STEPswiftcode) EXPRESS to Swift translator**


## Development environment
* Xcode version 26.2
* macOS Tahoe 26.2
