/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include "legato.h"
#include "pa_wdog.h"

#include <stdio.h>
#include <signal.h>
#include <sys/time.h>

#define TIMEOUT 120

static struct itimerval timer;

static void TafSigTermEventHandler(int tafSigNum)
{
    LE_CRIT("Terminated");
    exit(-tafSigNum);
}


void pa_wdog_Shutdown(void)
{
    LE_FATAL("TAF PA: Watchdog timer is expired. Restart the device.");
}


void pa_wdog_Kick(void)
{
    setitimer(ITIMER_REAL, &timer, NULL);
}

static void sig_handler(int signo)
{
    LE_FATAL("External wdog bite, stop telaf framework.");
}


void pa_wdog_Init(void)
{
    signal(SIGALRM, sig_handler);

    timer.it_value.tv_sec = TIMEOUT;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;

    pa_wdog_Kick();
}


COMPONENT_INIT
{
    // Block the signal
    le_sig_Block(SIGTERM);

    // Setup signal's event handler.
    le_sig_SetEventHandler(SIGTERM, TafSigTermEventHandler);
    LE_INFO("TAF PA: COMPONENT_INIT success, watchdog PA is ready.");
}

