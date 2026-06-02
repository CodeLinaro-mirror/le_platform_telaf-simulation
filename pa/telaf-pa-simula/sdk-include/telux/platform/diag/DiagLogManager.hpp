/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file  DiagLogManager.hpp
 * @brief IDiagLogManager provides on-device logging (ODL) support for diagnostics.
 *        Provides APIs to configure, start and stop collecting diagnostics logs.
 */

#ifndef TELUX_PLATFORM_DIAG_DIAGLOGMANAGER_HPP
#define TELUX_PLATFORM_DIAG_DIAGLOGMANAGER_HPP

#include <memory>
#include <cstdint>
#include <string>

#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace platform {
namespace diag {

/** @addtogroup telematics_diagnostics
 * @{ */

/**
 * Minimum size of the log file.
 */
#define MIN_DIAG_FILE_SIZE_MB  1

/**
 * Maximum size of the log file.
 */
#define MAX_DIAG_FILE_SIZE_MB  100

/**
 * Minimum number of the log files.
 */
#define MIN_NUM_DIAG_FILES  2

/**
 * Maximum number of the log files.
 */
#define MAX_NUM_DIAG_FILES  100

/**
 * Log flushing stops when this is reached.
 */
#define MIN_WATER_MARK  1

/**
 * Log flushing starts when this is reached.
 */
#define MAX_WATER_MARK  100

/**
 * Default value - flushing stops when this is reached.
 */
#define DEFAULT_LOW_WATER_MARK  15

/**
 * Default value - flushing starts when this is reached.
 */
#define DEFAULT_HIGH_WATER_MARK  85

/**
 * Specifies how an application gets the collected logs.
 */
enum class LogMethod {
    /**
     * No valid method defined.
     */
    NONE = 0,

    /**
     * File method - logs are saved in file(s).
     *
     * By default, these files are saved in the /tmp/diag directory. Client can specify a
     * different directory by defining platform.diag.diag_output_log_path in the tel.conf
     * file. If specified, the directory should exist before starting the log collection.
     *
     * The logs files typically follow 'diag_log_*_date_time.qmdl' naming convention and
     * can be read in plain text format using Qualcomm extensible diagnostic monitor (QXDM)
     * tool.
     */
    FILE,

    /**
     * Callback method - logs are provided via @ref IDiagListener::onAvailableLogs callback
     * whenever available, in raw format.
     */
    CALLBACK
};

/**
 * Specifies when an application gets the collected logs.
 */
enum class DiagLogMode {
    /**
     * Logs are provided immediately whenever available.
     */
    STREAMING = 0,

    /**
     * In threshold log collection mode, user specifies 2 thresholds -
     * high watermark and low watermark.
     * Refer to @ref BufferedModeConfig::highWaterMark and @ref BufferedModeConfig::lowWaterMark.
     *
     * Logs are flushed to the client when high-watermark is reached, and flushing
     * continues until low-watermark is reached where buffering starts again.
     *
     * Client can explicitly drain the buffer using @ref IDiagLogManager::drainPeripheralBuffer.
     *
     * Whenever a peripheral wants to pass logs to the client, it raises an interrupt
     * to the application processor. Therefore, for streaming mode, there are frequent
     * interrupts which prevents the application processor from entering and remaining
     * into the power collapsed state for longer duration based on the frequency and
     * amount of logs.
     *
     * Specifying this mode causes logs to be accumulated in the peripheral's local
     * buffer. An interrupt is raised only when the high-watermark is reached.
     * Therefore, application processor can remain power collapsed for longer time
     * saving power.
     *
     * Applicable for only peripherals with its own buffer such as Modem DSP.
     */
    THRESHOLD,

    /**
     * Logs are saved in peripheral's local buffer until the client explicitly
     * requests them through @ref IDiagLogManager::drainPeripheralBuffer.
     *
     * Old logs are overwritten when the buffer is full and drain is not issued.
     *
     * Similar to the THRESHOLD mode, this can also be used to save power.
     *
     * Applicable for only peripherals with its own buffer such as Modem DSP.
     */
    CIRCULAR_BUFFER
};

/**
 * Specifies the device for which logs are collected. Logs from all the peripherals
 * on the specified device are collected.
 */
enum DeviceType {
    /**
     * No device (only peripheral logs are collected)
     */
    DIAG_DEVICE_NONE = 0,

