/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/usb_device.h"
#include "pico/audio.h"
#include "pico/audio_i2s.h"
#include "pico/multicore.h"
#include "lufa/AudioClassCommon.h"

#include "dsp/dsp.h"

// dsp audio buffers
dspfx buf0[192];
dspfx buf1[192];

// equalizer filters
biquad(eq_bq_0)
biquad(eq_bq_1)
biquad(eq_bq_2)
biquad(eq_bq_3)
biquad(eq_bq_4)
biquad(eq_bq_5)
biquad(eq_bq_6)
biquad(eq_bq_7)
biquad(eq_bq_8)
/*
biquad(eq_bq_9)
biquad(eq_bq_10)
biquad(eq_bq_11)
biquad(eq_bq_12)
biquad(eq_bq_13)
biquad(eq_bq_14)
biquad(eq_bq_15)
biquad(eq_bq_16)
biquad(eq_bq_17)
biquad(eq_bq_18)
*/
// now can run twice as many filters.. they are not used for now

bufring_t bufring1 = {
.len = 0,
.index = 0,
.index1 = 0,
};

#define EQ_ENABLE
#define BASS_ENABLE

#define USE_EQ 1

//#define EQ_HGC 0.203125,1.09375,-0.28125,-1.25,0.25,1.5,-0.5,-2.0 // [0.0, 0.0, -0.25, -0.5, -0.0625, -0.125, -0.015625, -0.03125]

#if (USE_EQ == 1) // index v1.3
// Maximum Volume
// #define POWER_LIMIT 3 // 2^3 > 8x > -18dB

#define EQ_BASS 1.0147105601538713,-1.995692614841887,0.981052089435134,-1.995692614841887,0.9957626495890052 // PK Fc 64 Hz Gain 18 dB Q 0.7

#define EQ_I_0 0.9933974303278078,-1.9621488960960585,0.969336316124395,-1.9621488960960585,0.9627337464522029
#define EQ_I_1 0.9843565518525887,-1.8078976382553416,0.8699633385164783,-1.8078976382553416,0.8543198903690672
#define EQ_I_2 0.8989508079118412,-1.5671635721286816,0.8054852979548176,-1.5671635721286816,0.7044361058666587
#define EQ_I_3 0.9842856901614626,-1.5730208560446943,0.8900530063029806,-1.5730208560446943,0.8743386964644432
#define EQ_I_4 0.8635104955659536,-1.126229935166492,0.7528748088546126,-1.126229935166492,0.6163853044205663
#define EQ_I_5 0.5826439753387763,0.1089574413184568,0.4021037545443685,0.1089574413184568,-0.0152522701168552
#define EQ_I_6 0.9154113079740384,0.9704547205751488,0.8035130988717817,0.9704547205751488,0.7189244068458199

#define EQ_I_7 0.9536527631125168,-0.9952466765497138,0.8923422890960174,-0.9952466765497138,0.8459950522085343 // PK Fc 7650 Hz Gain -8 dB Q 8
#elif (USE_EQ == 2) // s05 v4
#define EQ_BASS 1.0149551288553773,-1.997038766193103,0.9821182842924134,-1.997038766193103,0.9970734131477906 // PK Fc 45 Hz Gain 21 dB Q 0.6
#define LIMIT_MUL ((dspfx)(0.316*(double)(1<<27))) // 0.32x > -10dB limit on output, -10-21 > -31dB limit on input
#define BASS_MUL floatfx(1./11.2) // Bass peak filter is 21dB > 11.2x

#define EQ_I_0 0.99504537822055,-1.965539865289331,0.9710251558479132,-1.965539865289331,0.966070534068463
#define EQ_I_1 1.010024732294892,-1.916401745772548,0.920419341348879,-1.916401745772548,0.9304440736437708
#define EQ_I_2 0.9486271727897283,-1.8324892037526428,0.9157061631265591,-1.8324892037526428,0.8643333359162875
#define EQ_I_3 0.8951520004042253,-1.5352650428405143,0.7258677340245401,-1.5352650428405143,0.6210197344287652
#define EQ_I_4 0.7515456851097414,-0.0815791615282333,0.4410087604060678,-0.0815791615282333,0.1925544455158092
#define EQ_I_5 0.681737420729942,0.7921226957869918,0.5824835623713254,0.7921226957869918,0.2642209831012674

#define EQ_I_6 0.9679375002660838,-1.8154878706806026,0.9255236005634416,-1.8154878706806026,0.8934611008295252 // PK Fc 2200 Hz Gain -8 dB Q 4
#elif (USE_EQ == 3) // ksc75 v1
#define EQ_BASS 1.0067574393348622,-1.9877770063944398,0.9813444364702077,-1.9879030764989125,0.9879758057005973 // LS Fc 95 Hz Gain 13 dB
#define LIMIT_MUL ((dspfx)(1*(double)(1<<27))) // 1x > 0dB limit on output, 0-13 > -13dB limit on input
#define BASS_MUL floatfx(1./4.5) // Bass peak filter is 13dB > 4.5x

#define EQ_I_0 0.9903251622217966,-1.940368569186812,0.9667193853665432,-1.940368569186812,0.9570445475883398
#define EQ_I_1 0.9890959496073357,-1.87264262553866,0.9134389341593286,-1.87264262553866,0.9025348837666642
#define EQ_I_2 0.97767677971005,-1.7029506619072934,0.8304098502403209,-1.7029506619072934,0.8080866299503711
#define EQ_I_3 0.9540799496951968,-1.354934132236624,0.8678758843118544,-1.354934132236624,0.8219558340070512
#define EQ_I_4 0.9557291703411076,-0.8569915472352453,0.8252914343320109,-0.8569915472352453,0.7810206046731185
#define EQ_I_5 1.0361853565502714,-0.0294756164204319,0.7653120089679191,-0.0294756164204319,0.8014973655181906
#endif

#ifndef EQ_BASS
#define EQ_BASS 1.0103039413192074,-1.9969618889340095,0.9867280269044961,-1.9969618889340095,0.9970319682237037 // PK Fc 64 Hz Gain 18 dB Q 1
#endif

