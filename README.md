## firmware presets
~~Note: most of these are, in the end, tuned by my ear and could sound a bit "off," you can use passthru fw if you do not need any dsp feature and/or will use software dsp~~

I have a cheap IMM-6 calibration microphone now, newer calibrations should be a good starting point

You can find different presets under releases.

## picoamp
firmware for pico with usb audio, i2s, and dsp

this is modified for lower latency, and has limited dsp functions (biquads and scuffed full wave integrator, not used)

supports 2ch 16/24/32bit 48khz, although internally there is only up to 28 bits of dynamic range, this is probably good enough

created for a project to power Valve Index speakers with a Pico/RP2040 and I2S amplifers (MAX98357/MAX98360), see https://github.com/sctanf/Bigscreen-Beyond-Valve-Index-Strap-Adapter/ and https://github.com/sctanf/picoamp-PCB

## hardware
I linked above for the companion boards that will work with picoamp firmware, but you can also build it out of module parts.

if you do make this with individual modules, you might need to remove the passives that are on the amplifier module's output, the MAX98357 does not require these and at least for my modules they were destroying the audio quality!

## wiring diagram for breakout boards
![wiring diagram for breakout boards](../../blob/main/images/picoamp.webp)

## tuning
It's possible to use any set of filters you want, these can be generated by Room EQ Wizard. picoamp supports 9 biquad filters, or 1x filter for bass tuning and another 8x filters

The newer firmware can support more filters, up to 18! However the newer firmware with high quality calibrations use less than 8.

todo

based off pico-playground usb_sound_card