    /**
     * External application processor (EAP). Client must be running on the EAP.
     * Unsupported.
     */
    DIAG_DEVICE_EXTERNAL_AP = (1 << 0),

    /**
     * Mobile data modem (MDM). Client can be running on either EAP or MDM.
     * EAP is not supported.
     */
    DIAG_DEVICE_MDM = (1 << 1)
};

/**
 * Bitmask taking devices from @ref diag::DeviceType.
 * Values can be OR'ed together, for ex; (DIAG_DEVICE_EXTERNAL_AP | DIAG_DEVICE_MDM)
 * to specify more than one device.
 */
using Devices = uint8_t;

/**
 * Specifies the peripheral(s) for which logs are collected. Logs from one
 * or more peripherals can be collected by setting corresponding bit in the
 * @ref diag::Peripherals.
 */
enum PeripheralType {
    /**
     * No peripheral (only device logs are collected).
     */
    DIAG_PERIPHERAL_NONE = 0,

    /**
     * Application processor (Apps). On platforms with hypervisor, includes
     * all virtual machines (host and guest).
     * The APPS logs can be collected ONLY via @ref DiagLogMode::STREAMING mode.
     */
    DIAG_PERIPHERAL_INTEGRATED_AP = (1 << 0),

    /**
     * Modem digital signal processor (often referred as Q6).
     */
    DIAG_PERIPHERAL_MODEM_DSP = (1 << 1),

    /**
     * Guest virtual machines (also referred as secondary virtual machine SVM).
     * Unsupported, use DIAG_PERIPHERAL_INTEGRATED_AP instead.
     */
    DIAG_PERIPHERAL_SVM = (1 << 2),

    /**
     * Low power audio subsystem (LPASS).
     * Unsupported.
     */
    DIAG_PERIPHERAL_LPASS = (1 << 3),

    /**
     * Compute digital signal processor (CDSP).
     * Unsupported.
     */
    DIAG_PERIPHERAL_CDSP = (1 << 4)
};

/**
 * Bitmask taking peripheral from @ref diag::PeripheralType.
 * Values can be OR'ed together, for ex; (DIAG_PERIPHERAL_MODEM_DSP | DIAG_PERIPHERAL_CDSP)
 * to specify more than one peripheral.
 */
using Peripherals = uint8_t;

/**
 * Defines source of the logs; device or peripheral. Logs will be collected
 * from this source.
 *
 * Note: device and peripheral logs collection are mutually exclusive.
 */
enum class SourceType {
    /**
     * Unknown source.
     */
    NONE = 0,

    /**
     * Device, refer @ref diag::DeviceType for details.
     */
    DEVICE,

    /**
     * Peripheral, refer @ref diag::PeripheralType for details.
     */
    PERIPHERAL
};

/**
 * Defines device(s) or peripheral(s) from which logs are collected.
 */
union SourceInfo {
    /**
     * Refer @ref diag::Devices for details.
     */
    Devices device = DeviceType::DIAG_DEVICE_NONE;

    /**
     * Refer @ref diag::Peripherals for details.
     */
    Peripherals peripheral;
};

/**
 * Current status of the log collection.
 */
struct DiagStatus {
    /**
     * Currently active method.
     */
    LogMethod logMethod;

    /**
     * True, if the collection has started, false otherwise.
     */
    bool isLoggingInProgress;

    /**
     * Always returns false.
     * @deprecated
     */
    bool isLogDrainInProgress;
};

/**
 * Defines size of a log file and number of such files when collecting logs
 * using file method.
 */
struct FileMethodConfig {
    /**
     * Optional, maximum file size in megabytes (MB) after which a new file will be
     * created to save further logs every time this size is crossed.
     *
     * Default value is MAX_DIAG_FILE_SIZE_MB.
     * Valid range is MIN_DIAG_FILE_SIZE_MB <= maxSize <= MAX_DIAG_FILE_SIZE_MB.
     */
    uint32_t maxSize;