#ifndef LIMIT_MUL
#define LIMIT_MUL ((dspfx)(0.316*(double)(1<<27))) // 0.32x > -10dB limit on output, -10-18 > -28dB limit on input
#endif

#ifndef BASS_MUL
#define BASS_MUL floatfx(1./7.9) // Bass peak filter is 18dB > 8x
#endif

#ifndef EQ_I_0
#define EQ_I_0 1.0,0.0,0.0,0.0,0.0
#endif

#ifndef EQ_I_1
#define EQ_I_1 1.0,0.0,0.0,0.0,0.0
#endif

#ifndef EQ_I_2
#define EQ_I_2 1.0,0.0,0.0,0.0,0.0
#endif

#ifndef EQ_I_3
#define EQ_I_3 1.0,0.0,0.0,0.0,0.0
#endif

#ifndef EQ_I_4
#define EQ_I_4 1.0,0.0,0.0,0.0,0.0
#endif

#ifndef EQ_I_5
#define EQ_I_5 1.0,0.0,0.0,0.0,0.0
#endif

#ifndef EQ_I_6
#define EQ_I_6 1.0,0.0,0.0,0.0,0.0
#endif

#ifndef EQ_I_7
#define EQ_I_7 1.0,0.0,0.0,0.0,0.0
#endif

/*
#ifndef EQ_I_8
#define EQ_I_8 1.0,0.0,0.0,0.0,0.0
#endif

#ifndef EQ_I_9
#define EQ_I_9 1.0,0.0,0.0,0.0,0.0
#endif

#ifndef EQ_I_10
#define EQ_I_10 1.0,0.0,0.0,0.0,0.0
#endif

#ifndef EQ_I_11
#define EQ_I_11 1.0,0.0,0.0,0.0,0.0
#endif

#ifndef EQ_I_12
#define EQ_I_12 1.0,0.0,0.0,0.0,0.0
#endif

#ifndef EQ_I_13
#define EQ_I_13 1.0,0.0,0.0,0.0,0.0
#endif

#ifndef EQ_I_14
#define EQ_I_14 1.0,0.0,0.0,0.0,0.0
#endif

#ifndef EQ_I_15
#define EQ_I_15 1.0,0.0,0.0,0.0,0.0
#endif

#ifndef EQ_I_16
#define EQ_I_16 1.0,0.0,0.0,0.0,0.0
#endif

#ifndef EQ_I_17
#define EQ_I_17 1.0,0.0,0.0,0.0,0.0
#endif
*/

int32_t actual_vol = 0;
#define VOL_STEP 600000

dspfx limit[192]; // sample store
int limit_index = 0;
int32_t limit_vol = 0;
//#define LIMIT_MUL ((dspfx)(0.04*(double)(1<<30))) // 0.04x > -28dB limit relative to input (-28+18 > Stops limiting at -10dB)
//#define BASS_MUL floatfx(1./8.) // Bass peak filter is 18dB > 8x
int64_t targ = 0;
#define BASS_STEP 600000
#define BASS_STEP_DOWN 1200000

uint cur_alt = 1;

// todo make descriptor strings should probably belong to the configs
static char *descriptor_strings[] =
        {
                "sctanf",
                "Pico Amp" 
        };

// todo fix these
#define VENDOR_ID   0x2e8au
#define PRODUCT_ID  0xfeddu

#define AUDIO_OUT_ENDPOINT  0x01U
#define AUDIO_IN_ENDPOINT   0x82U

#undef AUDIO_SAMPLE_FREQ
#define AUDIO_SAMPLE_FREQ(frq) (uint8_t)(frq), (uint8_t)((frq >> 8)), (uint8_t)((frq >> 16))

#define AUDIO_MAX_PACKET_SIZE(freq) (((freq + 999) / 1000) * 8)
#define FEATURE_MUTE_CONTROL 1u
#define FEATURE_VOLUME_CONTROL 2u

#define ENDPOINT_FREQ_CONTROL 1u

struct audio_device_config {
    struct usb_configuration_descriptor descriptor;
    struct usb_interface_descriptor ac_interface;
    struct __packed {
        USB_Audio_StdDescriptor_Interface_AC_t core;
        USB_Audio_StdDescriptor_InputTerminal_t input_terminal;
        USB_Audio_StdDescriptor_FeatureUnit_t feature_unit;
        USB_Audio_StdDescriptor_OutputTerminal_t output_terminal;
    } ac_audio;
    struct usb_interface_descriptor as_zero_interface;

    struct usb_interface_descriptor as_op_interface;
    struct __packed {
        USB_Audio_StdDescriptor_Interface_AS_t streaming;
        struct __packed {
            USB_Audio_StdDescriptor_Format_t core;
            USB_Audio_SampleFreq_t freqs[1];
        } format;
    } as_audio;
    struct __packed {
        struct usb_endpoint_descriptor_long core;
        USB_Audio_StdDescriptor_StreamEndpoint_Spc_t audio;
    } ep1;
    struct usb_endpoint_descriptor_long ep2;

    struct usb_interface_descriptor as_op_interface3;
    struct __packed {
        USB_Audio_StdDescriptor_Interface_AS_t streaming;
        struct __packed {
            USB_Audio_StdDescriptor_Format_t core;
            USB_Audio_SampleFreq_t freqs[1];
        } format;
    } as_audio3;
    struct __packed {
        struct usb_endpoint_descriptor_long core;
        USB_Audio_StdDescriptor_StreamEndpoint_Spc_t audio;
    } ep13;
    struct usb_endpoint_descriptor_long ep23;

    struct usb_interface_descriptor as_op_interface2;
    struct __packed {
        USB_Audio_StdDescriptor_Interface_AS_t streaming;
        struct __packed {
            USB_Audio_StdDescriptor_Format_t core;
            USB_Audio_SampleFreq_t freqs[1];
        } format;
    } as_audio2;
    struct __packed {
        struct usb_endpoint_descriptor_long core;
        USB_Audio_StdDescriptor_StreamEndpoint_Spc_t audio;
    } ep12;
    struct usb_endpoint_descriptor_long ep22;
};

