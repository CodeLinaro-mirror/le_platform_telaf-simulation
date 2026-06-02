/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file       ServingSystemDefines.hpp
 *
 * @brief      ServingSystemDefines contains enumerations and variables used for serving system
 *
 */

#ifndef TELUX_TEL_SERVINGSYSTEMDEFINES_HPP
#define TELUX_TEL_SERVINGSYSTEMDEFINES_HPP


namespace telux {
namespace tel {

// Forward declaration
class IServingSystemListener;

/** @addtogroup telematics_serving_system
 * @{ */

/**
 * Defines RF Bands.
 */
enum class RFBand {
   INVALID = -1,
   BC_0 = 0,
   BC_1 = 1,
   BC_3 = 3,
   BC_4 = 4,
   BC_5 = 5,
   BC_6 = 6,
   BC_7 = 7,
   BC_8 = 8,
   BC_9 = 9,
   BC_10 = 10,
   BC_11 = 11,
   BC_12 = 12,
   BC_13 = 13,
   BC_14 = 14,
   BC_15 = 15,
   BC_16 = 16,
   BC_17 = 17,
   BC_18 = 18,
   BC_19 = 19,
   GSM_450 = 40,
   GSM_480 = 41,
   GSM_750 = 42,
   GSM_850 = 43,
   GSM_900_EXTENDED = 44,
   GSM_900_PRIMARY = 45,
   GSM_900_RAILWAYS = 46,
   GSM_1800 = 47,
   GSM_1900 = 48,
   WCDMA_2100 = 80,
   WCDMA_PCS_1900 = 81,
   WCDMA_DCS_1800 = 82,
   WCDMA_1700_US = 83,
   WCDMA_850 = 84,
   WCDMA_800 = 85,
   WCDMA_2600 = 86,
   WCDMA_900 = 87,
   WCDMA_1700_JAPAN = 88,
   WCDMA_1500_JAPAN = 90,
   WCDMA_850_JAPAN = 91,
   E_UTRA_OPERATING_BAND_1 = 120,
   E_UTRA_OPERATING_BAND_2 = 121,
   E_UTRA_OPERATING_BAND_3 = 122,
   E_UTRA_OPERATING_BAND_4 = 123,
   E_UTRA_OPERATING_BAND_5 = 124,
   E_UTRA_OPERATING_BAND_6 = 125,
   E_UTRA_OPERATING_BAND_7 = 126,
   E_UTRA_OPERATING_BAND_8 = 127,
   E_UTRA_OPERATING_BAND_9 = 128,
   E_UTRA_OPERATING_BAND_10 = 129,
   E_UTRA_OPERATING_BAND_11 = 130,
   E_UTRA_OPERATING_BAND_12 = 131,
   E_UTRA_OPERATING_BAND_13 = 132,
   E_UTRA_OPERATING_BAND_14 = 133,
   E_UTRA_OPERATING_BAND_17 = 134,
   E_UTRA_OPERATING_BAND_33 = 135,
   E_UTRA_OPERATING_BAND_34 = 136,
   E_UTRA_OPERATING_BAND_35 = 137,
   E_UTRA_OPERATING_BAND_36 = 138,
   E_UTRA_OPERATING_BAND_37 = 139,
   E_UTRA_OPERATING_BAND_38 = 140,
   E_UTRA_OPERATING_BAND_39 = 141,
   E_UTRA_OPERATING_BAND_40 = 142,
   E_UTRA_OPERATING_BAND_18 = 143,
   E_UTRA_OPERATING_BAND_19 = 144,
   E_UTRA_OPERATING_BAND_20 = 145,
   E_UTRA_OPERATING_BAND_21 = 146,
   E_UTRA_OPERATING_BAND_24 = 147,
   E_UTRA_OPERATING_BAND_25 = 148,
   E_UTRA_OPERATING_BAND_41 = 149,
   E_UTRA_OPERATING_BAND_42 = 150,
   E_UTRA_OPERATING_BAND_43 = 151,
   E_UTRA_OPERATING_BAND_23 = 152,
   E_UTRA_OPERATING_BAND_26 = 153,
   E_UTRA_OPERATING_BAND_32 = 154,
   E_UTRA_OPERATING_BAND_125 = 155,
   E_UTRA_OPERATING_BAND_126 = 156,
   E_UTRA_OPERATING_BAND_127 = 157,
   E_UTRA_OPERATING_BAND_28 = 158,
   E_UTRA_OPERATING_BAND_29 = 159,
   E_UTRA_OPERATING_BAND_30 = 160,
   E_UTRA_OPERATING_BAND_66 = 161,
   E_UTRA_OPERATING_BAND_250 = 162,
   E_UTRA_OPERATING_BAND_46 = 163,
   E_UTRA_OPERATING_BAND_27 = 164,
   E_UTRA_OPERATING_BAND_31 = 165,
   E_UTRA_OPERATING_BAND_71 = 166,
   E_UTRA_OPERATING_BAND_47 = 167,
   E_UTRA_OPERATING_BAND_48 = 168,
   E_UTRA_OPERATING_BAND_67 = 169,
   E_UTRA_OPERATING_BAND_68 = 170,
   E_UTRA_OPERATING_BAND_49 = 171,
   E_UTRA_OPERATING_BAND_85 = 172,
   E_UTRA_OPERATING_BAND_72 = 173,
   E_UTRA_OPERATING_BAND_73 = 174,
   E_UTRA_OPERATING_BAND_86 = 175,
   E_UTRA_OPERATING_BAND_53 = 176,
   E_UTRA_OPERATING_BAND_87 = 177,
   E_UTRA_OPERATING_BAND_88 = 178,
   E_UTRA_OPERATING_BAND_70 = 179,
   TDSCDMA_BAND_A = 200,
   TDSCDMA_BAND_B = 201,
   TDSCDMA_BAND_C = 202,
   TDSCDMA_BAND_D = 203,
   TDSCDMA_BAND_E = 204,
   TDSCDMA_BAND_F = 205,
   NR5G_BAND_1 = 250,
   NR5G_BAND_2 = 251,
   NR5G_BAND_3 = 252,
   NR5G_BAND_5 = 253,
   NR5G_BAND_7 = 254,
   NR5G_BAND_8 = 255,
   NR5G_BAND_20 = 256,
   NR5G_BAND_28 = 257,
   NR5G_BAND_38 = 258,
   NR5G_BAND_41 = 259,
   NR5G_BAND_50 = 260,
   NR5G_BAND_51 = 261,
   NR5G_BAND_66 = 262,
   NR5G_BAND_70 = 263,
   NR5G_BAND_71 = 264,
   NR5G_BAND_74 = 265,
   NR5G_BAND_75 = 266,
   NR5G_BAND_76 = 267,
   NR5G_BAND_77 = 268,
   NR5G_BAND_78 = 269,
   NR5G_BAND_79 = 270,
   NR5G_BAND_80 = 271,
   NR5G_BAND_81 = 272,
   NR5G_BAND_82 = 273,
   NR5G_BAND_83 = 274,
   NR5G_BAND_84 = 275,
   NR5G_BAND_85 = 276,
   NR5G_BAND_257 = 277,
   NR5G_BAND_258 = 278,
   NR5G_BAND_259 = 279,
   NR5G_BAND_260 = 280,
   NR5G_BAND_261 = 281,
   NR5G_BAND_12 = 282,
   NR5G_BAND_25 = 283,
   NR5G_BAND_34 = 284,
   NR5G_BAND_39 = 285,
   NR5G_BAND_40 = 286,
   NR5G_BAND_65 = 287,
   NR5G_BAND_86 = 288,
   NR5G_BAND_48 = 289,
   NR5G_BAND_14 = 290,
   NR5G_BAND_13 = 291,
   NR5G_BAND_18 = 292,
   NR5G_BAND_26 = 293,
   NR5G_BAND_30 = 294,
   NR5G_BAND_29 = 295,
   NR5G_BAND_53 = 296,
   NR5G_BAND_46 = 297,
   NR5G_BAND_91 = 298,
   NR5G_BAND_92 = 299,
   NR5G_BAND_93 = 300,
   NR5G_BAND_94 = 301
};

/**
 * Defines RF Bandwidth Information.
 */

enum class RFBandWidth {
   INVALID_BANDWIDTH = -1,    /**<  Invalid Value */
   LTE_BW_NRB_6 = 0,          /**<  LTE 1.4 */
   LTE_BW_NRB_15 = 1,         /**<  LTE 3 */
   LTE_BW_NRB_25 = 2,         /**<  LTE 5  */
   LTE_BW_NRB_50 = 3,         /**<  LTE 10 */
   LTE_BW_NRB_75 = 4,         /**<  LTE 15 */
   LTE_BW_NRB_100 = 5,        /**<  LTE 20 */
   NR5G_BW_NRB_5 = 6,         /**<  NR5G 5 */
   NR5G_BW_NRB_10 = 7,        /**<  NR5G 10 */
   NR5G_BW_NRB_15 = 8,        /**<  NR5G 15 */
   NR5G_BW_NRB_20 = 9,        /**<  NR5G 20 */
   NR5G_BW_NRB_25 = 10,       /**<  NR5G 25 */
   NR5G_BW_NRB_30 = 11,       /**<  NR5G 30 */
   NR5G_BW_NRB_40 = 12,       /**<  NR5G 40 */
   NR5G_BW_NRB_50 = 13,       /**<  NR5G 50 */
   NR5G_BW_NRB_60 = 14,       /**<  NR5G 60 */
   NR5G_BW_NRB_80 = 15,       /**<  NR5G 80 */
   NR5G_BW_NRB_90 = 16,       /**<  NR5G 90 */
   NR5G_BW_NRB_100 = 17,      /**<  NR5G 100 */
   NR5G_BW_NRB_200 = 18,      /**<  NR5G 200 */
   NR5G_BW_NRB_400 = 19,      /**<  NR5G 400 */
   GSM_BW_NRB_2 = 20,         /**<  GSM  0.2 */
   TDSCDMA_BW_NRB_2 = 21,     /**<  TDSCDMA 1.6 */
   WCDMA_BW_NRB_5 = 22,       /**<  WCDMA 5 */
   WCDMA_BW_NRB_10 = 23,      /**<  WCDMA 10 */
   NR5G_BW_NRB_70 = 24        /**<  NR5G 70 */
};

/**
 * Defines GSM RF Bands.
 */
enum class GsmRFBand {
   GSM_INVALID = -1,      /**<  Invalid GSM band */
   GSM_450 = 1,           /**<  GSM 450 band */
   GSM_480 = 2,           /**<  GSM 480 band */
   GSM_750 = 3,           /**<  GSM 750 band */
   GSM_850 = 4,           /**<  GSM 850 band */
   GSM_900_EXTENDED = 5,  /**<  GSM 900 EXTENDED band */
   GSM_900_PRIMARY = 6,   /**<  GSM 900 PRIMARY band */
   GSM_900_RAILWAYS = 7,  /**<  GSM 900 RAILWAYS band */
   GSM_1800 = 8,          /**<  GSM 1800 band */
   GSM_1900 = 9           /**<  GSM 1900 band */
};

/**
 * Defines WCDMA RF Bands.
 */
enum class WcdmaRFBand {
   WCDMA_INVALID = -1,    /**<  Invalid WCDMA band */
   WCDMA_2100 = 1,        /**<  WCDMA 2100 band */
   WCDMA_PCS_1900 = 2,    /**<  WCDMA PCS 1900 band */
   WCDMA_DCS_1800 = 3,    /**<  WCDMA DCS 1800 band */
   WCDMA_1700_US = 4,     /**<  WCDMA 1700 US band */
   WCDMA_850 = 5,         /**<  WCDMA 850 band */
   WCDMA_800 = 6,         /**<  WCDMA 800 band */
   WCDMA_2600 = 7,        /**<  WCDMA 2600 band */
   WCDMA_900 = 8,         /**<  WCDMA 900 band */
   WCDMA_1700_JAPAN = 9,  /**<  WCDMA 1700 JAPAN band */
   WCDMA_1500_JAPAN = 10, /**<  WCDMA 1500 JAPAN band */
   WCDMA_850_JAPAN = 11   /**<  WCDMA 850 JAPAN band */
};

/**
 * Defines LTE RF Bands.
 */
enum class LteRFBand {
   E_UTRA_BAND_INVALID = -1,  /**<  Invalid LTE band */
   E_UTRA_BAND_1 = 1,         /**<  E-UTRA operating band 1 */
   E_UTRA_BAND_2 = 2,         /**<  E-UTRA operating band 2 */
   E_UTRA_BAND_3 = 3,         /**<  E-UTRA operating band 3 */
   E_UTRA_BAND_4 = 4,         /**<  E-UTRA operating band 4 */
   E_UTRA_BAND_5 = 5,         /**<  E-UTRA operating band 5 */
   E_UTRA_BAND_6 = 6,         /**<  E-UTRA operating band 6 */
   E_UTRA_BAND_7 = 7,         /**<  E-UTRA operating band 7 */
   E_UTRA_BAND_8 = 8,         /**<  E-UTRA operating band 8 */
   E_UTRA_BAND_9 = 9,         /**<  E-UTRA operating band 9 */
   E_UTRA_BAND_10 = 10,       /**<  E-UTRA operating band 10 */
   E_UTRA_BAND_11 = 11,       /**<  E-UTRA operating band 11 */
   E_UTRA_BAND_12 = 12,       /**<  E-UTRA operating band 12 */
   E_UTRA_BAND_13 = 13,       /**<  E-UTRA operating band 13 */
   E_UTRA_BAND_14 = 14,       /**<  E-UTRA operating band 14 */
   E_UTRA_BAND_17 = 17,       /**<  E-UTRA operating band 17 */
   E_UTRA_BAND_18 = 18,       /**<  E-UTRA operating band 18 */
   E_UTRA_BAND_19 = 19,       /**<  E-UTRA operating band 19 */
   E_UTRA_BAND_20 = 20,       /**<  E-UTRA operating band 20 */
   E_UTRA_BAND_21 = 21,       /**<  E-UTRA operating band 21 */
   E_UTRA_BAND_23 = 23,       /**<  E-UTRA operating band 23 */
   E_UTRA_BAND_24 = 24,       /**<  E-UTRA operating band 24 */
   E_UTRA_BAND_25 = 25,       /**<  E-UTRA operating band 25 */
   E_UTRA_BAND_26 = 26,       /**<  E-UTRA operating band 26 */
   E_UTRA_BAND_27 = 27,       /**<  E-UTRA operating band 27 */
   E_UTRA_BAND_28 = 28,       /**<  E-UTRA operating band 28 */
   E_UTRA_BAND_29 = 29,       /**<  E-UTRA operating band 29 */
   E_UTRA_BAND_30 = 30,       /**<  E-UTRA operating band 30 */
   E_UTRA_BAND_31 = 31,       /**<  E-UTRA operating band 31 */
   E_UTRA_BAND_32 = 32,       /**<  E-UTRA operating band 32 */
   E_UTRA_BAND_33 = 33,       /**<  E-UTRA operating band 33 */
   E_UTRA_BAND_34 = 34,       /**<  E-UTRA operating band 34 */
   E_UTRA_BAND_35 = 35,       /**<  E-UTRA operating band 35 */
   E_UTRA_BAND_36 = 36,       /**<  E-UTRA operating band 36 */
   E_UTRA_BAND_37 = 37,       /**<  E-UTRA operating band 37 */
   E_UTRA_BAND_38 = 38,       /**<  E-UTRA operating band 38 */
   E_UTRA_BAND_39 = 39,       /**<  E-UTRA operating band 39 */
   E_UTRA_BAND_40 = 40,       /**<  E-UTRA operating band 40 */
   E_UTRA_BAND_41 = 41,       /**<  E-UTRA operating band 41 */
   E_UTRA_BAND_42 = 42,       /**<  E-UTRA operating band 42 */
   E_UTRA_BAND_43 = 43,       /**<  E-UTRA operating band 43 */
   E_UTRA_BAND_44 = 44,       /**<  E-UTRA operating band 44 */
   E_UTRA_BAND_46 = 46,       /**<  E-UTRA operating band 46 */
   E_UTRA_BAND_47 = 47,       /**<  E-UTRA operating band 47 */
   E_UTRA_BAND_48 = 48,       /**<  E-UTRA operating band 48 */
   E_UTRA_BAND_49 = 49,       /**<  E-UTRA operating band 49 */
   E_UTRA_BAND_53 = 53,       /**<  E-UTRA operating band 53 */
   E_UTRA_BAND_60 = 60,       /**<  E-UTRA operating band 60 */
   E_UTRA_BAND_66 = 66,       /**<  E-UTRA operating band 66 */
   E_UTRA_BAND_67 = 67,       /**<  E-UTRA operating band 67 */
   E_UTRA_BAND_68 = 68,       /**<  E-UTRA operating band 68 */
   E_UTRA_BAND_70 = 70,       /**<  E-UTRA operating band 70 */
   E_UTRA_BAND_71 = 71,       /**<  E-UTRA operating band 71 */
   E_UTRA_BAND_85 = 85,       /**<  E-UTRA operating band 85 */
   E_UTRA_BAND_106 = 106,     /**<  E-UTRA operating band 106 */
   E_UTRA_BAND_125 = 125,     /**<  E-UTRA operating band 125 */
   E_UTRA_BAND_126 = 126,     /**<  E-UTRA operating band 126 */
   E_UTRA_BAND_127 = 127,     /**<  E-UTRA operating band 127 */
   E_UTRA_BAND_250 = 250,     /**<  E-UTRA operating band 250 */
   E_UTRA_BAND_252 = 252,     /**<  E-UTRA operating band 252 */
   E_UTRA_BAND_253 = 253,     /**<  E-UTRA operating band 253 */
   E_UTRA_BAND_255 = 255,     /**<  E-UTRA operating band 255 */
   E_UTRA_BAND_256 = 256      /**<  E-UTRA operating band 256 */
};

/**
 * Defines NR RF Bands, which are available for both SA and NSA modes.
 */
enum class NrRFBand {
   NR5G_BAND_INVALID = -1,  /**<  Invalid NR5G band */
   NR5G_BAND_1 = 1,         /**<  NR5G band 1 */
   NR5G_BAND_2 = 2,         /**<  NR5G band 2 */
   NR5G_BAND_3 = 3,         /**<  NR5G band 3 */
   NR5G_BAND_5 = 5,         /**<  NR5G band 5 */
   NR5G_BAND_7 = 7,         /**<  NR5G band 7 */
   NR5G_BAND_8 = 8,         /**<  NR5G band 8 */
   NR5G_BAND_12 = 12,       /**<  NR5G band 12 */
   NR5G_BAND_13 = 13,       /**<  NR5G band 13 */
   NR5G_BAND_14 = 14,       /**<  NR5G band 14 */
   NR5G_BAND_18 = 18,       /**<  NR5G band 18 */
   NR5G_BAND_20 = 20,       /**<  NR5G band 20 */
   NR5G_BAND_25 = 25,       /**<  NR5G band 25 */
   NR5G_BAND_26 = 26,       /**<  NR5G band 26 */
   NR5G_BAND_28 = 28,       /**<  NR5G band 28 */
   NR5G_BAND_29 = 29,       /**<  NR5G band 29 */
   NR5G_BAND_30 = 30,       /**<  NR5G band 30 */
   NR5G_BAND_34 = 34,       /**<  NR5G band 34 */
   NR5G_BAND_38 = 38,       /**<  NR5G band 38 */
   NR5G_BAND_39 = 39,       /**<  NR5G band 39 */
   NR5G_BAND_40 = 40,       /**<  NR5G band 40 */
   NR5G_BAND_41 = 41,       /**<  NR5G band 41 */
   NR5G_BAND_46 = 46,       /**<  NR5G band 46 */
   NR5G_BAND_47 = 47,       /**<  NR5G band 47 */
   NR5G_BAND_48 = 48,       /**<  NR5G band 48 */
   NR5G_BAND_50 = 50,       /**<  NR5G band 50 */
   NR5G_BAND_51 = 51,       /**<  NR5G band 51 */
   NR5G_BAND_53 = 53,       /**<  NR5G band 53 */
   NR5G_BAND_65 = 65,       /**<  NR5G band 65 */
   NR5G_BAND_66 = 66,       /**<  NR5G band 66 */
   NR5G_BAND_67 = 67,       /**<  NR5G band 67 */
   NR5G_BAND_68 = 68,       /**<  NR5G band 68 */
   NR5G_BAND_70 = 70,       /**<  NR5G band 70 */
   NR5G_BAND_71 = 71,       /**<  NR5G band 71 */
   NR5G_BAND_74 = 74,       /**<  NR5G band 74 */
   NR5G_BAND_75 = 75,       /**<  NR5G band 75 */
   NR5G_BAND_76 = 76,       /**<  NR5G band 76 */
   NR5G_BAND_77 = 77,       /**<  NR5G band 77 */
   NR5G_BAND_78 = 78,       /**<  NR5G band 78 */
   NR5G_BAND_79 = 79,       /**<  NR5G band 79 */
   NR5G_BAND_80 = 80,       /**<  NR5G band 80 */
   NR5G_BAND_81 = 81,       /**<  NR5G band 81 */
   NR5G_BAND_82 = 82,       /**<  NR5G band 82 */
   NR5G_BAND_83 = 83,       /**<  NR5G band 83 */
   NR5G_BAND_84 = 84,       /**<  NR5G band 84 */
   NR5G_BAND_85 = 85,       /**<  NR5G band 85 */
   NR5G_BAND_86 = 86,       /**<  NR5G band 86 */
   NR5G_BAND_89 = 89,       /**<  NR5G band 89 */
   NR5G_BAND_91 = 91,       /**<  NR5G band 91 */
   NR5G_BAND_92 = 92,       /**<  NR5G band 92 */
   NR5G_BAND_93 = 93,       /**<  NR5G band 93 */
   NR5G_BAND_94 = 94,       /**<  NR5G band 94 */
   NR5G_BAND_95 = 95,       /**<  NR5G band 95 */
   NR5G_BAND_96 = 96,       /**<  NR5G band 96 */
   NR5G_BAND_97 = 97,       /**<  NR5G band 97 */
   NR5G_BAND_98 = 98,       /**<  NR5G band 98 */
   NR5G_BAND_99 = 99,       /**<  NR5G band 99 */
   NR5G_BAND_102 = 102,     /**<  NR5G band 102 */
   NR5G_BAND_104 = 104,     /**<  NR5G band 104 */
   NR5G_BAND_105 = 105,     /**<  NR5G band 105 */
   NR5G_BAND_106 = 106,     /**<  NR5G band 106 */
   NR5G_BAND_257 = 257,     /**<  NR5G band 257 */
   NR5G_BAND_258 = 258,     /**<  NR5G band 258 */
   NR5G_BAND_259 = 259,     /**<  NR5G band 259 */
   NR5G_BAND_260 = 260,     /**<  NR5G band 260 */
   NR5G_BAND_261 = 261      /**<  NR5G band 261 */
} ;

/** @} */ /* end_addtogroup telematics_serving_system */
}
}

#endif // TELUX_TEL_SERVINGSYSTEMDEFINES_HPP
