/*
*  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 * @file  AudioListener.hpp
 * @brief Defines the listener classes and methods to receive asynchronous events.
 */

#ifndef TELUX_AUDIO_AUDIOLISTENER_HPP
#define TELUX_AUDIO_AUDIOLISTENER_HPP

#include <telux/common/SDKListener.hpp>
#include <telux/audio/AudioDefines.hpp>

namespace telux {
namespace audio {

/** @addtogroup telematics_audio_stream
 * @{ */

/**
 *  Listener for a DTMF tone detected event on a @ref StreamType::VOICE_CALL stream.
 */
class IVoiceListener : public telux::common::ISDKListener {
 public:
    /**
     * Called when a DTMF tone is detected on a @ref StreamType::VOICE_CALL stream.
     * Used in conjuction with @ref IAudioVoiceStream::registerListener().
     *
     * @param [in] dtmfTone Contains details of the tone detected
     */
    virtual void onDtmfToneDetection(DtmfTone dtmfTone) {}

    /**
     * Destructor of the IVoiceListener.
     */
    virtual ~IVoiceListener() {}
};

/**
 *  Listener for events on a playback stream.
 */
class IPlayListener : public telux::common::ISDKListener {
 public:
    /**
     * Called when the audio pipeline is ready to accept the next buffer to play
     * during compressed playback.
     */
    virtual void onReadyForWrite() {}

    /**
     * Called when the compressed playback has stopped. The stream should be
     * deleted only after this has been called.
     */
    virtual void onPlayStopped() {}

    /**
     * Destructor of IPlayListener.
     */
    virtual ~IPlayListener() {}
};

/** @} */ /* end_addtogroup telematics_audio_stream */

/** @addtogroup telematics_audio_transcoder
 * @{ */

/**
 *  Listener for events during transcoding.
 */
class ITranscodeListener : public telux::common::ISDKListener {
 public:
    /**
     * Called when the audio pipeline is ready to accept the next buffer containing
     * data to transcode.
     */
    virtual void onReadyForWrite() {}

    /**
     * Destructor of ITranscodeListener.
     */
    virtual ~ITranscodeListener() {}
};

/** @} */ /* end_addtogroup telematics_audio_transcoder */

/** @addtogroup telematics_audio_manager
 * @{ */

/**
 * Listener for the audio service availability. Refer to @ref telux::common::IServiceStatusListener
 * for details.
 *
 * When audio service becomes unavailable, if the client was waiting on any outstanding
 * response callbacks for APIs that were called just before the SSR, those response
 * callbacks will not be called anymore. For example, if stream->setVolume(callback) is
 * called and SSR occurs then the 'callback' will be never invoked.
 *
 * All the stream objects (IAudioVoiceStream/IAudioPlayStream/IAudioCaptureStream/
 * IAudioLoopbackStream/IAudioToneGeneratorStream) becomes invalid and should not be
 * used after SSR. When SSR occurs, the implementation cleans up all resources associated
 * with streams that were created before SSR. Application should create new audio stream.
 *
 * The AudioFactory and IAudioManager objects remains valid even after SSR and can be used
 * like they were used before SSR.
 */
class IAudioListener : public telux::common::IServiceStatusListener {
 public:
    /**
     * Destructor of IAudioListener.
     */
    virtual ~IAudioListener() {}
};

/** @} */ /* end_addtogroup telematics_audio_manager */

}  // end of namespace audio
}  // end of namespace telux

#endif // TELUX_AUDIO_AUDIOLISTENER_HPP