static const struct audio_device_config audio_device_config = {
        .descriptor = {
                .bLength             = sizeof(audio_device_config.descriptor),
                .bDescriptorType     = DTYPE_Configuration,
                .wTotalLength        = sizeof(audio_device_config),
                .bNumInterfaces      = 2,
                .bConfigurationValue = 0x01,
                .iConfiguration      = 0x00,
                .bmAttributes        = 0x80, // bus powered
                .bMaxPower           = 0xfa, // 500mA
        },
        .ac_interface = {
                .bLength            = sizeof(audio_device_config.ac_interface),
                .bDescriptorType    = DTYPE_Interface,
                .bInterfaceNumber   = 0x00,
                .bAlternateSetting  = 0x00,
                .bNumEndpoints      = 0x00,
                .bInterfaceClass    = AUDIO_CSCP_AudioClass,
                .bInterfaceSubClass = AUDIO_CSCP_ControlSubclass,
                .bInterfaceProtocol = AUDIO_CSCP_ControlProtocol,
                .iInterface         = 0x00,
        },
        .ac_audio = {
                .core = {
                        .bLength = sizeof(audio_device_config.ac_audio.core),
                        .bDescriptorType = AUDIO_DTYPE_CSInterface,
                        .bDescriptorSubtype = AUDIO_DSUBTYPE_CSInterface_Header,
                        .bcdADC = VERSION_BCD(1, 0, 0),
                        .wTotalLength = sizeof(audio_device_config.ac_audio),
                        .bInCollection = 1,
                        .bInterfaceNumbers = 1,
                },
                .input_terminal = {
                        .bLength = sizeof(audio_device_config.ac_audio.input_terminal),
                        .bDescriptorType = AUDIO_DTYPE_CSInterface,
                        .bDescriptorSubtype = AUDIO_DSUBTYPE_CSInterface_InputTerminal,
                        .bTerminalID = 1,
                        .wTerminalType = AUDIO_TERMINAL_STREAMING,
                        .bAssocTerminal = 0,
                        .bNrChannels = 2,
                        .wChannelConfig = AUDIO_CHANNEL_LEFT_FRONT | AUDIO_CHANNEL_RIGHT_FRONT,
                        .iChannelNames = 0,
                        .iTerminal = 0,
                },
                .feature_unit = {
                        .bLength = sizeof(audio_device_config.ac_audio.feature_unit),
                        .bDescriptorType = AUDIO_DTYPE_CSInterface,
                        .bDescriptorSubtype = AUDIO_DSUBTYPE_CSInterface_Feature,
                        .bUnitID = 2,
                        .bSourceID = 1,
                        .bControlSize = 1,
                        .bmaControls = {AUDIO_FEATURE_MUTE | AUDIO_FEATURE_VOLUME, 0, 0},
                        .iFeature = 0,
                },
                .output_terminal = {
                        .bLength = sizeof(audio_device_config.ac_audio.output_terminal),
                        .bDescriptorType = AUDIO_DTYPE_CSInterface,
                        .bDescriptorSubtype = AUDIO_DSUBTYPE_CSInterface_OutputTerminal,
                        .bTerminalID = 3,
                        .wTerminalType = AUDIO_TERMINAL_OUT_SPEAKER,
                        .bAssocTerminal = 0,
                        .bSourceID = 2,
                        .iTerminal = 0,
                },
        },

        .as_zero_interface = {
                .bLength            = sizeof(audio_device_config.as_zero_interface),
                .bDescriptorType    = DTYPE_Interface,
                .bInterfaceNumber   = 0x01,
                .bAlternateSetting  = 0x00,
                .bNumEndpoints      = 0x00,
                .bInterfaceClass    = AUDIO_CSCP_AudioClass,
                .bInterfaceSubClass = AUDIO_CSCP_AudioStreamingSubclass,
                .bInterfaceProtocol = AUDIO_CSCP_ControlProtocol,
                .iInterface         = 0x00,
        },

        .as_op_interface = {
                .bLength            = sizeof(audio_device_config.as_op_interface),
                .bDescriptorType    = DTYPE_Interface,
                .bInterfaceNumber   = 0x01,
                .bAlternateSetting  = 0x01,
                .bNumEndpoints      = 0x02,
                .bInterfaceClass    = AUDIO_CSCP_AudioClass,
                .bInterfaceSubClass = AUDIO_CSCP_AudioStreamingSubclass,
                .bInterfaceProtocol = AUDIO_CSCP_ControlProtocol,
                .iInterface         = 0x00,
        },
        .as_audio = {
                .streaming = {
                        .bLength = sizeof(audio_device_config.as_audio.streaming),
                        .bDescriptorType = AUDIO_DTYPE_CSInterface,
                        .bDescriptorSubtype = AUDIO_DSUBTYPE_CSInterface_General,
                        .bTerminalLink = 1,
                        .bDelay = 0,
                        .wFormatTag = 1, // PCM
                },
                .format = {
                        .core = {
                                .bLength = sizeof(audio_device_config.as_audio.format),
                                .bDescriptorType = AUDIO_DTYPE_CSInterface,
                                .bDescriptorSubtype = AUDIO_DSUBTYPE_CSInterface_FormatType,
                                .bFormatType = 1,
                                .bNrChannels = 2,
                                .bSubFrameSize = 4,
                                .bBitResolution = 32,
                                .bSampleFrequencyType = 1,
                        },
                        .freqs = {
                                AUDIO_SAMPLE_FREQ(48000)
                        },
                },
        },
        .ep1 = {
                .core = {
                        .bLength          = sizeof(audio_device_config.ep1.core),
                        .bDescriptorType  = DTYPE_Endpoint,
                        .bEndpointAddress = AUDIO_OUT_ENDPOINT,
                        .bmAttributes     = 13,
                        .wMaxPacketSize   = 384,
                        .bInterval        = 1,
                        .bRefresh         = 0,
                        .bSyncAddr        = 0,
                },
                .audio = {
                        .bLength = sizeof(audio_device_config.ep1.audio),
                        .bDescriptorType = AUDIO_DTYPE_CSEndpoint,
                        .bDescriptorSubtype = AUDIO_DSUBTYPE_CSEndpoint_General,
                        .bmAttributes = 0,
                        .bLockDelayUnits = 0,
                        .wLockDelay = 0,
                }
        },
        .ep2 = {
                .bLength          = sizeof(audio_device_config.ep2),
                .bDescriptorType  = 0x05,
                .bEndpointAddress = AUDIO_IN_ENDPOINT,
                .bmAttributes     = 0x11,
                .wMaxPacketSize   = 3,
                .bInterval        = 0x01,
                .bRefresh         = 2,
                .bSyncAddr        = 0,
        },

        .as_op_interface3 = {
                .bLength            = sizeof(audio_device_config.as_op_interface),
                .bDescriptorType    = DTYPE_Interface,
                .bInterfaceNumber   = 0x01,
                .bAlternateSetting  = 0x02,
                .bNumEndpoints      = 0x02,
                .bInterfaceClass    = AUDIO_CSCP_AudioClass,
                .bInterfaceSubClass = AUDIO_CSCP_AudioStreamingSubclass,
                .bInterfaceProtocol = AUDIO_CSCP_ControlProtocol,
                .iInterface         = 0x00,
        },
        .as_audio3 = {
                .streaming = {
                        .bLength = sizeof(audio_device_config.as_audio.streaming),
                        .bDescriptorType = AUDIO_DTYPE_CSInterface,
                        .bDescriptorSubtype = AUDIO_DSUBTYPE_CSInterface_General,
                        .bTerminalLink = 1,
                        .bDelay = 0,
                        .wFormatTag = 1, // PCM
                },
                .format = {
                        .core = {
                                .bLength = sizeof(audio_device_config.as_audio.format),
                                .bDescriptorType = AUDIO_DTYPE_CSInterface,
                                .bDescriptorSubtype = AUDIO_DSUBTYPE_CSInterface_FormatType,
                                .bFormatType = 1,
                                .bNrChannels = 2,
                                .bSubFrameSize = 3,
                                .bBitResolution = 24,
                                .bSampleFrequencyType = 1,
                        },
                        .freqs = {
                                AUDIO_SAMPLE_FREQ(48000)
                        },
                },
        },
        .ep13 = {
                .core = {
                        .bLength          = sizeof(audio_device_config.ep1.core),
                        .bDescriptorType  = DTYPE_Endpoint,
                        .bEndpointAddress = AUDIO_OUT_ENDPOINT,
                        .bmAttributes     = 13,
                        .wMaxPacketSize   = 288,
                        .bInterval        = 1,
                        .bRefresh         = 0,
                        .bSyncAddr        = 0,
                },
                .audio = {
                        .bLength = sizeof(audio_device_config.ep1.audio),
                        .bDescriptorType = AUDIO_DTYPE_CSEndpoint,
                        .bDescriptorSubtype = AUDIO_DSUBTYPE_CSEndpoint_General,
                        .bmAttributes = 0,
                        .bLockDelayUnits = 0,
                        .wLockDelay = 0,
                }
        },
        .ep23 = {
                .bLength          = sizeof(audio_device_config.ep2),
                .bDescriptorType  = 0x05,
                .bEndpointAddress = AUDIO_IN_ENDPOINT,
                .bmAttributes     = 0x11,
                .wMaxPacketSize   = 3,
                .bInterval        = 0x01,
                .bRefresh         = 2,
                .bSyncAddr        = 0,
        },

        .as_op_interface2 = {
                .bLength            = sizeof(audio_device_config.as_op_interface),
                .bDescriptorType    = DTYPE_Interface,
                .bInterfaceNumber   = 0x01,
                .bAlternateSetting  = 0x03,
                .bNumEndpoints      = 0x02,
                .bInterfaceClass    = AUDIO_CSCP_AudioClass,
                .bInterfaceSubClass = AUDIO_CSCP_AudioStreamingSubclass,
                .bInterfaceProtocol = AUDIO_CSCP_ControlProtocol,
                .iInterface         = 0x00,
        },
        .as_audio2 = {
                .streaming = {
                        .bLength = sizeof(audio_device_config.as_audio.streaming),
                        .bDescriptorType = AUDIO_DTYPE_CSInterface,
                        .bDescriptorSubtype = AUDIO_DSUBTYPE_CSInterface_General,
                        .bTerminalLink = 1,
                        .bDelay = 0,
                        .wFormatTag = 1, // PCM
                },
                .format = {
                        .core = {
                                .bLength = sizeof(audio_device_config.as_audio.format),
                                .bDescriptorType = AUDIO_DTYPE_CSInterface,
                                .bDescriptorSubtype = AUDIO_DSUBTYPE_CSInterface_FormatType,
                                .bFormatType = 1,
                                .bNrChannels = 2,
                                .bSubFrameSize = 2,
                                .bBitResolution = 16,
                                .bSampleFrequencyType = 1,
                        },
                        .freqs = {
                                AUDIO_SAMPLE_FREQ(48000)
                        },
                },
        },
        .ep12 = {
                .core = {
                        .bLength          = sizeof(audio_device_config.ep1.core),
                        .bDescriptorType  = DTYPE_Endpoint,
                        .bEndpointAddress = AUDIO_OUT_ENDPOINT,
                        .bmAttributes     = 13,
                        .wMaxPacketSize   = 192,
                        .bInterval        = 1,
                        .bRefresh         = 0,
                        .bSyncAddr        = 0,
                },
                .audio = {
                        .bLength = sizeof(audio_device_config.ep1.audio),
                        .bDescriptorType = AUDIO_DTYPE_CSEndpoint,
                        .bDescriptorSubtype = AUDIO_DSUBTYPE_CSEndpoint_General,
                        .bmAttributes = 0,
                        .bLockDelayUnits = 0,
                        .wLockDelay = 0,
                }
        },
        .ep22 = {
                .bLength          = sizeof(audio_device_config.ep2),
                .bDescriptorType  = 0x05,
                .bEndpointAddress = AUDIO_IN_ENDPOINT,
                .bmAttributes     = 0x11,
                .wMaxPacketSize   = 3,
                .bInterval        = 0x01,
                .bRefresh         = 2,
                .bSyncAddr        = 0,
        },
};

