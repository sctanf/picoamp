firmware for pico with usb audio, i2s, and dsp

this is modified for lower latency, and has limited dsp functions (biquads and scuffed full wave integrator, not used)

supports 2ch 16/24/32bit 48khz, although internally there is only up to 28 bits of dynamic range, this is probably good enough

created for a project to power Valve Index speakers with a Pico/RP2040 and I2S amplifers (MAX98357/MAX98360), see https://github.com/sctanf/Bigscreen-Beyond-Valve-Index-Strap-Adapter/ and https://github.com/sctanf/picoamp-PCB

if you do make this with individual modules, you might need to remove the passives that are on the amplifier module's output, the MAX98357 does not require these and at least for my modules they were destroying the audio quality!

![pcb](../../blob/main/images/picoamp.webp)

![pcb 3d](../../blob/main/images/picoamp2.webp)

based off pico-playground usb_sound_card
