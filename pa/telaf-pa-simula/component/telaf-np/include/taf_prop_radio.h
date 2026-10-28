/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TAF_PROP_RADIO_H
#define TAF_PROP_RADIO_H

#include "taf_prop_common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TAF_PROP_RADIO_PCI_SCAN_CELL_MAX_COUNT 5
#define TAF_PROP_RADIO_PCI_SCAN_PLMN_ID_MAX_COUNT 6
#define TAF_PROP_RADIO_DATA_AVAIL_SYS_MAX_COUNT 15
#define TAF_PROP_RADIO_LTE_CPHY_SCELL_INFO_MAX_COUNT 10

#define TAF_PROP_RADIO_BITMASK_RAT_GSM 0x1
#define TAF_PROP_RADIO_BITMASK_RAT_CDMA 0x2
#define TAF_PROP_RADIO_BITMASK_RAT_UMTS 0x4
#define TAF_PROP_RADIO_BITMASK_RAT_TDSCDMA 0x8
#define TAF_PROP_RADIO_BITMASK_RAT_LTE 0x10
#define TAF_PROP_RADIO_BITMASK_RAT_NR5G 0x20
typedef uint64_t taf_prop_radio_RatBitMask_t;

#define TAF_PROP_RADIO_BITMASK_SO_5G_NSA 0x80000000000
typedef uint64_t taf_prop_radio_SoBitMask_t;

typedef enum
{
    TAF_PROP_RADIO_SYS_INFO_IND_LIMIT_NONE = 0,
    TAF_PROP_RADIO_SYS_INFO_IND_LIMIT_BY_STATE_TOGGLE = (1 << 0),
    TAF_PROP_RADIO_SYS_INFO_IND_LIMIT_BY_SRV_STATUS = (1 << 1)
} taf_prop_radio_SysInfoIndLimitMask_t;

typedef enum
{
    TAF_PROP_RADIO_DISABLE_IND_MODE_NONE,
    TAF_PROP_RADIO_DISABLE_IND_MODE_ALL,
    TAF_PROP_RADIO_DISABLE_IND_MODE_SKIP_NAS_SYS_INFO_IND
} taf_prop_radio_DisableIndicationMode_t;

typedef enum
{
    TAF_PROP_RADIO_RAT_UNKNOWN = 0,
    TAF_PROP_RADIO_RAT_GSM = 1,
    TAF_PROP_RADIO_RAT_CDMA = 2,
    TAF_PROP_RADIO_RAT_UMTS = 3,
    TAF_PROP_RADIO_RAT_TDSCDMA = 4,
    TAF_PROP_RADIO_RAT_LTE = 5,
    TAF_PROP_RADIO_RAT_NR5G = 6
} taf_prop_radio_Rat_t;

typedef enum
{
    TAF_PROP_RADIO_RAT_SERVICE_STATUS_UNKNOWN = 0,
    TAF_PROP_RADIO_RAT_SERVICE_STATUS_NO_SERVICE = 1,
    TAF_PROP_RADIO_RAT_SERVICE_STATUS_LIMITED = 2,
    TAF_PROP_RADIO_RAT_SERVICE_STATUS_SERVICE = 3,
    TAF_PROP_RADIO_RAT_SERVICE_STATUS_LIMITED_REGIONAL = 4,
    TAF_PROP_RADIO_RAT_SERVICE_STATUS_POWER_SAVE = 5
} taf_prop_radio_RatServiceStatus_t;

typedef enum
{
    TAF_PROP_RADIO_LTE_CPHY_CA_BANDWIDTH_UNKNOWN = 0,
    TAF_PROP_RADIO_LTE_CPHY_CA_BANDWIDTH_NRB_6 = 1,
    TAF_PROP_RADIO_LTE_CPHY_CA_BANDWIDTH_NRB_15 = 2,
    TAF_PROP_RADIO_LTE_CPHY_CA_BANDWIDTH_NRB_25 = 3,
    TAF_PROP_RADIO_LTE_CPHY_CA_BANDWIDTH_NRB_50 = 4,
    TAF_PROP_RADIO_LTE_CPHY_CA_BANDWIDTH_NRB_75 = 5,
    TAF_PROP_RADIO_LTE_CPHY_CA_BANDWIDTH_NRB_100 = 6
} taf_prop_radio_LteCphyCaBandwidth_t;