static struct usb_interface ac_interface;
static struct usb_interface as_op_interface;
static struct usb_endpoint ep_op_out, ep_op_sync;

static const struct usb_device_descriptor boot_device_descriptor = {
        .bLength            = 18,
        .bDescriptorType    = 0x01,
        .bcdUSB             = 0x0110, // 1.1
        .bDeviceClass       = 0x00,
        .bDeviceSubClass    = 0x00,
        .bDeviceProtocol    = 0x00,
        .bMaxPacketSize0    = 0x40,
        .idVendor           = VENDOR_ID,
        .idProduct          = PRODUCT_ID,
        .bcdDevice          = 0x0200,
        .iManufacturer      = 0x01,
        .iProduct           = 0x02,
        .iSerialNumber      = 0x00,
        .bNumConfigurations = 0x01,
};

const char *_get_descriptor_string(uint index) {
    if (index <= count_of(descriptor_strings)) {
        return descriptor_strings[index - 1];
    } else {
        return "";
    }
}

static struct {
    uint32_t freq;
    int16_t volume;
    int32_t vol_mul;
    bool mute;
} audio_state = {
        .freq = 48000,
};

uint64_t times[1024];
int timei = 0;
static void __not_in_flash_func(_as_audio_packet)(struct usb_endpoint *ep) { // total 733 us + 87 us
    uint64_t now_time = time_us_64();

    struct usb_buffer *usb_buffer = usb_current_out_packet_buffer(ep);
    int16_t count;
    int32_t vol_mul = audio_state.mute ? 0 : audio_state.vol_mul;

    switch (cur_alt)
    {
    case 1: // 32bit
        count = (usb_buffer->data_len + 256) / 8; // the packet size is overflowing!!!
        {
            int32_t *in = (int32_t *) usb_buffer->data;
            for (int i = 0; i < count * 2; i++) {
                buf0[i] = in[i] >> 1; // divide by 2, allow some headroom in case the filters need it
            }
        }
        break;
    case 2: // 24bit
        count = (usb_buffer->data_len + 256) / 6; // the packet size is overflowing!!!
        {
            uint8_t *in = (uint8_t *) usb_buffer->data;
            for (int i = 0; i < count * 2; i += 2) {
                int j = i * 3;
                buf0[i] = (in[j] | in[j+1] << 8 | (int8_t)in[j+2] << 16) << 7; // divide by 2, allow some headroom in case the filters need it
                buf0[i+1] = (in[j+3] | in[j+4] << 8 | (int8_t)in[j+5] << 16) << 7; // divide by 2, allow some headroom in case the filters need it
            }
        }
        break;
    case 3: // 16bit
        count = usb_buffer->data_len / 4;
        {
            int16_t *in = (int16_t *) usb_buffer->data;
            for (int i = 0; i < count * 2; i++) {
                buf0[i] = in[i] << 15; // divide by 2, allow some headroom in case the filters need it
            }
        }
        break;
    default: // No alternate mode enumerated :(
        count = 0;
        break;
    }
    
    usb_grow_transfer(ep->current_transfer, 1);
    usb_packet_done(ep);
    // 8 us elapsed
#ifdef EQ_ENABLE // 521 us + 87 us
    process_biquad(&eq_bq_1, biquadconstsfx(EQ_I_0), count, buf0, buf1); // 87 us each
    process_biquad(&eq_bq_2, biquadconstsfx(EQ_I_1), count, buf1, buf0);
    process_biquad(&eq_bq_3, biquadconstsfx(EQ_I_2), count, buf0, buf1);
    process_biquad(&eq_bq_4, biquadconstsfx(EQ_I_3), count, buf1, buf0);
    process_biquad(&eq_bq_5, biquadconstsfx(EQ_I_4), count, buf0, buf1);
    process_biquad(&eq_bq_6, biquadconstsfx(EQ_I_5), count, buf1, buf0);
    process_biquad(&eq_bq_7, biquadconstsfx(EQ_I_6), count, buf0, buf1);
    process_biquad(&eq_bq_8, biquadconstsfx(EQ_I_7), count, buf1, buf0);
/*
    process_biquad(&eq_bq_9, biquadconstsfx(EQ_I_8), count, buf0, buf1);
    process_biquad(&eq_bq_10, biquadconstsfx(EQ_I_9), count, buf1, buf0);
    process_biquad(&eq_bq_11, biquadconstsfx(EQ_I_10), count, buf0, buf1);
    process_biquad(&eq_bq_12, biquadconstsfx(EQ_I_11), count, buf1, buf0);
    process_biquad(&eq_bq_13, biquadconstsfx(EQ_I_12), count, buf0, buf1);
    process_biquad(&eq_bq_14, biquadconstsfx(EQ_I_13), count, buf1, buf0);
    process_biquad(&eq_bq_15, biquadconstsfx(EQ_I_14), count, buf0, buf1);
    process_biquad(&eq_bq_16, biquadconstsfx(EQ_I_15), count, buf1, buf0);
    process_biquad(&eq_bq_17, biquadconstsfx(EQ_I_16), count, buf0, buf1);
    process_biquad(&eq_bq_18, biquadconstsfx(EQ_I_17), count, buf1, buf0);
*/
#endif
    for (int i = 0; i < count * 2; i += 2) { // 25 us
        if (actual_vol - VOL_STEP > vol_mul) actual_vol -= VOL_STEP;
        else if (actual_vol < vol_mul - VOL_STEP) actual_vol += VOL_STEP;
        else actual_vol = vol_mul;
        buf0[i] = mulfx2(buf0[i], actual_vol);
        buf0[i+1] = mulfx2(buf0[i+1], actual_vol);
    }
#ifdef POWER_LIMIT
    for (int i = 0; i < count * 2; i++) { // 5 us
        buf0[i] = buf0[i] >> POWER_LIMIT;
    }
#endif
#ifdef BASS_ENABLE
    for (int i = 0; i < count * 2; i++) { // 5 us
        buf0[i] = buf0[i] >> 3; // divide by 8, headroom for bass eq
    }
    
    process_biquad(&eq_bq_0, biquadconstsfx(EQ_BASS), count, buf0, buf1); // 87 us

    limit[limit_index] = fxabs(buf1[0]);
    if (limit[limit_index]>0) limit_index++;
    limit_index %= 192;
    limit[limit_index] = fxabs(buf1[1]);
    if (limit[limit_index]>0) limit_index++;
    limit_index %= 192;
    dspfx max = 0;
    for (int i = 0; i < 192; i++) // 8 us
    {
        if (limit[i]>max) max = limit[i];
    }
//    if (max < 15000) return;
    if (max < LIMIT_MUL) max = LIMIT_MUL;
    int64_t actualtarg = (int64_t)(LIMIT_MUL - mulfx(max, BASS_MUL)) * (int64_t)(1<<30) / (int64_t)(max - mulfx(max, BASS_MUL)); // target mix to limit bass
    if (actualtarg < 0) actualtarg = 0; // if the mix goes negative, ignore it
    
    for (int i = 0; i < count * 2; i += 2) { // 61 us
        if (targ - BASS_STEP_DOWN > actualtarg) targ -= BASS_STEP_DOWN;
        else if (targ < actualtarg - BASS_STEP) targ += BASS_STEP;
        else targ = actualtarg;
        buf0[i] = (mulfx2(buf0[i], (1<<30) - targ) + mulfx2(buf1[i], targ)) << 3;
        if (buf0[i] > (1<<30) - 1) buf0[i] = (1<<30) - 1;
        if (buf0[i] < (-1<<30)) buf0[i] = (-1<<30);
        buf0[i+1] = (mulfx2(buf0[i+1], (1<<30) - targ) + mulfx2(buf1[i+1], targ)) << 3;
        if (buf0[i+1] > (1<<30) - 1) buf0[i+1] = (1<<30) - 1;
        if (buf0[i+1] < (-1<<30)) buf0[i+1] = (-1<<30);
    }
#endif
//    while (bufring1.len > 1024-32-2-(count * 2)) {tight_loop_contents();}
mutex_enter_blocking(&bufring1.corelock2);
//while (bufring1.corelock == 2) {tight_loop_contents();}
//bufring1.corelock = 1; // 20us
    audioi2sconstuff2();
    int curin = bufring1.index;
    for (int i = 0; i < count * 2; i++) {
        bufring1.buf[curin] = buf0[i]<<1;
        if (curin < 32) {
            bufring1.buf[curin+1024-32] = bufring1.buf[curin];
        }
        curin++;
        curin %= 1024-32;
    }
    bufring1.len = bufring1.len + count * 2;
    bufring1.index = (bufring1.index + count * 2) % (1024-32);
//bufring1.corelock = 0;
mutex_exit(&bufring1.corelock2);
    times[timei] = time_us_64() - now_time;
    timei++;
    timei %= 1024;
}

