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
 * @file  AudioManager.hpp
 * @brief Defines the APIs to create and manage streams.
 */

#ifndef TELUX_AUDIO_AUDIOMANAGER_HPP
#define TELUX_AUDIO_AUDIOMANAGER_HPP

#include <telux/audio/AudioTranscoder.hpp>

namespace telux {
namespace audio {

class IAudioDevice;
class IAudioStream;
class IAudioVoiceStream;
class IAudioPlayStream;
class IAudioCaptureStream;

/** @addtogroup telematics_audio_stream
 * @{ */

/**
 *  Represents the buffer containing the audio data for playback when used with
 *  the @ref StreamType::PLAY stream. Represents the audio data received when used
 *  with the @ref StreamType::CAPTURE stream.
 */
class IAudioBuffer {
 public:
   /**
    * For the @ref StreamType::PLAY stream, specifies the optimal number of bytes that
    * must be sent for playback. For the @ref StreamType::CAPTURE stream, specifies
    * the optimal number of bytes that can be read.
    *
    * @returns Optimal size (in bytes)
    */
   virtual size_t getMinSize() = 0;

   /**
    * For the @ref StreamType::PLAY stream, specifies the maximum number of bytes that
    * can be sent for playback. For the @ref StreamType::CAPTURE stream, specifies
    * the maximum number of bytes that can be read.
    *
    * @returns Maximum size (in bytes)
    */
   virtual size_t getMaxSize() = 0;

   /**
    * Gives the managed raw buffer. It is freed when IAudioBuffer is destructed. For
    * the @ref StreamType::PLAY stream, the actual audio samples should be copied into
    * this raw buffer for playback. For the @ref StreamType::CAPTURE stream, the actual
    * audio contents are obtained from this buffer.
    *
    * @returns Managed raw buffer
    */
   virtual uint8_t *getRawBuffer() = 0;

   /**
    * For the @ref StreamType::CAPTURE stream, specifies how many bytes were read.
    * Not used for the @ref StreamType::PLAY stream.
    *
    * @returns Size of the valid data bytes in the raw buffer
    */
   virtual uint32_t getDataSize() = 0;

   /**
    * For the @ref StreamType::PLAY stream, specifies how many bytes should be played.
    * Not used for the @ref StreamType::CAPTURE stream.
    *
    * @returns Size of the valid data bytes in the raw buffer
    */
   virtual void setDataSize(uint32_t size) = 0;

   /**
    * Clears the contents of the managed raw buffer.
    *
    * @returns @ref telux::common::Status::SUCCESS if the buffer is cleared successfully,
    *          otherwise, an appropriate error code
    */
   virtual telux::common::Status reset() = 0;

