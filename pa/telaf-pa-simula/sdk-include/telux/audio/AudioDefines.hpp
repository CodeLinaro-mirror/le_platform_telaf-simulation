/*
*  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are
*  met:
*    * Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the following disclaimer.
*    * Redistributions in binary form must reproduce the above
*      copyright notice, this list of conditions and the following
*      disclaimer in the documentation and/or other materials provided
*      with the distribution.
*    * Neither the name of The Linux Foundation nor the names of its
*      contributors may be used to endorse or promote products derived
*      from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
*  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
*  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
*  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
*  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
*  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
*  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
*  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/*
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file  AudioDefines.hpp
 * @brief Defines various enumerations and data types used with telsdk
 *        audio APIs.
 */

#ifndef TELUX_AUDIO_AUDIODEFINES_HPP
#define TELUX_AUDIO_AUDIODEFINES_HPP

#include <cstdint>
#include <cstring>
#include <vector>

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace audio {

/** Specifies that the DTMF tone should be played indefinitely */
const uint16_t INFINITE_DTMF_DURATION = 0xFFFF;

/** Specifies that the audio tone should be played indefinitely */
const uint16_t INFINITE_TONE_DURATION = 0xFFFF;

/** @addtogroup telematics_audio_manager
 * @{ */

/**
 * Represents an audio device. Each device is mapped to its corresponding
 * platform specific audio device type. This mapping is done in tel.conf file by the system
 * integrator. Refer to README specified for the tel.conf file for details on the mapping of
 * the DeviceType to a specific device on the HW platform.
 */
enum DeviceType {
    /** Default device (invalid) */
    DEVICE_TYPE_NONE = -1,
    /** Sink device as per above mapping */
    DEVICE_TYPE_SPEAKER = 1,
    /** Sink device as per above mapping */
    DEVICE_TYPE_SPEAKER_2 = 2,
    /** Sink device as per above mapping */
    DEVICE_TYPE_SPEAKER_3 = 3,
    /** Bluetooth sink device for voice call */
    DEVICE_TYPE_BT_SCO_SPEAKER = 4,
    /** Virtual sink device as per above mapping */
    DEVICE_TYPE_PROXY_SPEAKER = 5,
    /** Source device as per above mapping */
    DEVICE_TYPE_MIC = 257,
    /** Source device as per above mapping */
    DEVICE_TYPE_MIC_2 = 258,
    /** Source device as per above mapping */
    DEVICE_TYPE_MIC_3 = 259,
    /** Bluetooth source device for voice call */
    DEVICE_TYPE_BT_SCO_MIC = 260,
    /** Virtual mic connected over ethernet */
    DEVICE_TYPE_PROXY_MIC = 261,
};

/**
 *  Defines the direction of an audio device.
 */
enum class DeviceDirection {

    /** Default direction (invalid) */
    NONE = -1,

    /** Audio will go out of the device, for example through a speaker (sink) */
    RX = 1,

    /** Audio will come into the device, for example through a mic (source) */
    TX = 2,
};

/**
 *  Defines the type of the audio stream and the type's purpose.
 */
enum class StreamType {

    /** Default type (invalid) */
    NONE = -1,

    /** Used for audio over a cellular network */
    VOICE_CALL = 1,

    /** Used for playing audio, for example playing music and notifications */
    PLAY = 2,

    /** Used for capturing audio, for example recording sound using a mic */
    CAPTURE = 3,

    /** Used for generating audio from a @ref DeviceDirection::RX device, which
     *  is intended to be captured back by a @ref DeviceDirection::TX device */
    LOOPBACK = 4,

    /** Used for single tone and DTMF tone generation */
    TONE_GENERATOR = 5,
};

/**
 *  Defines the direction of an audio stream.
 */
enum class StreamDirection {

    /** Default direction (invalid) */
    NONE = -1,

    /** Specifies that the audio data will flow towards a sink device */
    RX = 1,

    /** Specifies that the audio data originates from a source device */
    TX = 2,
};

/** @} */ /* end_addtogroup telematics_audio_manager */

/** @addtogroup telematics_audio_stream
 * @{ */

/**
 *  Used for an in-call playback/capture and HPCM use cases. Represents
 *  the direction of the audio data flow.
 */
enum class Direction {

    /** Indicates voice downlink path (cellular network to a device) */
    RX = 1,

    /** Indicates voice uplink path (device to a cellular network) */
    TX = 2,
};

/**
 *  Adds positional perspective to the audio data in a given audio frame.
 *  For example, in a 2-speaker audio system, ChannelType::LEFT may represent
 *  audio played on speaker-1 while ChannelType::RIGHT represents audio played
 *  on speaker-2.
 */
enum ChannelType {

    /** Specifies the left channel */
    LEFT = (1 << 0),

    /** Specifies the right channel */
    RIGHT = (1 << 1),
};

/**
 *  Describes the arrangment of audio samples in a given audio frame through
 *  @ref ChannelType.
 */
using ChannelTypeMask = int;

/**
 *  Specifies how audio data is represented (for example, endianness and
 *  number of bits) for storage or exchanging among various audio software
 *  and hardware layers.
 */
enum class AudioFormat {

    /** Default format (invalid) */
    UNKNOWN = -1,

    /** PCM signed 16 bits */
    PCM_16BIT_SIGNED = 1,

    /** Adaptive multirate narrow band format */
    AMRNB = 20,

    /** Adaptive multirate wide band format */
    AMRWB,

    /** Extended adaptive multirate wide band format */
    AMRWB_PLUS,
};

/**
 *  When generating a DTMF tone, defines the value of the low frequency component.
 */
enum class DtmfLowFreq {
    /** 697 Hz */
    FREQ_697 = 697,
    /** 770 Hz */
    FREQ_770 = 770,
    /** 852 Hz */
    FREQ_852 = 852,
    /** 941 Hz */
    FREQ_941 = 941
};

/**
 *  When generating a DTMF tone, defines the value of the high frequency component.
 */
enum class DtmfHighFreq {
    /** 1209 Hz */
    FREQ_1209 = 1209,
    /** 1336 Hz */
    FREQ_1336 = 1336,
    /** 1477 Hz */
    FREQ_1477 = 1477,
    /** 1633 Hz */
    FREQ_1633 = 1633
};

/**
 *  Defines the behavior for how a compressed audio format playback should be finished.
 */
enum class StopType {

    /** Stop playing immediately and discard all pending audio samples */
    FORCE_STOP,

    /** Stop playing after all samples in the pipeline have been played */
    STOP_AFTER_PLAY,
};

/** @} */ /* end_addtogroup telematics_audio_stream */

/** @addtogroup telematics_audio_manager
 * @{ */

/**
 *  Defines the properties of the audio data for compressed playback and transcoding.
 */
enum class AmrwbpFrameFormat {

    /** Default format (invalid) */
    UNKNOWN = -1,

    /** Unsupported */
    TRANSPORT_INTERFACE_FORMAT,

    /** Specifies that the AMR header has been stripped from the audio data sent */
    FILE_STORAGE_FORMAT,
};

/**
 *  On a voice call stream, enables or disables echo cancellation and noise reduction (ECNR).
 *  Used with an audio device capable of supporting ECNR.
 */
enum class EcnrMode {

    /** Disables ECNR */
    DISABLE = 0,

    /** Enables ECNR */
    ENABLE = 1,
};

/**
 *  Represents the state of the platform calibration for audio.
 */
enum class CalibrationInitStatus {

    /** Default state */
    UNKNOWN = -1,

    /** Platform calibrated successfully */
    INIT_SUCCESS = 0,

    /** Platform calibration failed */
    INIT_FAILED = 1,
};

/**
 *  Represents the base class for compressed audio formats.
 */
struct FormatParams {

};

/**
 *  Specifies the details of the adaptive multirate wide band format frame.
 */
struct AmrwbpParams : FormatParams {

    /** Bit width of the stream (for example 16 bit) */
    uint32_t bitWidth;

    /** Refer to @ref AmrwbpFrameFormat */
    AmrwbpFrameFormat frameFormat;
};

/**
 *  Defines the parameters when creating an audio stream. The required
 *  parameters for a given use-case are as follows:
 *
 *  For regular voicecall:
 *      type, slotId, channelTypeMask, format, deviceTypes
 *  For hpcm-voicecall:
 *      type, slotId, channelTypeMask, format, deviceTypes, enableHpcm
 *  For ecall:
 *      type, slotId, channelTypeMask, format, deviceTypes, ecnrMode
 *  For proxy mic voicecall:
 *      type, slotId, channelTypeMask, format, deviceTypes, sampleRate
 *
 *  For playback:
 *      type, sampleRate, channelTypeMask, format, deviceTypes
 *  For incall-playback:
 *      type, sampleRate, channelTypeMask, format, deviceTypes, voicePaths
 *  For hpcm-playback:
 *      type, sampleRate, channelTypeMask, format, deviceTypes, voicePaths, enableHpcm
 *  For proxy speaker playback:
 *      type, channelTypeMask, format, deviceTypes, sampleRate
 *
 *  For capture:
 *      type, sampleRate, channelTypeMask, format, deviceTypes
 *  For incall-capture:
 *      type, sampleRate, channelTypeMask, format, deviceTypes, voicePaths
 *  For hpcm-capture:
 *      type, sampleRate, channelTypeMask, format, deviceTypes, voicePaths, enableHpcm
 *
 *  For loopback:
 *      type, sampleRate, channelTypeMask, format, deviceTypes
 *
 *  For tone-generation:
 *      type, channelTypeMask, format, deviceTypes
 */
struct StreamConfig {

    /** @ref StreamType - defines purpose of the stream */
    StreamType type;

    /** @deprecated, use the @ref StreamConfig::slotId field instead of this */
    int modemSubId = 1;

    /** SlotId - specifies the slot ID where the UICC card is inserted */
    SlotId slotId = INVALID_SLOT_ID;

    /** Sample rate in Hz. Typical values are 8k, 16k, 32k and 48k.
     *  For Bluetooth use-cases, supported values are 8k and 16k */
    uint32_t sampleRate;

    /** @ref ChannelTypeMask - defines audio channels to use */
    ChannelTypeMask channelTypeMask;

    /** @ref AudioFormat - defines audio format */
    AudioFormat format;

    /** Defines the list of audio devices @ref DeviceType to use for this stream.
     *  For StreamType::PLAY and StreamType::TONE_GENERATOR, a single sink device
     *  should be specified. For StreamType::CAPTURE, a single source device should
     *  be specified. For StreamType::VOICE_CALL and StreamType::LOOPBACK, both
     *  sink and source should be specified with sink as the first device and
     *  source as the second. */
    std::vector<DeviceType> deviceTypes;

    /** For an in-call and hpcm audio usecase, this represents the voice path direction
     *  @ref Direction */
    std::vector<Direction> voicePaths;

    /** @ref FormatParams - defines compressed playback format */
    FormatParams *formatParams;

    /** @ref EcnrMode - true to enable ECNR on an ecall */
    EcnrMode ecnrMode = EcnrMode::DISABLE;

    /** True - if voice call is used with HPCM, false otherwise */
    bool enableHpcm = false;
};

/**
 *  Specifies the parameters when setting up streams for transcoding.
 */
struct FormatInfo {

    /** Sample rate in Hz, typical values 8k/16k/32k/48k
     *  Sample rate is a dummy paramter for voice stream and compressed playback */
    uint32_t sampleRate;

    /** Refer to @ref ChannelTypeMask */
    ChannelTypeMask mask;

    /** Refer to @ref AudioFormat */
    AudioFormat format;

    /** Refer to @ref FormatParams */
    FormatParams *params;
};

/** @} */ /* end_addtogroup telematics_audio_manager */

/** @addtogroup telematics_audio_stream
 * @{ */

/**
 *  Defines the volume levels for a given audio channel.
 */
struct ChannelVolume {

    /** @ref ChannelType to which the volume level is associated. */
    ChannelType channelType;

    /** Volume level — minimum 0.0 and maximum 1.0 */
    float vol;
};

/**
 *  Defines the volume levels for the audio device.
 */
struct StreamVolume {

    /** List of the volume levels per channel, specified by @ref ChannelVolume */
    std::vector<ChannelVolume> volume;

    /** @ref StreamDirection associated with the device */
    StreamDirection dir;
};

/**
 *  Specifies the mute state of the audio device.
 */
struct StreamMute {

    /** True if the device is muted, False if the device is unmuted */
    bool enable;

    /** @ref StreamDirection associated with the device */
    StreamDirection dir;
};

/**
 *  Defines the characteristics of the DTMF tone.
 */
struct DtmfTone {

    /** Lower frequency associated with the DTMF tone */
    DtmfLowFreq lowFreq;

    /** Higher frequency associated with the DTMF tone */
    DtmfHighFreq highFreq;

    /** @ref StreamDirection associated with the stream */
    StreamDirection direction;
};

/** @} */ /* end_addtogroup telematics_audio_stream */

}  // End of namespace audio
}  // End of namespace telux

#endif // TELUX_AUDIO_AUDIODEFINES_HPP
