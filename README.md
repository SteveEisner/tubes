# tubes
These portable LED light poles make pretty lights for a dance party - and better yet:

They're **portable**! Convenient tripod bases keep them standing at attention, and a 10Ah USB battery (like the one you charge your phone with) will power them for about 8 hours.

They're **versatile**! You can deploy them in a [wide open space at Burning Man](https://youtu.be/UtXL0ScNUoE), in a [private event space](https://youtu.be/4YlAoL8A3Lg), or right next to each other [in your living room](https://youtu.be/UmiRBCOAMBg).  (Click into these links for videos of them in action.)

They're **coordinated**! If you put them next to each other, they sync using near-field radio. And they'll even daisy-chain, meaning the furthest ones can be really far apart as long as there are some in the middle to relay the signal.

![IMG_1521](assembly/poles_in_use.jpg)

They're **sturdy**! I built 20 of them and deployed them at parties in all kinds of weather, from hot & dusty windstorms to torrential rain. They've been tossed in a truck, carried around as totems, knocked or blown over, and almost all of 'em are still working.  Just don't let people use them like light sabers (lesson learned.)

They're also **in progress** with lots of things that could be improved.  But for now:

* they play patterns from an expanding set of "genetically-driven" combinations
* they stay in sync or deliberately drift from each other in pleasing ways
* they operate internally at a certain BPM - allowing them to sync to music

### How do they work?

Each light tube is running custom software on a Teensy LC (Arduino-compatible) microcontroller. The software is running a generative light program that has a simple "DNA" specifying its pattern, color, effects overlay, offset, and so on.

The patterns run on a clock that's synced to a specific BPM and counts out the 4/4 rhythm of most dance music, so they morph and change on individual beats, measures, and phrases. After several beat phrases pass, a pole's DNA is mutated a little, which can cause it to change color, or adopt a new pattern, etc.

An on-board NRF radio lets a light pole broadcast its DNA and clock timer; when others hear the signal, they can choose to follow along by updating their own pattern DNA to match it. The method of coordination is pretty simple right now: each pole has an 8-bit ID, and lower-ID poles obey higher-ID poles.

If they're all close enough, they'll soon start doing the same thing as a group and keeping their clock timers in sync. In case they're not close enough, poles also re-broadcast all the signals they receive and obey. This lets them pass along a signal through the group until all of them have found it.

Some randomness and chaos is intentional. Radio isn't 100% reliable, so they sometimes fall out of contact and then re-connect. And in some cases, the poles will deliberately offset their own clock a bit so that they are clearly doing the same thing but not exactly at the same time. Each tube is actually running several copies of the software and smoothly "cross-fading" between them, to avoid any jarring transitions.

### Want your own?

They're not for sale, but check out the build instructions in [this directory](assembly), complete with a parts list & assembly instructions. I'd love it if you build your own!

### Credits

The form factor was inspired by [Mark Lottor's Hexatron](http://www.3waylabs.com/projects/hex/). The color schemes were inspired by [the work of Christopher Schardt](https://americanart.si.edu/exhibitions/burning-man/online/christopher-schardt) and many were found on [cpt-city](http://soliton.vm.bytemark.co.uk/pub/cpt-city/).  The code was written using [FastLED](http://fastled.io/) and most patterns are evolved versions of FastLED examples.  It's based on [Chuck Sommerville's led-swarm](https://github.com/chucks13/LED-swarm), and still follows its [theory](https://github.com/chucks13/LED-swarm/blob/master/theory.txt) for radio connection. I used [Paul Stoffregen's WS2812Serial](https://github.com/PaulStoffregen/WS2812Serial) for smooth animation, and of course [his Teensy CPU](https://www.pjrc.com/teensy/teensyLC.html) made the whole thing possible. Standing on the shoulders of these giants let me create these in about a month, in time to debut at Burning Man 2019 and appear at many parties since!