    /**
     * Destructor of the IAudioBuffer.
     */
   virtual ~IAudioBuffer() {};
};

/**
 *  Implements the @ref IAudioBuffer interface to give contexual meaning to its methods
 *  based on the @ref StreamType type associated with the stream, with which this
 *  buffer will be used.
 */
class IStreamBuffer : virtual public IAudioBuffer {
 public:
    /**
     * Destructor of the IStreamBuffer.
     */
    virtual ~IStreamBuffer() {};
};

/** @} */ /* end_addtogroup telematics_audio_stream */

/** @addtogroup telematics_audio_manager
 * @{ */

/**
 * Invoked to pass the list of the supported audio devices. Used in conjunction
 * with IAudioManager::getDevices().
 *
 * @param [in] devices  List of the @ref IAudioDevice devices
 *
 * @param [in] error    @ref telux::common::ErrorCode::SUCCESS if the list is
 *                      fetched successfully, otherwise, an appropriate error code
 */
using GetDevicesResponseCb = std::function<void(std::vector<
        std::shared_ptr<IAudioDevice>> devices, telux::common::ErrorCode error)>;

/**
 * Invoked to pass the list of the supported audio stream types. Used in conjunction
 * with IAudioManager::getStreamTypes().
 *
 * @param [in] streamTypes List of the @ref StreamType types
 *
 * @param [in] error       @ref telux::common::ErrorCode::SUCCESS if the list is
 *                         fetched successfully, otherwise, an appropriate error code
 */
using GetStreamTypesResponseCb = std::function<void(std::vector<StreamType> streamTypes,
        telux::common::ErrorCode error)>;

/**
 * Invoked to pass the instance of the audio stream created. Used in conjunction with
 * IAudioManager::createStream().
 *
 * Passed stream should be type casted before using it, according to the StreamType::*
 * type that was requested while creating it.
 *
 * For @ref StreamType::VOICE_CALL type cast to IAudioManager::IAudioVoiceStream.
 * For @ref StreamType::CAPTURE type cast to IAudioManager::IAudioCaptureStream.
 * For @ref StreamType::PLAY cast to IAudioManager::IAudioPlayStream.
 * For @ref StreamType::LOOPBACK cast to IAudioManager::IAudioLoopbackStream.
 * For @ref StreamType::TONE_GENERATOR cast to IAudioManager::IAudioToneGeneratorStream.
 *
 * @param [in] stream  Audio stream created or nullptr if creation failed
 *
 * @param [in] error   @ref telux::common::ErrorCode::SUCCESS if the stream is
 *                     created successfully, otherwise, an appropriate error code
 */
using CreateStreamResponseCb = std::function<void(std::shared_ptr<IAudioStream> &stream,
        telux::common::ErrorCode error)>;

/**
 * Invoked to pass the instance of the @ref ITranscoder created. Used in conjunction
 * with IAudioManager::createTranscoder().
 *
 * @param [in] transcoder ITranscoder instance or nullptr if transcoder can't be setup
 *
 * @param [in] error      @ref telux::common::ErrorCode::SUCCESS if the transcoder is
 *                        set up successfully, otherwise, an appropriate error code
 */
using CreateTranscoderResponseCb = std::function<void(std::shared_ptr<ITranscoder> &transcoder,
        telux::common::ErrorCode error)>;

/**
 * Invoked to confirm if the stream is deleted or not. Used in conjunction with
 * IAudioManager::deleteStream().
 *
 * @param [in] error   @ref telux::common::ErrorCode::SUCCESS if the stream is
 *                     deleted, otherwise, an appropriate error code
 */
using DeleteStreamResponseCb = std::function<void(telux::common::ErrorCode error)>;

/**
 * Invoked to pass the audio calibration database (ACDB) initialization status. Used in
 * conjunction with IAudioManager::getCalibrationInitStatus().
 *
 * @param [in] calInitStatus @ref CalibrationInitStatus::INIT_SUCCESS if the ACDB is
 *                           initialized successfully, CalibrationInitStatus::INIT_FAILED
 *                           if initialization failed
 *
 * @param [in] error         @ref telux::common::ErrorCode::SUCCESS if the status is
 *                           is fetched successfully, otherwise, an appropriate error code
 */
using GetCalInitStatusResponseCb = std::function<void(CalibrationInitStatus calInitStatus,
        telux::common::ErrorCode error)>;

/**
 *  Provides the APIs to discover the supported audio devices, create streams, and subscribe
 *  for audio service status updates.
 */
class IAudioManager {
 public:
   /**
    * Checks if the audio service is ready for use.
    *
    * @returns True if the audio service is ready for use, otherwise, False
    *
    * @deprecated Use @ref getServiceStatus()
    */
   virtual bool isSubsystemReady() = 0;

   /**
    * Gets the audio service status.
    *
    * @returns @ref telux::common::ServiceStatus::SERVICE_AVAILABLE if the audio
    *          service is ready for use,
    *          @ref telux::common::ServiceStatus::SERVICE_UNAVAILABLE if the audio
    *          service is temporarily unavailable (possibly undergoing initialization),
    *          @ref telux::common::ServiceStatus::SERVICE_FAILED if the audio
    *          service needs re-initialization
    */
   virtual telux::common::ServiceStatus getServiceStatus() = 0;

   /**
    * Suggests when the audio service is ready.
    *
    * @returns Future to block on until the service status is updated to read
    *
    * @deprecated Use @ref telux::common::InitResponseCb in @ref AudioFactory::getAudioManager()
    */
   virtual std::future<bool> onSubsystemReady() = 0;