    /**
     * Optional, maximum number of the log files. Older files gets overwritten
     * once this number is reached.
     *
     * Default value is MAX_NUM_DIAG_FILES.
     * Valid range is MIN_NUM_DIAG_FILES <= maxNumber <= MAX_NUM_DIAG_FILES.
     */
    uint32_t maxNumber;
};

/**
 * Defines low and high water marks for @ref DiagLogMode::THRESHOLD
 * For file log collection only default watermarks are used.
 */
struct BufferedModeConfig {
    /**
     * High watermark percentage - logs will be flushed from peripheral's
     * local buffer to the diag framework on the HLOS, when this mark is hit.
     *
     * Default value is DEFAULT_HIGH_WATER_MARK.
     * Valid range is MIN_WATER_MARK <= highWaterMark <= MAX_WATER_MARK.
     */
     uint8_t highWaterMark;

    /**
     * Low watermark percentage - logs flushing will stop when this
     * mark is hit and buffering will resume.
     *
     * Default value is DEFAULT_LOW_WATER_MARK.
     * Valid range is MIN_WATER_MARK <= lowWaterMark <= MAX_WATER_MARK.
     */
    uint8_t lowWaterMark;
};

/**
 * Specifies configuration for the diagnostics log collection process.
 * Please refer to the below table while setting up the configuration-
 *
 * | Log Mode        | File Log Method | Callback Log Method | Is Draining Supported? | Are Watermarks applicable? | APPS log collection | Q6 log collection |
 * |---------------- |-----------------|---------------------|------------------------|----------------------------|---------------------|-------------------|
 * | Streaming       | 1               | 1                   | 0                      | 0                          | 1                   | 1                 |
 * | Threshold       | 1               | 1                   | 1                      | 1                          | 0                   | 1                 |
 * | CircularBuffer  | 1               | 1                   | 1                      | 0                          | 0                   | 1                 |
 *
 * Legends -
 * 1: Supported
 * 0: Not supported
 *
 */
struct DiagConfig {
    /**
     * Refer @ref diag::SourceType for details.
     */
    SourceType srcType;

    /**
     * Refer @ref diag::SourceInfo for details. Based on the source type defined in
     * DiagConfig::srcType, the corresponding field in DiagConfig::srcInfo will be used.
     */
    SourceInfo srcInfo;

    /**
     * Optional, absolute path to the diag packet mask file to use when collecting logs
     * from a MDM device/peripheral. It contains required information to filter logs
     * based on the log type (for ex; event, message, response etc.). It is generated
     * using Qualcomm extensible diagnostic monitor (QXDM) and have cfg extension.
     *
     * Applicable only for MDM device/peripheral.
     *
     * The DiagConfig::SourceType defines the subsystem for which logs are collected.
     * A packet mask further narrows down them to a specific type.
     */
    std::string mdmLogMaskFile = "";

    /**
     * Optional, absolute path to the diag packet mask file when collecting logs from an
     * EAP device or an EAP peripheral.
     *
     * Applicable only for EAP device/peripheral.
     * EAP based support is currently not supported.
     */
    std::string eapLogMaskFile = "";

    /**
     * Specifies how an application gets the collected logs.
     * Refer @ref diag::LogMethod for details
     */
    LogMethod method;

    /**
     * Configuration for the selected method.
     */
    union MethodConfig {
        /**
         * Configuration specific to the file log collection method.
         */
        FileMethodConfig fileConfig;
    } methodConfig;

    /**
     * Specifies when an application gets the collected logs.
     * Refer @ref diag::DiagLogMode for details.
     */
    DiagLogMode modeType = DiagLogMode::STREAMING;

    /**
     * Configuration for the selected mode.
     */
    union ModeConfig {
        /**
         * Configuration specific to the threshold/circular buffer mode.
         */
        BufferedModeConfig bufferedModeConfig;
    } modeConfig;
};

/**
 * Receives diagnostics logs when using callback log method.
 *
 * The methods in listener can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * Client implementing methods in this interface should not perform any long
 * running or blocking operation from within methods in this class to ensure
 * there is no delay in internal processing and further log passing to the
 * application.
 */
class IDiagListener : public telux::common::ISDKListener {
 public:
    /**
     * Called to pass collected diagnostics logs to the client.
     *
     * On platforms with access control enabled, caller needs to have
     * TELUX_DIAG_OPS permission to invoke this API successfully.
     *
     * @param[in] data Logs collected (as byte array)
     * @param[in] length Data length (number of bytes)
     */
    virtual void onAvailableLogs(uint8_t *data, int length){};