typedef enum
{
    TAF_PROP_RADIO_LTE_CPHY_SCELL_STATE_UNKNOWN = 0,
    TAF_PROP_RADIO_LTE_CPHY_SCELL_STATE_DECONFIGURED = 1,
    TAF_PROP_RADIO_LTE_CPHY_SCELL_STATE_CONFIGURED_DEACTIVATED = 2,
    TAF_PROP_RADIO_LTE_CPHY_SCELL_STATE_CONFIGURED_ACTIVATED = 3
} taf_prop_radio_LteCphyScellState_t;

typedef enum
{
    TAF_PROP_RADIO_DATA_ROAMING_STATUS_UNKNOWN = 0,
    TAF_PROP_RADIO_DATA_ROAMING_STATUS_ON = 1,
    TAF_PROP_RADIO_DATA_ROAMING_STATUS_OFF = 2
} taf_prop_radio_DataRoamingStatus_t;

typedef struct
{
    uint16_t pci;
    uint32_t freq;
    taf_prop_radio_LteCphyCaBandwidth_t cphyCaDlBandwidth;
    uint32_t band;
} taf_prop_radio_LteCphyPcellInfo_t;

typedef struct
{
    uint16_t pci;
    uint32_t freq;
    taf_prop_radio_LteCphyCaBandwidth_t cphyCaDlBandwidth;
    uint32_t band;
    taf_prop_radio_LteCphyScellState_t scellState;
    uint8_t scellIndex;
    uint8_t ulConfigured;
} taf_prop_radio_LteCphyScellInfo_t;

typedef struct
{
    taf_prop_radio_LteCphyPcellInfo_t pcellInfo;
    uint32_t scellInfoCount;
    taf_prop_radio_LteCphyScellInfo_t scellInfo[TAF_PROP_RADIO_LTE_CPHY_SCELL_INFO_MAX_COUNT];
} taf_prop_radio_LteCphyCaInfo_t;

typedef struct
{
    uint16_t mcc;
    uint16_t mnc;
    uint8_t mncIncludesPcsDigit;
} taf_prop_radio_PlmnId_t;

typedef struct
{
    uint16_t cellId;
    uint32_t globalCellId;
    uint32_t plmnCount;
    taf_prop_radio_PlmnId_t plmnId[TAF_PROP_RADIO_PCI_SCAN_PLMN_ID_MAX_COUNT];
} taf_prop_radio_PciCellInformation_t;

typedef struct
{
    uint32_t pciCellCount;
    taf_prop_radio_PciCellInformation_t pciCellInfo[TAF_PROP_RADIO_PCI_SCAN_CELL_MAX_COUNT];
} taf_prop_radio_PciScanInformation_t;

typedef struct
{
    taf_prop_radio_Rat_t rat;
    taf_prop_radio_SoBitMask_t soMask;
} taf_prop_radio_DataAvailSysStatusInfo_t;

typedef struct
{
    uint32_t availSysCount;
    taf_prop_radio_DataAvailSysStatusInfo_t availSysStatusInfo[TAF_PROP_RADIO_DATA_AVAIL_SYS_MAX_COUNT];
} taf_prop_radio_DataAvailSysStatus_t;

typedef struct
{
    uint8_t gsmSvcStatusValid;
    taf_prop_radio_RatServiceStatus_t gsmSvcStatus;
    uint8_t cdmaSvcStatusValid;
    taf_prop_radio_RatServiceStatus_t cdmaSvcStatus;
    uint8_t umtsSvcStatusValid;
    taf_prop_radio_RatServiceStatus_t umtsSvcStatus;
    uint8_t tdscdmaSvcStatusValid;
    taf_prop_radio_RatServiceStatus_t tdscdmaSvcStatus;
    uint8_t lteSvcStatusValid;
    taf_prop_radio_RatServiceStatus_t lteSvcStatus;
    uint8_t nr5gSvcStatusValid;
    taf_prop_radio_RatServiceStatus_t nr5gSvcStatus;
} taf_prop_radio_RatSvcStatusIndication_t;

typedef struct
{
    uint8_t pcellInfoValid;
    taf_prop_radio_LteCphyPcellInfo_t pcellInfo;
    uint8_t scellInfoValid;
    uint32_t scellInfoCount;
    taf_prop_radio_LteCphyScellInfo_t scellInfo[TAF_PROP_RADIO_LTE_CPHY_SCELL_INFO_MAX_COUNT];
} taf_prop_radio_LteCphyCaIndication_t;

typedef struct
{
    uint8_t availSysValid;
    taf_prop_radio_DataAvailSysStatus_t availSys;
} taf_prop_radio_DataAvailSysStatusIndication_t;

