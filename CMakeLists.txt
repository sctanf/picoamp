cmake_minimum_required(VERSION 3.12)

# Pull in PICO SDK (must be before project)
include(pico_sdk_import.cmake)

# We also need PICO EXTRAS
include(pico_extras_import.cmake)

project(pico_amp C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

add_subdirectory(i2s) # ???

add_executable(pico_amp
        main.c
        )

target_compile_definitions(pico_amp PRIVATE
        AUDIO_FREQ_MAX=48000

        # ours are zero based, so say so
        PICO_USBDEV_USE_ZERO_BASED_INTERFACES=1

        # need large descriptor
        PICO_USBDEV_MAX_DESCRIPTOR_SIZE=256


        PICO_USBDEV_ISOCHRONOUS_BUFFER_STRIDE_TYPE=1
        PICO_USBDEV_ENABLE_DEBUG_TRAgCE

        PICO_AUDIO_I2S_MONO_OUTPUT=0
        PICO_AUDIO_I2S_MONO_INPUT=0

        PICO_AUDIO_I2S_DATA_PIN=18
        PICO_AUDIO_I2S_CLOCK_PIN_BASE=16
)

target_link_libraries(pico_amp pico_stdlib usb_device pico_audio sctanf_i2s pico_multicore) # change the name so it doesnt try to use the original i2s stuffs
pico_add_extra_outputs(pico_amp)