    /**
     * Destructor for IDiagListener
     */
    virtual ~IDiagListener(){};
};

/**
 * IDiagLogManager provides ability to collect diagnostics logs on-device.
 *
 * - Methods; two methods are provided; namely file and callback. In the file method, logs
 *   are stored in file(S). In callback method, logs are delivered to the client through a
 *   callback.
 *
 * - Modes; three modes are provided; streaming, threshold and circular to let the client
 *   manage when to get the logs.
 *
 * - Source; logs can be collected from the whole device or selected peripherals.
 *
 *   A client running on the MDM can collect logs from the whole MDM device or from the
 *   specific  MDM peripheral(s).
 *
 *   Consider a fusion architecture where an EAP is connected to the MDM SoC via some interconnect
 *   (e.g. USB, PCIe or Ethernet). (a) If the client is running on the MDM, it can collect logs
 *   from the whole MDM device or from the specific MDM peripheral(s). It cannot collect logs from
 *   EAP whole device or from the peripherals on EAP. (b) If the client is running on the EAP,
 *   it can collect logs from the whole MDM device, from the whole EAP device or from the specific
 *   peripheral(s) on EAP. Currently, EAP is not supported.
 *
 * - Granularity, on top of the log source, logs can be further narrowed down to be of specific
 *   type like message/event/F3 etc. using a mask file.
 *
 * - Decoding: When using file method, logs are in a propriety format and can be decoded through
 *   QXDM to read them in a plain text format. For callback method, they are in raw format.
 *
 * - Concurrency: Two or more processes can not collect logs from the same peripheral. A process
 *   can have only one session. A session is started with startLogCollection() and stopped with
 *   stopLogCollection(). The file and callback methods are mutually exclusive.
 */
class IDiagLogManager {
 public:
    /**
     * Returns current status of the log collection.
     *
     * @returns @ref diag::DiagStatus detailing log collection status.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and could
     *       break backwards compatibility.
     */
    virtual DiagStatus getStatus() = 0;

    /**
     * Registers the given listener to receive logs whenever they are available via
     * @ref IDiagListener.
     *
     * @param[in] listener Receives the diagnostics logs.
     *
     * @returns @ref telux::common::Status::SUCCESS if the listener is registered,
     *          otherwise, an appropriate error code.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and could
     *       break backwards compatibility.
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IDiagListener> listener) = 0;

    /**
     * Deregisters the given listener registered with @ref IDiagLogManager::registerListener().
     *
     * @param[in] listener Listener to deregister.
     *
     * @returns @ref telux::common::Status::SUCCESS if the listener is deregistered,
     *          otherwise, an appropriate error code.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and could
     *       break backwards compatibility.
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<IDiagListener> listener) = 0;

    /**
     * Sets the configuration for the log collection. Once set, it remains effective until
     * new configuration is set.
     *
     * Must be called before starting the log collection. Calling it again during ongoing
     * collection will return @ref telux::common::ErrorCode::INVALID_STATE.
     *
     * On platforms with access control enabled, caller needs to have TELUX_DIAG_OPS permission
     * to invoke this API successfully.
     *
     * @param[in] config @ref diag::DiagConfig defining collection configuration
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS, if the configuration is set,
     *          @ref telux::common::ErrorCode::INVALID_STATE, if the log collection is
     *          in progress, @ref telux::common::ErrorCode::MISSING_RESOURCE when running on
     *          EAP and remote MDM device cannot be detected, otherwise an appropriate error code.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and could
     *       break backwards compatibility.
     */
    virtual telux::common::ErrorCode setConfig(const DiagConfig config) = 0;

    /**
     * Gets current log collection configuration.
     *
     * @returns current configuration @ref telux::platform::diag::DiagConfig.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and could
     *       break backwards compatibility.
     */
    virtual DiagConfig getConfig() = 0;

