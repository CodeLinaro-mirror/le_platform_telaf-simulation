/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
 * @file       Log.hpp
 * @brief      Log class provides APIs for logging messages at different
 *             log level like DEBUG, INFO, WARNING, ERROR and PERF.
 */

#ifndef TELUX_COMMON_LOG_HPP
#define TELUX_COMMON_LOG_HPP

#include <string>
#include <sstream>

/**
 * Double-Macro-Stringy Technique
 * This technique is being used as __FILE__,__LINE__ are predefined Macros
 * So we need to expand them twice to get the value in desired format
 */

#define LINE_NO_STR(x) #x
#define LINE_NO(x) LINE_NO_STR(x)


/**
 * @ref telux::common::LogLevel::LEVEL_INFO.
 */
#define INFO telux::common::LogLevel::LEVEL_INFO
/**
 * @ref telux::common::LogLevel::LEVEL_DEBUG.
 */
#define DEBUG telux::common::LogLevel::LEVEL_DEBUG
/**
 * @ref telux::common::LogLevel::LEVEL_WARNING.
 */
#define WARNING telux::common::LogLevel::LEVEL_WARNING
/**
 * @ref telux::common::LogLevel::LEVEL_ERROR.
 */
#define ERROR telux::common::LogLevel::LEVEL_ERROR
/**
 * @ref telux::common::LogLevel::LEVEL_PERF.
 */
#define PERF telux::common::LogLevel::LEVEL_PERF

#if !defined(TELUX_TECH_AREA)
#define TELUX_TECH_AREA 0
#endif

/**
 * Public utility macro for logging at different log level(i.e INFO, DEBUG) with variable argument
 * list. More information like file name, line number are automatically added to each logs.
 * Example for using Macro: LOG(DEBUG, "Message").
 */
#define LOG(logLevel, args...) \
   telux::common::Log::logMessage(logLevel, __FILE__, LINE_NO(__LINE__), TELUX_TECH_AREA, args)

namespace telux {
namespace common {

/** @addtogroup telematics_common
 * @{ */

/**
 * Indicates supported logging levels.
 */
enum class LogLevel {
   LEVEL_NONE = 1,
   LEVEL_PERF, /**< Prints messages with nanoseconds precision timestamp */
   LEVEL_ERROR, /**< Prints perf and error messages only */
   LEVEL_WARNING, /**< Prints perf, error and warning messages */
   LEVEL_INFO, /**< Prints perf, errors, warning and information messages */
   LEVEL_DEBUG, /**< Full logging including debug messages */
};

class Log {
public:

   /**
    * Public API to log a message
    *
    * @param [in] logLevel             Log level @ref LogLevel
    * @param [in] fileName             File name from where log is getting printed
    * @param [in] lineNo               Line number from where log is getting printed
    * @param [in] component            Identifier, as listed in SDK configuration
    * @param [in] params               Additional parameters to be logged
    */
   template <typename... MessageArgs>
   static void logMessage(LogLevel logLevel, const std::string &fileName, const std::string &lineNo,
                          const int &component, MessageArgs... params) {
      if (isLoggingEnabled(logLevel, component)) {
         std::ostringstream outputStream;
         telux::common::Log::constructMessage(outputStream, params...);
         telux::common::Log::logStream(outputStream, logLevel, fileName, lineNo, component);
      }
   }

   /**
    * Public API to log a string stream
    *
    * @param [in] outputStream         String stream which will be logged
    * @param [in] logLevel             Log level @ref LogLevel
    * @param [in] fileName             File name from where log is getting printed
    * @param [in] lineNo               Line number from where log is getting printed
    * @param [in] component            Identifier, as listed in SDK configuration
    */
   static void logStream(std::ostringstream &outputStream, LogLevel logLevel,
                         const std::string &fileName, const std::string &lineNo,
                         const int &component);

private:
   /*
    * Recursive helper methods to construct the complete log message
    * from input arguments
    */
   template <typename K, typename T>
   static void constructMessage(K &os, T param) {
      os << param;
   }

   template <typename K, typename T, typename... MessageArgs>
   static void constructMessage(K &os, T param, MessageArgs... params) {
      os << param;
      return constructMessage(os, params...);
   }

   static bool isLoggingEnabled(LogLevel logLevel, const int& component);

};

/** @} */ /* end_addtogroup telematics_common */

} // End of namespace common

} // End of namespace telux

#endif // TELUX_COMMON_LOG_HPP
