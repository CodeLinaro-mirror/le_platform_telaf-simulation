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
 * @file    AudioTranscoder.hpp
 * @brief   Contain the APIs for transcoding audio data. Transcoding is real time
 *          taking the playback time of the file. For all APIs, the same transcoder
 *          instance should be used.
 */

#ifndef TELUX_AUDIO_AUDIOTRANSCODER_HPP
#define TELUX_AUDIO_AUDIOTRANSCODER_HPP

#include <future>
#include <memory>

#include <telux/audio/AudioListener.hpp>

namespace telux {
namespace audio {

/** @addtogroup telematics_audio_transcoder
 * @{ */

class IAudioBuffer;

/**
 * Called to pass the transcoded audio data. Used with @ref ITranscoder::read().
 *
 * @param [in] buffer       Contains the transcoded data with @ref IAudioBuffer::getDataSize()
 *                          giving the actual number of data bytes in this buffer. Should be
 *                          cleared using @ref IAudioBuffer::reset() before reusing it for
 *                          sending the next compressed data for transcoding.
 *
 * @param [in] isLastBuffer Indicates that this is the last chunk of the transcoded data
 *
 * @param [in] error        @ref telux::common::ErrorCode::SUCCESS if the transcoding
 *                          was successful, an appropriate error code otherwise
 */
using TranscoderReadResponseCb = std::function<void(std::shared_ptr<IAudioBuffer> buffer,
        uint32_t isLastBuffer, telux::common::ErrorCode error)>;

/**
 * Called when the compressed data has been sent for transcoding. Used with
 * @ref ITranscoder::write().
 *
 * @param [in] buffer       Buffer that is passed to @ref ITranscoder::write(). To reuse it for
 *                          sending the next compressed data for transcoding, clear it using
 *                          @ref IAudioBuffer::reset().
 *
 * @param [in] bytesWritten Number of data bytes sent for transcoding
 *
 * @param [in] error        @ref telux::common::ErrorCode::SUCCESS if the data was sent
 *                          successfully, an appropriate error code otherwise
 */
using TranscoderWriteResponseCb = std::function<void(std::shared_ptr<IAudioBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error)>;

/**
 *  Provides the methods for transcoding the compressed audio data.
 */
class ITranscoder {
 public:
    /**
     * Gets a buffer for sending the data for transcoding.
     *
     * @returns @ref IAudioBuffer instance representing the buffer or nullptr if allocation failed
     */
    virtual std::shared_ptr<IAudioBuffer> getWriteBuffer() = 0;

    /**
     * Gets a buffer that will contain the transcoded data.
     *
     * @returns @ref IAudioBuffer instance representing the buffer or nullptr if allocation failed
     */
    virtual std::shared_ptr<IAudioBuffer> getReadBuffer() = 0;

    /**
     * Sends the compressed data for transcoding. First write starts the transcoding operation.
     *
     * Internally, a pipeline is maintained for the data to transcode. The application should send
     * the next data for transcoding only when the pipeline can accomodate more data. This readiness
     * is indicated by calling the @ref ITranscodeListener::onReadyForWrite() method.
     *
     * @param [in] buffer        Contains the data to transcode
     *
     * @param [in] isLastBuffer  Marks that this is the last chunk of the data to transcode
     *
     * @param [in] callback      Optional, invoked to pass the status of pushing the data in the pipeline
     *
     * @returns @ref telux::common::Status::SUCCESS if the data is sent, otherwise,
     *          an appropriate error code
     */
    virtual telux::common::Status write(std::shared_ptr<IAudioBuffer> buffer,
        uint32_t isLastBuffer, TranscoderWriteResponseCb callback = nullptr) = 0;

    /**
     * Destroys the ITranscoder instance created with @ref IAudioManager::createTranscoder().
     * This must be called after the transcoding is finished.
     *
     * @param [in] callback Optional, invoked to pass the result of the destruction
     *
     * @returns @ref telux::common::Status::SUCCESS if the teardown was initiated,
     *          otherwise, an appropriate error code
     */
    virtual telux::common::Status tearDown(
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Initiates a read request to fetch the transcoded data. Transcoded data will be by the
     * @ref TranscoderReadResponseCb callback.
     *
     * @param [in] buffer       Buffer that will contain the transcoded data
     *
     * @param [in] bytesToRead  Length of the data to fetch
     *
     * @param [in] callback     Optional, invoked to pass the transcoded data
     *
     * @returns @ref telux::common::Status::SUCCESS if the request is sent,
     *          otherwise, an appropriate error code
     */
    virtual telux::common::Status read(std::shared_ptr<IAudioBuffer> buffer,
        uint32_t bytesToRead, TranscoderReadResponseCb callback = nullptr) = 0;

    /**
     * Registers the given listener to know 'when the pipeline is ready to accept the next buffer'
     * for transcoding. Event is received by the @ref ITranscodeListener::onReadyForWrite()
     * method.
     *
     * @param [in] listener Receives the events during transcoding
     *
     * @returns @ref telux::common::Status::SUCCESS if the listener is registered,
     *          otherwise, an appropriate error code
     */
    virtual telux::common::Status registerListener(
        std::weak_ptr<ITranscodeListener> listener) = 0;

    /**
     * Unregisters the given listener registered with @ref ITranscoder::registerListener().
     *
     * @param [in] listener Listener to unregister
     *
     * @returns @ref telux::common::Status::SUCCESS if the listener is unregistered,
     *          otherwise, an appropriate error code
     */
    virtual telux::common::Status deRegisterListener(
        std::weak_ptr<ITranscodeListener> listener) = 0;

    /**
     * Destructor of the ITranscoder.
     */
    virtual ~ITranscoder() {};
};

/** @} */ /* end_addtogroup telematics_audio_transcoder */

}  // End of namespace audio
}  // End of namespace telux

#endif // TELUX_AUDIO_AUDIOTRANSCODER_HPP