    /**
     *
     * Draining the buffer refers to clearing of the peripheral's local buffer.
     *
     * This is applicable for @ref DiagLogMode::THRESHOLD and @ref DiagLogMode::CIRCULAR_BUFFER
     *
     * In the case of Threshold log collection, logs are drained until the
     * @ref BufferedModeConfig::lowWaterMark is reached.
     *
     * In the case of Circular buffer log collection, logs are drained until the buffer is empty.
     *
     * This is currently applicable only for @ref PeripheralType::DIAG_PERIPHERAL_MODEM_DSP
     *
     * On platforms with access control enabled, caller needs to have TELUX_DIAG_OPS permission
     * to invoke this API successfully.
     *
     * @param[in] peripherals @ref diag::Peripherals bitmask of peripherals to drain
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS if the drain is initiated,
     *          @ref telux::common::ErrorCode::INVALID_STATE if there is no collection in progress
     *          for given peripheral, @ref telux::common::ErrorCode::NOT_SUPPORTED if draining on
     *          given peripheral is not supported, an appropriate error code in all other cases.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and could
     *       break backwards compatibility.
     */
    virtual telux::common::ErrorCode drainPeripheralBuffers(Peripherals peripherals) = 0;

    /**
     * Starts collecting the logs.
     *
     * Configuration must be set at-least once using @ref IDiagLogManager::setConfig() before
     * calling this method.
     *
     * On platforms with access control enabled, caller needs to have TELUX_DIAG_OPS permission
     * to invoke this API successfully.
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS, if the log collection is started,
     *          @ref telux::common::ErrorCode::NO_EFFECT, if the log collection has already
     *          started, @ref telux::common::ErrorCode:: INVALID_STATE if a valid config is
     *          not set before calling this method, otherwise an appropriate error code.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and could
     *       break backwards compatibility.
     */
    virtual telux::common::ErrorCode startLogCollection() = 0;

    /**
     * Stops collecting the logs.
     *
     * On platforms with access control enabled, caller needs to have TELUX_DIAG_OPS permission
     * to invoke this API successfully.
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS, if the log collection is stopped,
     *          @ref telux::common::ErrorCode::NO_EFFECT, if the log collection has already
     *          stopped, otherwise an appropriate error code.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and could
     *       break backwards compatibility.
     */
    virtual telux::common::ErrorCode stopLogCollection() = 0;

    /**
     * Gets the diag log manager service status.
     *
     * @returns @ref telux::common::ServiceStatus::SERVICE_AVAILABLE if the service is ready,
     *          @ref telux::common::ServiceStatus::SERVICE_UNAVAILABLE if the service
     *          is temporarily unavailable (possibly undergoing initialization),
     *          @ref telux::common::ServiceStatus::SERVICE_FAILED if the service needs
     *          re-initialization
     */
   virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Drains logs from the circular buffer.
     *
     * Applicable only for the circular buffering mode.
     *
     * Until @ref IDiagLogManager::stopDrainingLogs is called, logs will continue to be
     * written to the circular buffer, getting flushed and provided to the application.
     *
     * On platforms with access control enabled, caller needs to have TELUX_DIAG_OPS permission
     * to invoke this API successfully.
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS if the draining is started,
     *          @ref telux::common::ErrorCode::INVALID_STATE if there is no collection in progress,
     *          @ref telux::common::ErrorCode::NO_EFFECT if draining is already in progress,
     *          an appropriate error code in all other cases.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and could
     *       break backwards compatibility.
     *
     * @deprecated Use @ref IDiagLogManager::drainPeripheralBuffer() instead
     */
    virtual telux::common::ErrorCode startDrainingLogs() = 0;

    /**
     * Stops draining the logs and continues collecting them pushing into internal buffer.
     *
     * Applicable only for the circular buffering mode.
     *
     * On platforms with access control enabled, caller needs to have TELUX_DIAG_OPS permission
     * to invoke this API successfully.
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS if the draining is stopped,
     *          @ref telux::common::ErrorCode::INVALID_STATE if there is no collection in progress,
     *          @ref telux::common::ErrorCode::NO_EFFECT if there is no draining in progress,
     *          an appropriate error code in all other cases.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and could
     *       break backwards compatibility.
     *
     * @deprecated This is no longer used.
     */
    virtual telux::common::ErrorCode stopDrainingLogs() = 0;

    /**
     * Destructor for IDiagLogManager.
     */
    virtual ~IDiagLogManager(){};
};

/** @} */ /* end_addtogroup telematics_diagnostics */

} // end of namespace diag
} // end of namespace platform
} // end of namespace telux

#endif // TELUX_PLATFORM_DIAG_DIAGLOGMANAGER_HPP
