/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <syslog.h>
#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

#include "ReportReader.hpp"

using namespace telux::loc;

void ReportReader::readCsvData(std::string configVal, csvData &csvReportData,
                               std::shared_ptr<SimulationConfigParser> configParser)
{
    syslog(LOG_INFO, "[bypass] Location/readCsvData");

    /* Declare a class' method type */
    typedef void (ReportReader::*classMethodType) (std::string , csvData &, std::shared_ptr<SimulationConfigParser> );

    /* Just once Initail to get the original class method */
    static classMethodType originalClassMethod = 0;
    if (originalClassMethod == 0) {
        /* Get the mangling name for this method via 'nm' */
        void *tmpPtr = dlsym(RTLD_NEXT, "_ZN5telux3loc12ReportReader11readCsvDataENSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEERSt13unordered_mapIS7_St6vectorIS7_SaIS7_EESt4hashIS7_ESt8equal_toIS7_ESaISt4pairIKS7_SB_EEESt10shared_ptrI22SimulationConfigParserE");
        /* Copy the method pointer to our storage */
        memcpy(&originalClassMethod, &tmpPtr, sizeof(&tmpPtr));
    }

    /* Do some thing different */
    reportCannedDataInit();

    /* Real method we want to call */
    (this->*originalClassMethod)(configVal, csvReportData, configParser);
}
