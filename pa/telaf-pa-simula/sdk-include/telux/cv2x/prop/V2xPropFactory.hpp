/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       V2xPropFactory.hpp
 * @brief      V2xPropFactory is a primary interface to retrieve the
 *              proprietary V2X Managers.
 */

#ifndef TELUX_CV2X_PROP_V2XPROPFACTORY_HPP
#define TELUX_CV2X_PROP_V2XPROPFACTORY_HPP
#include <telux/cv2x/prop/CongestionControlManager.hpp>
namespace telux {
namespace cv2x {
namespace prop {

/** @addtogroup telematics_cv2x_cpp
 * @{ */

/**
 * @brief V2xPropFactory allows creation of ICongestionControlManager.
 */
class V2xPropFactory {
 public:
    /**
     * Gets the V2xPropFactory instance.
     * On platforms with access control enabled, the caller needs to have
     * TELUX_CV2X_CONGESTION_CONTROL permission to successfully invoke this API.
     */
    static V2xPropFactory &getInstance();

    /**
     * Provides a CongestionControlManager instance to be used to perform v2x
     * congestion control.
     * On platforms with access control enabled, the caller needs to have
     * TELUX_CV2X_CONGESTION_CONTROL permission to successfully invoke this API.
     * @returns ICongestionControlManager instance
     */
    virtual std::shared_ptr<ICongestionControlManager> getCongestionControlManager() = 0;

#ifndef TELUX_DOXY_SKIP
 protected:
    V2xPropFactory();
    virtual ~V2xPropFactory();
#endif

 private:
    V2xPropFactory(const V2xPropFactory &) = delete;
    V2xPropFactory &operator=(const V2xPropFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_cv2x_cpp */

}  // End of namespace prop
}  // End of namespace cv2x
}  // End of namespace telux
#endif // TELUX_CV2X_PROP_V2XPROPFACTORY_HPP