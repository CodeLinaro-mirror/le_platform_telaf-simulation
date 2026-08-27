// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// taf_prop_pa_pms.cpp -- pure-MQTT wakeup-source filter sublayer.
//
// The flat extern "C" prop API is synchronous (SetWsFilter/GetWsFilter return
// the modem's answer, with a TIMEOUT result), while IModemBridge::send_request
// is asynchronous and fires its callback on the bridge worker thread. The
// bridge is therefore driven through syncRequest() below, which blocks the
// caller on a std::promise the callback fulfills -- the same pattern as
// component/sim/MultiSimManager.cpp's fetchSlotStatus_.
//
// Behaviour mirrors power-ns/component/taf_prop_pms/taf_prop_pa_pms.cpp (the
// QMI PDC reference); the transport deliberately does not. There is no QMI
// anywhere in the simulation.
//
// This is the sublayer the strong tafPmsPa.cpp actually loads at runtime (its
// DT_NEEDED is libComponent_taf_prop_pa_pms.so, which SIMULA stages first).
//
// The filter bitset's ground truth lives in MPSS (sml/mpss/wakeup), not here:
// this layer holds no authoritative cache.

#include "taf_prop_pa_pms.hpp"
#include "taf_prop_common.h"

#include "ModemBridge.hpp"
#include "Envelope.hpp"

#include "generated/cpp/topics.h"

#include <chrono>
#include <future>
#include <memory>
#include <optional>

#include <nlohmann/json.hpp>

using telux::common::simula::Envelope;
using telux::common::simula::ModemBridge;
using telux::common::simula::makeRequestEnvelope;

namespace {

// Matches the RPC timeout the other simula domains use.
constexpr auto kRpcTimeout = std::chrono::milliseconds(3000);

// Every wakeup source this prop ABI knows about (4 bits; the ns-prefix ABI
// only had 3 -- MODEM_WS_NAS_SYS_INFO is prop-only).
constexpr int kAllWakeupSources =
  MODEM_WS_INCOMING_SMS | MODEM_WS_INCOMING_VCALL |
  MODEM_WS_SIM_PROFILE_SWAP | MODEM_WS_NAS_SYS_INFO;

// Opaque object behind taf_prop_pa_pms_MpssRef_t. Deliberately thin: it only
// carries the PA's error callback and proves the handle is live. Modem state
// is MPSS's, so there is nothing else to cache.
struct PmsCtx
{
    taf_prop_pa_pms_ErrCallback errCb = nullptr;
    void* errCbCtx = nullptr;
};

// One synchronous request/response round-trip. Returns std::nullopt if no
// response arrived in time (-> Result_TIMEOUT).
//
// The promise is held by shared_ptr so a late callback -- one that fires after
// this function has already given up -- writes into a still-live object rather
// than a dangling stack slot; set_value then throws future_error, which is
// swallowed exactly as MultiSimManager does.
std::optional<Envelope>
syncRequest(
  const char* reqTopic,
  const char* rspSchemaId,
  nlohmann::json data
)
{
    auto& bridge = ModemBridge::instance();

    auto prom = std::make_shared<std::promise<std::optional<Envelope>>>();
    auto fut  = prom->get_future();

    auto req = makeRequestEnvelope(bridge.currentPaId(), std::move(data));

    bridge.send_request(
      reqTopic,
      rspSchemaId,
      req,
      [prom](std::optional<Envelope> rsp) {
          try
          {
              prom->set_value(std::move(rsp));
          }
          catch (const std::future_error&)
          {
              // Callback fired after the caller timed out; drop it.
          }
      },
      kRpcTimeout
    );

    // Slightly beyond the bridge's own deadline: the bridge already answers
    // with nullopt on timeout, so this wait only guards against the callback
    // never running at all.
    if (fut.wait_for(kRpcTimeout + std::chrono::milliseconds(500))
          != std::future_status::ready)
    {
        return std::nullopt;
    }
    return fut.get();
}

}  // namespace

