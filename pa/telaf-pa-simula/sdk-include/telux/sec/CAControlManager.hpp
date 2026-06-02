/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file  CAControlManager.hpp
 * @brief CAControlManager provides support for gathering statistical information
 *        about crypto operations that can be used to control crypto accelerator
 *        usage.
 */

#ifndef TELUX_SEC_CACONTROLMANAGER_HPP
#define TELUX_SEC_CACONTROLMANAGER_HPP

#include <cstdint>
#include <memory>

#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace sec {

/** @addtogroup telematics_sec_mgmt
 * @{ */

/**
 * Specifies how load should be calculated.
 */
struct LoadConfig {
    /* Defines the time window (in milliseconds) during which
     * load is calculated. At the end of this window, load will be
     * received by @ref ICAControlManagerListener::onLoadUpdate(). */
    uint64_t calculationInterval;
};

/**
 * Represents curve-wise absolute capacity. This value represents
 * capacity as if only that type of curve is used in all crypto
 * operations. For example, a capacity of 3000 for sm2 means, 3000
 * signature verifications of type sm2 can be done under current
 * operating conditions, when no other type of verifications are performed.
 */
struct CACapacity {

    uint32_t sm2;
    /**< SM2 ISO/IEC 14888 */
    uint32_t nist256;
    /**< NIST curve P-256 */
    uint32_t nist384;
    /**< NIST curve P-396 */
    uint32_t bp256;
    /**< Brainpool 256-bit curve */
    uint32_t bp384;
    /**< Brainpool 384-bit curve */
};

/**
 * Represents curve-wise absolute load as calculated in the time window defined
 * by @ref LoadConfig::calculationInterval. For example, a value of 1000
 * for sm2 means, in that time window, 1000 sm2 type verification were completed.
 * This verification includes both passed and failed signature.
 */
struct CALoad {

    uint32_t sm2;
    /**< SM2 ISO/IEC 14888 */
    uint32_t nist256;
    /**< NIST curve P-256 */
    uint32_t nist384;
    /**< NIST curve P-396 */
    uint32_t bp256;
    /**< Brainpool 256-bit curve */
    uint32_t bp384;
    /**< Brainpool 384-bit curve */
};

/**
 * Receives load and capacity updates.
 */
class ICAControlManagerListener : public telux::common::ISDKListener {

 public:
    /**
     * Invoked to provide an updated capacity.
     *
     * @param[in] newCapacity New capacity as per current allowed conditions.
     *
     */
    virtual void onCapacityUpdate(struct CACapacity newCapacity) { }

    /**
     * Invoked to provide load on crypto accelerator, as observed during time window
     * defined by @ref LoadConfig::calculationInterval.
     *
     * @param[in] currentLoad Load as observed in the set time window.
     *
     */
    virtual void onLoadUpdate(struct CALoad currentLoad) { }

    /**
     * Destructor for ICAControlManagerListener.
     */
    virtual ~ICAControlManagerListener() { }
};

/*
 * Provides support for gathering statistical information about crypto operations
 * that can be used to control crypto accelerator usage.
 */
class ICAControlManager {

 public:

   /**
    * Registers the given listener to get load and capacity updates in
    * @ref ICAControlManagerListener::onLoadUpdate() and
    * @ref ICAControlManagerListener::onCapacityUpdate() methods.
    *
    * Capacity updates are received whenever capacity changes. Load updates
    * are received as per parameters specified with @ref startMonitoring().
    *
    * @param [in] listener Receives load and capacity updates
    *
    * @returns @ref telux::common::Status::SUCCESS if the listener is registered,
    *          otherwise, an appropriate error code
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
    *             to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode registerListener(
        std::weak_ptr<ICAControlManagerListener> listener) = 0;

   /**
    * Unregisters the given listener registered previously with @ref registerListener().
    *
    * @param [in] listener Listener to deregister
    *
    * @returns @ref telux::common::Status::SUCCESS if the listener is unregistered,
    *          otherwise, an appropriate error code
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
    *             to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode deRegisterListener(
        std::weak_ptr<ICAControlManagerListener> listener) = 0;

   /**
    * Starts monitoring and reporting load calculated based on the parameters specified.
    * Calculated load is received by @ref ICAControlManagerListener::onLoadUpdate()
    * periodically as per time interval specified.
    *
    * On platforms with access control enabled, caller needs to have TELUX_SEC_CA_CTRL_LOAD_OPS
    * permission to invoke this API successfully.
    *
    * @param [in] loadConfig Defines load calculation parameters
    *
    * @returns @ref telux::common::Status::SUCCESS if the monitoring started,
    *          otherwise, an appropriate error code
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
    *             to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode startMonitoring(LoadConfig loadConfig) = 0;

   /**
    * Stops monitoring the load calculation previosuly started by @ref startMonitoring().
    *
    * On platforms with access control enabled, caller needs to have TELUX_SEC_CA_CTRL_LOAD_OPS
    * permission to invoke this API successfully.
    *
    * @returns @ref telux::common::Status::SUCCESS if the monitoring stopped,
    *          otherwise, an appropriate error code
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
    *             to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode stopMonitoring() = 0;

   /**
    * Provides current verification capacity of the crypto accelerator.
    *
    * @param [out] capacity current capacity of the crypto accelerator
    *
    * @returns @ref telux::common::Status::SUCCESS if the capacity is fetched,
    *          otherwise, an appropriate error code
    *
    * @note Eval: This is a new API and is being evaluated. It is subject
    *             to change and could break backwards compatibility.
    */
   virtual telux::common::ErrorCode getCapacity(CACapacity& capacity) = 0;

   /**
    * Destructor of ICAControlManager. Cleans up as applicable.
    */
   virtual ~ICAControlManager() { };
};

/** @} */  // end_addtogroup telematics_sec_mgmt

}  // End of namespace sec
}  // End of namespace telux

#endif // TELUX_SEC_CACONTROLMANAGER_HPP
