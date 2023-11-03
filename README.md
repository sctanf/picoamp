firmware for pico with usb audio, i2s, and dsp

this is modified for lower latency, and has limited dsp functions (biquads and scuffed full wave integrator, not used)

created for a project to power Valve Index speakers with a Pico/RP2040 and I2S amplifers (MAX98357/MAX98360)

if you do make this with individual modules, you might need to remove the passives that are on the amplifier module's output, the MAX98357 does not require these and at least for my modules they were destroying the audio quality!

![pcb](../../blob/main/picoamp.png)

based off pico-playground usb_sound_card