   /**
    * Gets the list of the supported audio devices.
    *
    * @param [in] callback Mandatory, callback that will receive the list
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status getDevices(GetDevicesResponseCb callback = nullptr) = 0;

   /**
    * Gets the list of the supported stream types.
    *
    * @param [in] callback Mandatory, callback that will receive the list
    *
    * @returns Status @ref telux::common::Status::SUCCESS if the request is initiated
    *                 successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status getStreamTypes(GetStreamTypesResponseCb callback = nullptr) = 0;

   /**
    * Creates an audio stream with the parameters provided.
    *
    * For incall playback/capture use cases, StreamType::VOICE_CALL should be created
    * before StreamType::PLAY and StreamType::CAPTURE.
    *
    * On platforms with access control enabled, the caller must have TELUX_AUDIO_VOICE,
    * TELUX_AUDIO_PLAY, TELUX_AUDIO_CAPTURE, or TELUX_AUDIO_FACTORY_TEST permission
    * to invoke this method successfully.
    *
    * @param [in] streamConfig Parameters of the stream
    *
    * @param [in] callback     Mandatory, invoked to pass the stream created
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status createStream(StreamConfig streamConfig,
        CreateStreamResponseCb callback = nullptr) = 0;

   /**
    * Set up the transcoder with the given parameters.
    *
    * Transcoder instance is obtained in @ref CreateTranscoderResponseCb. It can be
    * used only for a single transcoding operation.
    *
    * On platforms with access control enabled, the caller must have TELUX_AUDIO_TRANSCODE
    * permission to invoke this method successfully.
    *
    * @param [in] input     Details of the input to transcode
    * @param [in] output    Details of the transcoded output required
    * @param [in] callback  Mandatory, invoked to pass the transcoder instance
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
    virtual telux::common::Status createTranscoder(FormatInfo input, FormatInfo output,
        CreateTranscoderResponseCb callback) = 0;

   /**
    * Deletes the stream created with @ref createStream(). It closes the stream and releases
    * all resources allocated for this stream.
    *
    * For incall playback/capture use cases, StreamType::PLAY and StreamType::CAPTURE
    * streams should be deleted before StreamType::VOICE_CALL.
    *
    * On platforms with access control enabled, the caller must have TELUX_AUDIO_VOICE,
    * TELUX_AUDIO_PLAY, TELUX_AUDIO_CAPTURE, or TELUX_AUDIO_FACTORY_TEST permission to
    * invoke this method successfully.
    *
    * @param [in] stream    Stream to delete
    * @param [in] callback  Optional, invoked to pass the result of the stream deletion
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status deleteStream(std::shared_ptr<IAudioStream> stream,
        DeleteStreamResponseCb callback = nullptr) = 0;

   /**
    * Registers the given listener to get notified when the audio service status changes.
    * The method @ref IAudioListener::onServiceStatusChange() is invoked to notify of the new status.
    *
    * @param [in] listener Invoked to pass the new service status
    *
    * @returns @ref telux::common::Status::SUCCESS if the listener is registered,
    *          otherwise, an appropriate error code
    */
   virtual telux::common::Status registerListener(std::weak_ptr<IAudioListener> listener) = 0;

   /**
    * Unregisters the given listener registered previously with @ref registerListener().
    *
    * @param [in] listener Listener to unregister
    *
    * @returns @ref telux::common::Status::SUCCESS if the listener is deregistered,
    *          otherwise, an appropriate error code
    */
   virtual telux::common::Status deRegisterListener(std::weak_ptr<IAudioListener> listener) = 0;

   /**
    * Gets the current initialization status of the audio calibration database (ACDB).
    * This status is obtained in the @ref GetCalInitStatusResponseCb callback.
    *
    * @param [in] callback Mandatory, invoked to pass the initialization status
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status getCalibrationInitStatus(GetCalInitStatusResponseCb callback) = 0;

    /**
     * Destructor of the IAudioManager.
     */
   virtual ~IAudioManager() {};
};

/**
 *  Represents an audio device. Used in conjunction with @ref GetDevicesResponseCb.
 */
class IAudioDevice {
 public:
   /**
    * Gets the type of the audio device.
    *
    * @returns Type of the audio device
    */
   virtual DeviceType getType() = 0;

   /**
    * Gets the direction of the audio device.
    *
    * @returns Direction of the audio device
    */
   virtual DeviceDirection getDirection() = 0;