extern "C" {

taf_prop_pa_pms_Result_t
taf_prop_pa_pms_Init(
  taf_prop_pa_pms_MpssRef_t* mpssRefPtr,
  taf_prop_pa_pms_ErrCallback errCbFn,
  void* errCbCtx
)
{
    if (mpssRefPtr == nullptr)
    {
        PROP_ERROR("taf_prop_pa_pms_Init: null mpssRefPtr");
        return taf_prop_pa_pms_Result_BAD_PARAMETER;
    }
    if (*mpssRefPtr != nullptr)
    {
        PROP_WARN("taf_prop_pa_pms_Init: already initialized");
        return taf_prop_pa_pms_Result_OK;
    }

    // Idempotent; the bridge is a process-wide singleton shared with the
    // telux_* simulated SDK libraries.
    ModemBridge::instance().start();

    auto* ctx = new PmsCtx{errCbFn, errCbCtx};
    *mpssRefPtr = reinterpret_cast<taf_prop_pa_pms_MpssRef_t>(ctx);

    PROP_INFO("taf_prop_pa_pms init done (MQTT bridge)");
    return taf_prop_pa_pms_Result_OK;
}

taf_prop_pa_pms_Result_t
taf_prop_pa_pms_Deinit(taf_prop_pa_pms_MpssRef_t* mpssRefPtr)
{
    if (mpssRefPtr == nullptr || *mpssRefPtr == nullptr)
    {
        PROP_ERROR("taf_prop_pa_pms_Deinit: null reference");
        return taf_prop_pa_pms_Result_BAD_PARAMETER;
    }

    // The bridge itself is not stopped: it is shared process-wide.
    delete reinterpret_cast<PmsCtx*>(*mpssRefPtr);
    *mpssRefPtr = nullptr;

    PROP_INFO("taf_prop_pa_pms deinit done");
    return taf_prop_pa_pms_Result_OK;
}

taf_prop_pa_pms_Result_t
taf_prop_pa_pms_SetWsFilter(
  taf_prop_pa_pms_MpssRef_t mpssRef,
  taf_prop_pa_pms_ModemWakeupSource_t bitset
)
{
    if (mpssRef == nullptr)
    {
        PROP_ERROR("taf_prop_pa_pms_SetWsFilter: null reference");
        return taf_prop_pa_pms_Result_BAD_PARAMETER;
    }

    const int bits = static_cast<int>(bitset);
    if ((bits & ~kAllWakeupSources) != 0)
    {
        PROP_ERROR("taf_prop_pa_pms_SetWsFilter: unknown bits in 0x%04x", bits);
        return taf_prop_pa_pms_Result_BAD_PARAMETER;
    }

    nlohmann::json data = nlohmann::json::object();
    data["bitset"] = bits;

    auto rsp = syncRequest(
      topics::wakeup::set_ws_filter::req,
      "wakeup.set_ws_filter.rsp",
      std::move(data)
    );

    if (!rsp)
    {
        PROP_ERROR("taf_prop_pa_pms_SetWsFilter: no response from MPSS");
        return taf_prop_pa_pms_Result_TIMEOUT;
    }
    if (rsp->error)
    {
        PROP_ERROR("taf_prop_pa_pms_SetWsFilter: MPSS rejected bitset 0x%04x", bits);
        return taf_prop_pa_pms_Result_FAULT;
    }

    PROP_INFO("wakeup source filter set to 0x%04x", bits);
    return taf_prop_pa_pms_Result_OK;
}

taf_prop_pa_pms_Result_t
taf_prop_pa_pms_GetWsFilter(
  taf_prop_pa_pms_MpssRef_t mpssRef,
  taf_prop_pa_pms_ModemWakeupSource_t* bitset
)
{
    if (mpssRef == nullptr || bitset == nullptr)
    {
        PROP_ERROR("taf_prop_pa_pms_GetWsFilter: null reference or out param");
        return taf_prop_pa_pms_Result_BAD_PARAMETER;
    }

    auto rsp = syncRequest(
      topics::wakeup::get_ws_filter::req,
      "wakeup.get_ws_filter.rsp",
      nlohmann::json::object()
    );

    if (!rsp)
    {
        PROP_ERROR("taf_prop_pa_pms_GetWsFilter: no response from MPSS");
        return taf_prop_pa_pms_Result_TIMEOUT;
    }
    if (rsp->error || !rsp->data)
    {
        PROP_ERROR("taf_prop_pa_pms_GetWsFilter: MPSS failed to report the bitset");
        return taf_prop_pa_pms_Result_FAULT;
    }

    const int bits = rsp->data->value("bitset", 0);
    *bitset = static_cast<taf_prop_pa_pms_ModemWakeupSource_t>(bits);

    PROP_DEBUG("wakeup source filter read back as 0x%04x", bits);
    return taf_prop_pa_pms_Result_OK;
}

taf_prop_pa_pms_Result_t
taf_prop_pa_pms_EnableAllWs(taf_prop_pa_pms_MpssRef_t mpssRef)
{
    // "Enable all" is SetWsFilter with every known bit set -- no separate RPC,
    // so MPSS only ever has one way to learn the filter.
    return taf_prop_pa_pms_SetWsFilter(
      mpssRef,
      static_cast<taf_prop_pa_pms_ModemWakeupSource_t>(kAllWakeupSources)
    );
}

}  // extern "C"