static void _as_sync_packet(struct usb_endpoint *ep) {
    struct usb_buffer *buffer = usb_current_in_packet_buffer(ep);
    buffer->data_len = 3;

    int64_t feedback_buf = (double)((audio_state.freq) * (double)(1u << 14u));

    uint feedback = feedback_buf / 1000u; // lie

    buffer->data[0] = feedback;
    buffer->data[1] = feedback >> 8u;
    buffer->data[2] = feedback >> 16u;

    usb_grow_transfer(ep->current_transfer, 1);
    usb_packet_done(ep);
}

static const struct usb_transfer_type as_transfer_type = {
        .on_packet = _as_audio_packet,
        .initial_packet_count = 1,
};

static const struct usb_transfer_type as_sync_transfer_type = {
        .on_packet = _as_sync_packet,
        .initial_packet_count = 1,
};

static struct usb_transfer as_transfer;
static struct usb_transfer as_sync_transfer;

static bool do_get_current(struct usb_setup_packet *setup) {
    usb_debug("AUDIO_REQ_GET_CUR\n");

    if ((setup->bmRequestType & USB_REQ_TYPE_RECIPIENT_MASK) == USB_REQ_TYPE_RECIPIENT_INTERFACE) {
        switch (setup->wValue >> 8u) {
            case FEATURE_MUTE_CONTROL: {
                usb_start_tiny_control_in_transfer(audio_state.mute, 1);
                return true;
            }
            case FEATURE_VOLUME_CONTROL: {
                /* Current volume. See UAC Spec 1.0 p.77 */
                usb_start_tiny_control_in_transfer(audio_state.volume, 2);
                return true;
            }
        }
    } else if ((setup->bmRequestType & USB_REQ_TYPE_RECIPIENT_MASK) == USB_REQ_TYPE_RECIPIENT_ENDPOINT) {
        if ((setup->wValue >> 8u) == ENDPOINT_FREQ_CONTROL) {
            /* Current frequency */
            usb_start_tiny_control_in_transfer(audio_state.freq, 3);
            return true;
        }
    }
    return false;
}