    /**
     * Destructor of the IAudioDevice.
     */
   virtual ~IAudioDevice() {};

 };

/** @} */ /* end_addtogroup telematics_audio_manager */

/** @addtogroup telematics_audio_stream
 * @{ */

/**
 * Invoked to pass the list of the audio devices associated with the stream. Used in
 * conjunction with @ref IAudioStream::getDevice().
 *
 * @param [in] devices List of the devices
 *
 * @param [in] error  @ref telux::common::ErrorCode::SUCCESS if the device list is
 *                    is sent successfully, otherwise, an appropriate error code
 */
using GetStreamDeviceResponseCb = std::function<void(std::vector<DeviceType> devices,
        telux::common::ErrorCode error)>;

/**
 * Invoked to pass the current volume level of the audio device. Used in conjunction
 * with @ref IAudioStream::getVolume().
 *
 * @param [in] volume Details of the volume
 *
 * @param [in] error  @ref telux::common::ErrorCode::SUCCESS if the volume level
 *                    is read successfully, otherwise, an appropriate error code
 */
using GetStreamVolumeResponseCb = std::function<void(StreamVolume volume,
        telux::common::ErrorCode error)>;

/**
 * Invoked to pass the current mute state of the stream. Used in conjunction with
 * IAudioStream::getMute().
 *
 * @param [in] mute   Details about the mute state
 *
 * @param [in] error  @ref telux::common::ErrorCode::SUCCESS if the mute state
 *                    is read successfully, otherwise, an appropriate error code
 */
using GetStreamMuteResponseCb = std::function<void(StreamMute mute,
        telux::common::ErrorCode error)>;

/**
 *  Base class for all audio stream types. Contains the common properties and methods.
 */
class IAudioStream {
 public:
   /**
    * Gets the @ref StreamType associated with the stream.
    *
    * @returns Type of the stream
    */
   virtual StreamType getType() = 0;