typedef struct taf_prop_radio_RatSvcStatusHandler* taf_prop_radio_RatSvcStatusHandlerRef_t;

typedef struct taf_prop_radio_LteCphyCaHandler* taf_prop_radio_LteCphyCaHandlerRef_t;

typedef struct taf_prop_radio_DataAvailSysStatusHandler*
    taf_prop_radio_DataAvailSysStatusHandlerRef_t;

typedef void (*taf_prop_radio_RatSvcStatusHdlrFunc_t)
(
    uint32_t instance,
    taf_prop_radio_RatSvcStatusIndication_t indication,
    void* contextPtr
);

typedef void (*taf_prop_radio_LteCphyCaHdlrFunc_t)
(
    uint32_t instance,
    taf_prop_radio_LteCphyCaIndication_t indication,
    void* contextPtr
);

typedef void (*taf_prop_radio_DataAvailSysStatusHdlrFunc_t)
(
    uint32_t instance,
    taf_prop_radio_DataAvailSysStatusIndication_t indication,
    void* contextPtr
);

PROP_SHARED int32_t taf_prop_radio_Init
(
    void
);

PROP_SHARED int32_t taf_prop_radio_InitInstance
(
    uint32_t instance
);

PROP_SHARED int32_t taf_prop_radio_RegisterIndication
(
    uint32_t instance,
    uint8_t registration,
    taf_prop_radio_DisableIndicationMode_t mode
);

PROP_SHARED int32_t taf_prop_radio_SetSysInfoIndLimit
(
    uint32_t instance,
    taf_prop_radio_SysInfoIndLimitMask_t limitMask
);

PROP_SHARED int32_t taf_prop_radio_GetServiceStatus
(
    uint32_t instance,
    taf_prop_radio_Rat_t *servingRat,
    taf_prop_radio_RatServiceStatus_t* status
);

PROP_SHARED int32_t taf_prop_radio_GetSysInfoIndLimit
(
    uint32_t instance,
    taf_prop_radio_SysInfoIndLimitMask_t *limitMask
);

PROP_SHARED int32_t taf_prop_radio_PerformPciNetworkScan
(
    uint32_t instance,
    taf_prop_radio_RatBitMask_t bitmask,
    taf_prop_radio_PciScanInformation_t* informationPtr
);

PROP_SHARED int32_t taf_prop_radio_GetServingRat
(
    uint32_t instance,
    taf_prop_radio_Rat_t* ratPtr
);

PROP_SHARED int32_t taf_prop_radio_GetRatSvcStatus
(
    uint32_t instance,
    taf_prop_radio_Rat_t rat,
    taf_prop_radio_RatServiceStatus_t* statusPtr
);

PROP_SHARED int32_t taf_prop_radio_GetServingCellRac
(
    uint32_t instance,
    taf_prop_radio_Rat_t rat,
    uint8_t* racPtr
);

PROP_SHARED int32_t taf_prop_radio_GetDataAvailSysStatus
(
    uint32_t instance,
    taf_prop_radio_DataAvailSysStatus_t* statusPtr
);

PROP_SHARED int32_t taf_prop_radio_GetLteCphyCaInfo
(
    uint32_t instance,
    taf_prop_radio_LteCphyCaInfo_t* infoPtr
);

PROP_SHARED taf_prop_radio_RatSvcStatusHandlerRef_t taf_prop_radio_AddRatSvcStatusHandler
(
    uint32_t instance,
    taf_prop_radio_RatSvcStatusHdlrFunc_t handlerFuncPtr,
    void* contextPtr
);

PROP_SHARED taf_prop_radio_LteCphyCaHandlerRef_t taf_prop_radio_AddLteCphyCaHandler
(
    uint32_t instance,
    taf_prop_radio_LteCphyCaHdlrFunc_t handlerFuncPtr,
    void* contextPtr
);

PROP_SHARED taf_prop_radio_DataAvailSysStatusHandlerRef_t taf_prop_radio_AddDataAvailSysStatusHandler
(
    uint32_t instance,
    taf_prop_radio_DataAvailSysStatusHdlrFunc_t handlerFuncPtr,
    void* contextPtr
);

PROP_SHARED int32_t taf_prop_radio_GetDataCurrRoamingStatus
(
    uint32_t instance,
    taf_prop_radio_DataRoamingStatus_t* statusPtr
);

#ifdef __cplusplus
}
#endif

#endif /* TAF_PROP_RADIO_H */