// better
int32_t db_to_vol_hp[100] = {0x40000000,0x390a415f,0x32d64617,0x2d4efbd5,0x28619ae9,0x23fd6678,0x2013739e,0x1c9676c6,0x197a967f,0x16b54337,0x143d1362,0x1209a37a,0x10137987,0xe53ebb3,0xcc509ab,0xb618871,0xa24b062,0x90a4d2f,0x80e9f96,0x72e50a6,0x6666666,0x5b439bc,0x5156d68,0x487e5fb,0x409c2b0,0x399570c,0x3352529,0x2dbd8ad,0x28c423f,0x2455385,0x2061b89,0x1cdc38c,0x19b8c27,0x16ecac5,0x146e75d,0x1235a71,0x103ab3d,0xe76e1e,0xce4328,0xb7d4dd,0xa3d70a,0x9205c6,0x82248a,0x73fd65,0x676044,0x5c224e,0x521d50,0x492f44,0x4139d3,0x3a21f3,0x33cf8d,0x2e2d27,0x29279d,0x24ade0,0x20b0bc,0x1d22a4,0x19f786,0x17249c,0x14a050,0x126216,0x10624d,0xe9a2d,0xd03a7,0xb9956,0xa566d,0x936a1,0x83621,0x75186,0x685c8,0x5d031,0x52e5a,0x49e1d,0x41d8f,0x3aafc,0x344df,0x2e9dd,0x298c0,0x25076,0x21008,0x1d69b,0x1a36e,0x175d1,0x14d2a,0x128ef,0x108a4,0xebdc,0xd236,0xbb5a,0xa6fa,0x94d1,0x84a2,0x7636,0x695b,0x5de6,0x53af,0x4a96,0x4279,0x3b3f,0x34cd,0x2f0f};