   /**
    * Associates the given audio device with the stream.
    *
    * Applicable for @ref StreamType::VOICE_CALL, @ref StreamType::PLAY, and
    * @ref StreamType::CAPTURE only.
    *
    * For @ref StreamType::VOICE_CALL, the stream must be started using
    * @ref IAudioVoiceStream::startAudio() to make the device effective.
    *
    * @param [in] devices List of the audio devices to use with the stream
    *
    * @param [in] callback Optional, invoked to confirm if the device is associated
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status setDevice(std::vector<DeviceType> devices,
        telux::common::ResponseCallback callback = nullptr) = 0;

   /**
    * Gets the list of the audio devices associated with the stream.
    *
    * Applicable for @ref StreamType::VOICE_CALL, @ref StreamType::PLAY, and
    * @ref StreamType::CAPTURE only.
    *
    * @param [in] callback Mandatory, invoked to pass the associated device
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status getDevice(GetStreamDeviceResponseCb callback = nullptr) = 0;

   /**
    * Sets the volume level of the audio device.
    *
    * For @ref StreamType::VOICE_CALL and @ref StreamType::PLAY, direction must be
    * @ref StreamDirection::RX. For @ref StreamType::CAPTURE, direction must be
    * @ref StreamDirection::TX.
    *
    * If the stream was created with ChannelType::LEFT, ChannelType::LEFT must be
    * specified in StreamVolume. If the stream was created with ChannelType::RIGHT,
    * ChannelType::RIGHT must be specified in StreamVolume. If the stream contains
    * both channels, both channels must be given in StreamVolume.
    *
    * Applicable for @ref StreamType::VOICE_CALL, @ref StreamType::PLAY, and
    * @ref StreamType::CAPTURE only.
    *
    * @param [in] volume   Specifies the volume level and the stream's direction
    *
    * @param [in] callback Optional, invoked to confirm if the volume level is set
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status setVolume(StreamVolume volume,
        telux::common::ResponseCallback callback = nullptr) = 0;

   /**
    * Gets the current volume level of the audio device.
    *
    * For @ref StreamType::VOICE_CALL and @ref StreamType::PLAY, direction must be
    * @ref StreamDirection::RX. For @ref StreamType::CAPTURE, direction must be
    * @ref StreamDirection::TX.
    *
    * If the stream was created with ChannelType::LEFT, ChannelType::LEFT must be
    * specified in StreamVolume. If the stream was created with ChannelType::RIGHT,
    * ChannelType::RIGHT must be specified in StreamVolume. If the stream contains
    * both channels, both channels must be given in StreamVolume.
    *
    * Applicable for @ref StreamType::VOICE_CALL, @ref StreamType::PLAY, and
    * @ref StreamType::CAPTURE only.
    *
    * @param [in] dir      Direction of the stream associated with the device
    *
    * @param [in] callback Mandatory, invoked to pass the volume read
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status getVolume(StreamDirection dir,
        GetStreamVolumeResponseCb callback = nullptr) = 0;

   /**
    * Mute or unmute the stream as specified by the @ref StreamMute provided.
    *
    * Applicable for @ref StreamType::VOICE_CALL, @ref StreamType::PLAY, and
    * @ref StreamType::CAPTURE only.
    *
    * For @ref StreamType::VOICE_CALL, the stream must be started using
    * @ref IAudioVoiceStream::startAudio() before setting the mute state.
    *
    * @param [in] mute     Defines the stream is to be muted or unmuted
    *
    * @param [in] callback Optional, invoked to confirm if the stream is muted/unmuted
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status setMute(StreamMute mute,
        telux::common::ResponseCallback callback = nullptr) = 0;

   /**
    * Gets the current mute state of the audio stream.
    *
    * Applicable for @ref StreamType::VOICE_CALL, @ref StreamType::PLAY, and
    * @ref StreamType::CAPTURE only.
    *
    * For @ref StreamType::VOICE_CALL, the stream must be started using
    * @ref IAudioVoiceStream::startAudio() before reading the mute state.
    *
    * @param [in] dir      Direction of the stream
    *
    * @param [in] callback Mandatory, invoked to pass the mute state
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status getMute(StreamDirection dir,
        GetStreamMuteResponseCb callback = nullptr) = 0;

    /**
     * Destructor of the IAudioStream.
     */
   virtual ~IAudioStream() {};
};

/**
 *  Represents the stream created with the @ref StreamType::VOICE_CALL type. Provides
 *  methods to establish a voice call on a cellular network, and play and detect DTMF tones.
 */
class IAudioVoiceStream : virtual public IAudioStream {
 public:
   /**
    * Starts a voice call stream.
    *
    * @param [in] callback Optional, invoked to confirm if the stream has started
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status startAudio(
        telux::common::ResponseCallback callback = nullptr) = 0;

   /**
    * Stops a voice call stream.
    *
    * @param [in] callback Optional, invoked to confirm if the stream has stopped
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status stopAudio(
        telux::common::ResponseCallback callback = nullptr) = 0;

   /**
    * Generates a DTMF tone on a local device (on RX path) associated with the active
    * voice call stream.
    *
    * @param [in] dtmfTone  Specifies the tone's properties
    *
    * @param [in] duration Duration (in milliseconds) for which the tone is played.
    *                      Set it to @ref INFINITE_TONE_DURATION to play indefinitely
    *
    * @param [in] gain      Volume level of the tone, valid value range is 0 to 4000
    *
    * @param [in] callback  Optional, invoked to confirm if the tone play has started
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status playDtmfTone(DtmfTone dtmfTone, uint16_t duration,
        uint16_t gain, telux::common::ResponseCallback callback = nullptr) = 0;

   /**
    * If @ref IAudioVoiceStream::playDtmfTone() was called with the duration set to
    * @ref INFINITE_DTMF_DURATION, then this method stops playing the DTMF tone.
    *
    * @param [in] direction Direction of the stream
    *
    * @param [in] callback  Optional, invoked to confirm if the tone play has stopped
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status stopDtmfTone(StreamDirection direction,
        telux::common::ResponseCallback callback = nullptr) = 0;

   /**
    * Registers the given listener to get notified whenever a DTMF tone is detected on a
    * voice call stream. Used in conjunction with @ref IVoiceListener::onDtmfToneDetection().
    *
    * @param [in] listener Receives the DTMF tone detected event
    *
    * @param [in] callback Optional, invoked to confirm if the registration is successful
    *
    * @returns @ref telux::common::Status::SUCCESS if the listener is registered,
    *          otherwise, an appropriate error code
    */
   virtual telux::common::Status registerListener(std::weak_ptr<IVoiceListener> listener,
        telux::common::ResponseCallback callback = nullptr) = 0;

