# STEPswiftcode/ exp2swift
exp2swift translator which translates STEP schema files described in ISO 10303-11 STEP EXPRESS language to the Swift programing language.

by Tsutomu Yoshida, Minokamo Japan.

Based on the [STEPcode](http://stepcode.github.io/docs/home/)/STEP Class Libraties (SCL) by the U.S. National Institute of Stnadards and Technology (NIST), the exp2swift translates STEP schema files described in ISO 10303-11 EXPRESS language to the Swift programing languate. 
Currently it can only handle a single schema file (the ISO 10303-11:1994 longform schema).
The main intention of the development of this translator is to translate ISO 10303-242 (AP242) schema definition into the Swift programing language. therefore the translator is only tested for the AP242 2nd edition schema.

Speaking of the AP242 schema definition, it was not able to translate the original schema definition because of the difficulties related to the implementation of SELECT data type related operator handlings. In addition it was surfaced that the original AP242 schema definition contains several apparent bugs and inefficiencies in some function definitions.
To resolve these problems, the original AP242 schema definition was slightly modified before translation. The modifications applied to the original schema definition has been "diff"ed and provided in this package. To restore the exp2swift translatable AP242 schema definition file, run the patch command and apply the required modifications to the [original schema definition file](https://www.cax-if.org/cax/cax_express.php "CAx Imteroperability Forum") (which is required to obtain separately).



## swift STEP code suite
* [SwiftSDAIcore](https://github.com/tsun7170/SwiftSDAIcore)
* [SwiftSDAIap242](https://github.com/tsun7170/SwiftSDAIap242)
* [SwiftAP242PDMkit](https://github.com/tsun7170/SwiftAP242PDMkit)
* [simpleP21ReadSample](https://github.com/tsun7170/simpleP21ReadSample)
* [multipleP21ReadsSample](https://github.com/tsun7170/multipleP21ReadsSample)
* [STEPswiftcode/exp2swift](https://github.com/tsun7170/STEPswiftcode)


## Development environment
* Xcode version 12.4
* macOS Catalina version 10.15.7