// actually windows doesn't seem to like this in the middle, so set top range to 0db
#define CENTER_VOLUME_INDEX 91

#define ENCODE_DB(x) ((uint16_t)(int16_t)((x)*256))

#define MIN_VOLUME           ENCODE_DB(-CENTER_VOLUME_INDEX)
#define DEFAULT_VOLUME       -6<<8 // -6dB
#define MAX_VOLUME           0
#define VOLUME_RESOLUTION    1

static bool do_get_minimum(struct usb_setup_packet *setup) {
    usb_debug("AUDIO_REQ_GET_MIN\n");
    if ((setup->bmRequestType & USB_REQ_TYPE_RECIPIENT_MASK) == USB_REQ_TYPE_RECIPIENT_INTERFACE) {
        switch (setup->wValue >> 8u) {
            case FEATURE_VOLUME_CONTROL: {
                usb_start_tiny_control_in_transfer(MIN_VOLUME, 2);
                return true;
            }
        }
    }
    return false;
}

static bool do_get_maximum(struct usb_setup_packet *setup) {
    usb_debug("AUDIO_REQ_GET_MAX\n");
    if ((setup->bmRequestType & USB_REQ_TYPE_RECIPIENT_MASK) == USB_REQ_TYPE_RECIPIENT_INTERFACE) {
        switch (setup->wValue >> 8u) {
            case FEATURE_VOLUME_CONTROL: {
                usb_start_tiny_control_in_transfer(MAX_VOLUME, 2);
                return true;
            }
        }
    }
    return false;
}

static bool do_get_resolution(struct usb_setup_packet *setup) {
    usb_debug("AUDIO_REQ_GET_RES\n");
    if ((setup->bmRequestType & USB_REQ_TYPE_RECIPIENT_MASK) == USB_REQ_TYPE_RECIPIENT_INTERFACE) {
        switch (setup->wValue >> 8u) {
            case FEATURE_VOLUME_CONTROL: {
                usb_start_tiny_control_in_transfer(VOLUME_RESOLUTION, 2);
                return true;
            }
        }
    }
    return false;
}

static struct audio_control_cmd {
    uint8_t cmd;
    uint8_t type;
    uint8_t cs;
    uint8_t cn;
    uint8_t unit;
    uint8_t len;
} audio_control_cmd_t;

static void _audio_reconfigure() {
    audio_state.freq = 48000; // only support 48kHz anyway
}

static void audio_set_volume(int16_t volume) {
    audio_state.volume = volume;
    int16_t pos = fxabs(volume)/256;
    int16_t mix = fxabs(volume)%256; // mix between two steps
    if (pos > 92) {
        audio_state.vol_mul = 0;
    }
    else if (pos >= 91) {
        audio_state.vol_mul = (db_to_vol_hp[91] * (256 - mix)) >> 8;
    }
    else {
        audio_state.vol_mul = (((int64_t)db_to_vol_hp[pos] * (256 - mix)) + ((int64_t)db_to_vol_hp[pos+1] * mix)) >> 8;
    }
}

static void audio_cmd_packet(struct usb_endpoint *ep) {
    assert(audio_control_cmd_t.cmd == AUDIO_REQ_SetCurrent);
    struct usb_buffer *buffer = usb_current_out_packet_buffer(ep);
    audio_control_cmd_t.cmd = 0;
    if (buffer->data_len >= audio_control_cmd_t.len) {
        if (audio_control_cmd_t.type == USB_REQ_TYPE_RECIPIENT_INTERFACE) {
            switch (audio_control_cmd_t.cs) {
                case FEATURE_MUTE_CONTROL: {
                    audio_state.mute = buffer->data[0];
                    usb_warn("Set Mute %d\n", buffer->data[0]);
                    break;
                }
                case FEATURE_VOLUME_CONTROL: {
                    audio_set_volume(*(int16_t *) buffer->data);
                    break;
                }
            }

        } else if (audio_control_cmd_t.type == USB_REQ_TYPE_RECIPIENT_ENDPOINT) {
            if (audio_control_cmd_t.cs == ENDPOINT_FREQ_CONTROL) {
                uint32_t new_freq = (*(uint32_t *) buffer->data) & 0x00ffffffu;
                usb_warn("Set freq %d\n", new_freq == 0xffffffu ? -1 : (int) new_freq);

                if (audio_state.freq != new_freq) {
                    audio_state.freq = new_freq;
                    _audio_reconfigure();
                }
            }
        }
    }
    usb_start_empty_control_in_transfer_null_completion();
    // todo is there error handling?
}

static const struct usb_transfer_type _audio_cmd_transfer_type = {
        .on_packet = audio_cmd_packet,
        .initial_packet_count = 1,
};

static bool as_set_alternate(struct usb_interface *interface, uint alt) {
    assert(interface == &as_op_interface);
    usb_warn("SET ALTERNATE %d\n", alt);
    if (alt < 4) cur_alt = alt;
    return alt < 4;
}