   /**
    * Unregisters the given listener registered with @ref IAudioVoiceStream::registerListener().
    *
    * @param [in] listener Listener to unregister
    *
    * @returns @ref telux::common::Status::SUCCESS if the listener is unregistered,
    *          otherwise, an appropriate error code
    */
   virtual telux::common::Status deRegisterListener(std::weak_ptr<IVoiceListener> listener) = 0;

    /**
     * Destructor of the IAudioVoiceStream.
     */
   virtual ~IAudioVoiceStream() {};
};

/**
 * Used in conjunction with @ref IAudioPlayStream::write(). Invoked to pass the audio data
 * length (in bytes) played from the given buffer.
 *
 * Application can clear the contents of the buffer by calling @ref IAudioBuffer::reset()
 * before reusing it for the subsequent write operation.
 *
 * @param [in] buffer Buffer passed in the call to @ref IAudioPlayStream::write()
 *
 * @param [in] error  @ref telux::common::ErrorCode::SUCCESS if the playback was successful,
 *                    otherwise, an appropriate error code
 */
using WriteResponseCb =
        std::function<void(std::shared_ptr<IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error)>;

/**
 *  Represents the stream created with the @ref StreamType::PLAY type. Provides the methods to
 *  play the audio.
 */
class IAudioPlayStream : virtual public IAudioStream {
 public:
   /**
    * Gets an audio buffer containing the audio samples to play.
    *
    * @returns @ref IStreamBuffer instance or nullptr if memory allocation fails
    */
    virtual std::shared_ptr<IStreamBuffer> getStreamBuffer() = 0;

   /**
    * Sends the audio data for playback. First write starts the playback operation.
    *
    * For uncompressed playback (for example, @ref AudioFormat::PCM_16BIT_SIGNED),
    * the next buffer can be sent the moment telux::common::ErrorCode::SUCCESS is
    * received by @ref WriteResponseCb.
    *
    * For compressed playback (for example, AudioFormat::AMR*), the next buffer
    * should be sent only after both; (a) telux::common::ErrorCode::SUCCESS is received
    * by @ref WriteResponseCb (indicating that the current buffer has been pushed in the pipeline
    * for playback) and (b) IPlayListener::onReadyForWrite() has been invoked (indicating that
    * the pipeline can accommodate the next buffer).
    *
    * @param [in] buffer   Contains the audio data to play
    *
    * @param [in] callback Optional, invoked to confirm if the data is played successfully
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
    virtual telux::common::Status write(std::shared_ptr<IStreamBuffer> buffer,
        WriteResponseCb callback = nullptr) = 0;

   /**
    * Finishes the ongoing compressed playback in a way specified by the @ref StopType provided.
    *
    * @param [in] callback Optional, invoked to confirm if the playback has finished
    *
    * @param [in] stopType Defines how to finish playback
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
    virtual telux::common::Status stopAudio(StopType stopType,
        telux::common::ResponseCallback callback = nullptr) = 0;

   /**
    * Registers the given listener to receive events; (a) pipeline is ready to accept the next
    * buffer for compressed playback (b) compressed playback has stopped. Events are
    * received by the listener implementing the @ref IPlayListener interface.
    *
    * @param [in] listener Receives the playstream events
    *
    * @returns @ref telux::common::Status::SUCCESS if the listener is registered,
    *          otherwise, an appropriate error code
    */
    virtual telux::common::Status registerListener(std::weak_ptr<IPlayListener> listener) = 0;

   /**
    * Unregisters the given listener registered with @ref IAudioPlayStream::registerListener().
    *
    * @param [in] listener Listener to unregister
    *
    * @returns @ref telux::common::Status::SUCCESS if the listener is unregistered,
    *          otherwise, an appropriate error code
    */
    virtual telux::common::Status deRegisterListener(std::weak_ptr<IPlayListener> listener) = 0;