static bool do_set_current(struct usb_setup_packet *setup) {
#ifndef NDEBUG
    usb_warn("AUDIO_REQ_SET_CUR\n");
#endif

    if (setup->wLength && setup->wLength < 64) {
        audio_control_cmd_t.cmd = AUDIO_REQ_SetCurrent;
        audio_control_cmd_t.type = setup->bmRequestType & USB_REQ_TYPE_RECIPIENT_MASK;
        audio_control_cmd_t.len = (uint8_t) setup->wLength;
        audio_control_cmd_t.unit = setup->wIndex >> 8u;
        audio_control_cmd_t.cs = setup->wValue >> 8u;
        audio_control_cmd_t.cn = (uint8_t) setup->wValue;
        usb_start_control_out_transfer(&_audio_cmd_transfer_type);
        return true;
    }
    return false;
}

static bool ac_setup_request_handler(__unused struct usb_interface *interface, struct usb_setup_packet *setup) {
    setup = __builtin_assume_aligned(setup, 4);
    if (USB_REQ_TYPE_TYPE_CLASS == (setup->bmRequestType & USB_REQ_TYPE_TYPE_MASK)) {
        switch (setup->bRequest) {
            case AUDIO_REQ_SetCurrent:
                return do_set_current(setup);

            case AUDIO_REQ_GetCurrent:
                return do_get_current(setup);

            case AUDIO_REQ_GetMinimum:
                return do_get_minimum(setup);

            case AUDIO_REQ_GetMaximum:
                return do_get_maximum(setup);

            case AUDIO_REQ_GetResolution:
                return do_get_resolution(setup);

            default:
                break;
        }
    }
    return false;
}

bool _as_setup_request_handler(__unused struct usb_endpoint *ep, struct usb_setup_packet *setup) {
    setup = __builtin_assume_aligned(setup, 4);
    if (USB_REQ_TYPE_TYPE_CLASS == (setup->bmRequestType & USB_REQ_TYPE_TYPE_MASK)) {
        switch (setup->bRequest) {
            case AUDIO_REQ_SetCurrent:
                return do_set_current(setup);

            case AUDIO_REQ_GetCurrent:
                return do_get_current(setup);

            case AUDIO_REQ_GetMinimum:
                return do_get_minimum(setup);

            case AUDIO_REQ_GetMaximum:
                return do_get_maximum(setup);

            case AUDIO_REQ_GetResolution:
                return do_get_resolution(setup);

            default:
                break;
        }
    }
    return false;
}

void usb_sound_card_init() {
    //msd_interface.setup_request_handler = msd_setup_request_handler;
    usb_interface_init(&ac_interface, &audio_device_config.ac_interface, NULL, 0, true);
    ac_interface.setup_request_handler = ac_setup_request_handler;

    static struct usb_endpoint *const op_endpoints[] = {
            &ep_op_out, &ep_op_sync
    };
    usb_interface_init(&as_op_interface, &audio_device_config.as_op_interface, op_endpoints, count_of(op_endpoints),
                       true);
    as_op_interface.set_alternate_handler = as_set_alternate;
    ep_op_out.setup_request_handler = _as_setup_request_handler;
    as_transfer.type = &as_transfer_type;
    usb_set_default_transfer(&ep_op_out, &as_transfer);
    as_sync_transfer.type = &as_sync_transfer_type;
    usb_set_default_transfer(&ep_op_sync, &as_sync_transfer);

    static struct usb_interface *const boot_device_interfaces[] = {
            &ac_interface,
            &as_op_interface,
    };
    __unused struct usb_device *device = usb_device_init(&boot_device_descriptor, &audio_device_config.descriptor,
                                                         boot_device_interfaces, count_of(boot_device_interfaces),
                                                         _get_descriptor_string);
    assert(device);
    audio_set_volume(DEFAULT_VOLUME);
    _audio_reconfigure();
//    device->on_configure = _on_configure;
    usb_device_start();
}

static void __not_in_flash_func(core1_worker)() {
    audio_i2s_set_enabled(true);
}

// https://github.com/jasongaunt/rp2040-bootsel-reboot-example/
#include "hardware/structs/ioqspi.h"
#include "hardware/watchdog.h"

bool __no_inline_not_in_flash_func(get_bootsel_button)() {
	const uint CS_PIN_INDEX = 1;

	// Must disable interrupts, as interrupt handlers may be in flash, and we
	// are about to temporarily disable flash access!
	uint32_t flags = save_and_disable_interrupts();

	// Set chip select to Hi-Z
	hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
		GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
		IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

	// Note we can't call into any sleep functions in flash right now
	for (volatile int i = 0; i < 1000; ++i);

	// The HI GPIO registers in SIO can observe and control the 6 QSPI pins.
	// Note the button pulls the pin *low* when pressed.
	bool button_state = !(sio_hw->gpio_hi_in & (1u << CS_PIN_INDEX));

	// Need to restore the state of chip select, else we are going to have a
	// bad time when we return to code in flash!
	hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
		GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
		IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);

	restore_interrupts(flags);

	return button_state;
}

void arm_watchdog() {
	watchdog_enable(500, 1);
}

void update_watchdog() {
	watchdog_update();
}

int main(void) {
    set_sys_clock_khz(270000, true); // so i can fit more filters

    // MG - slowing down clock to 250MHz brings SPI CLK within flash spec at nominal SPI_CLKDIV of 2
    // Loses some performance though :/
    // set_sys_clock_khz(250000, true); 

mutex_init(&bufring1.corelock2);
audioi2sconstuff(&bufring1);

    // initialize for 48k
    struct audio_format audio_format_48k = {
            .format = AUDIO_BUFFER_FORMAT_PCM_S16,
            .sample_freq = 48000,
            .channel_count = 2,
    };

    struct audio_i2s_config config = {
            .data_pin = PICO_AUDIO_I2S_DATA_PIN,
            .clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE,
            .dma_channel = 0,
            .pio_sm = 0,
    };

    audio_i2s_setup(&audio_format_48k, &config);

    usb_sound_card_init();

    multicore_launch_core1(core1_worker);
    // MSD is irq driven
    arm_watchdog();
    while (1) {
        __wfi();
        if (get_bootsel_button())
            while(1);
        update_watchdog();
    }
}