    /**
     * Destructor of the IAudioPlayStream.
     */
    virtual ~IAudioPlayStream() {};
};

/**
 * Used in conjunction with @ref IAudioCaptureStream::read(). Invoked to pass the captured
 * audio samples. The IAudioBuffer::getDataSize() gives the length of the data (in bytes).
 *
 * After the samples have been processed by the application, it can clear the contents of the
 * buffer by calling @ref IAudioBuffer::reset().
 *
 * @param [in] buffer Buffer passed in the call to @ref IAudioCaptureStream::read()
 *
 * @param [in] error  @ref telux::common::ErrorCode::SUCCESS if the capture was successful,
 *                    otherwise, an appropriate error code
 */
using ReadResponseCb =
        std::function<void(std::shared_ptr<IStreamBuffer> buffer,
        telux::common::ErrorCode error)>;

/**
 *  Represents the stream created with the @ref StreamType::CAPTURE type. Provides
 *  the methods to read the captured audio.
 */
class IAudioCaptureStream : virtual public IAudioStream {
 public:
   /**
    * Gets an audio buffer that will contain the audio data read.
    *
    * @returns @ref IStreamBuffer instance or nullptr if memory allocation fails
    */
   virtual std::shared_ptr<IStreamBuffer> getStreamBuffer() = 0;

   /**
    * Read the audio data from the source device associated with this stream. Data captured
    * will be received by the @ref ReadResponseCb callback.
    *
    * First read call starts the capture operation.
    *
    * @param [in] buffer       Buffer in which data should be read
    *
    * @param [in] bytesToRead  Length of the data (in bytes) to read
    *
    * @param [in] callback     Mandatory, receives the captured data
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status read(std::shared_ptr<IStreamBuffer> buffer,
      uint32_t bytesToRead, ReadResponseCb callback = nullptr) = 0;

    /**
     * Destructor of the IAudioCaptureStream.
     */
   virtual ~IAudioCaptureStream() {};
};

/**
 *  Represents the stream created with the @ref StreamType::LOOPBACK type. Provides
 *  the methods to start and stop the audio loopback operation.
 */
class IAudioLoopbackStream : virtual public IAudioStream {
 public:
  /**
    * Starts looping back the audio between the source and sink devices associated with this
    * stream.
    *
    * @param [in] callback Optional, invoked to confirm if the loopback has started
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status startLoopback(
        telux::common::ResponseCallback callback = nullptr) = 0;

  /**
    * Starts looping back the audio between the source and sink devices associated with this
    * stream.
    *
    * @param [in] callback  Optional, invoked to confirm if the loopback has stopped
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status stopLoopback(
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Destructor of the IAudioLoopbackStream.
     */
   virtual ~IAudioLoopbackStream() {};
};

/**
 *  Represents the stream created with the @ref StreamType::TONE_GENERATOR type. Provides
 *  the methods to play an audio tone.
 */
class IAudioToneGeneratorStream : virtual public IAudioStream {
 public:
  /**
    * Plays an audio tone with the given parameters.
    *
    * @param [in] freq      Frequency of the tone. For single tone, freq[0] should be provided.
    *                       For dual tone, both freq[0] and freq[1] should be provided.
    *
    * @param [in] duration  Duration (in milliseconds) for which the tone is played. Set it
    *                       to @ref INFINITE_TONE_DURATION to play indefinitely
    *
    * @param [in] gain      Defines the volume level of the tone, valid value range is 0 to 4000
    *
    * @param [in] callback  Optional, invoked to confirm if the tone play started
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status playTone(std::vector<uint16_t> freq, uint16_t duration,
               uint16_t gain, telux::common::ResponseCallback callback = nullptr) = 0;

   /**
    * If the @ref IAudioToneGeneratorStream::playTone() was called with the
    * @ref INFINITE_TONE_DURATION duration, then this method stops playing the tone.
    *
    * @param [in] callback  Optional, invoked to confirm if the tone play has stopped
    *
    * @returns @ref telux::common::Status::SUCCESS if the request is initiated
    *          successfully, otherwise, an appropriate error code
    */
   virtual telux::common::Status stopTone(telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Destructor of the IAudioToneGeneratorStream.
     */
   virtual ~IAudioToneGeneratorStream() {};
};

/** @} */ /* end_addtogroup telematics_audio_stream */

}  // End of namespace audio
}  // End of namespace telux

#endif // TELUX_AUDIO_AUDIOMANAGER_HPP
