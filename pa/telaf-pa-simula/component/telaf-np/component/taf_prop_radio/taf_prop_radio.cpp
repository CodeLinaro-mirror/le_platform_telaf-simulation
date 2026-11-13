// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "taf_prop_radio.h"
#include "taf_prop_common.h"

#include "Envelope.hpp"
#include "ModemBridge.hpp"
#include "generated/cpp/topics.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <future>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

using telux::common::simula::Envelope;
using telux::common::simula::ModemBridge;
using telux::common::simula::makeRequestEnvelope;

namespace {

constexpr uint32_t kInstanceCount = 2;
constexpr auto kRpcTimeout = std::chrono::milliseconds(3000);
constexpr auto kRatPollInterval = std::chrono::milliseconds(200);
constexpr auto kRatPollStep = std::chrono::milliseconds(50);

struct Handler
{
    uint32_t instance = 0;
    void* fn = nullptr;
    void* ctx = nullptr;
};

struct InstanceState
{
    bool initialized = false;
    bool indications = false;
    bool sysInfoIndications = false;
    taf_prop_radio_SysInfoIndLimitMask_t limit = TAF_PROP_RADIO_SYS_INFO_IND_LIMIT_NONE;
};

struct RadioCtx
{
    std::mutex mutex;
    bool initialized = false;
    bool polling = false;
    std::thread pollThread;
    std::atomic<bool> pollStop{false};
    InstanceState instances[kInstanceCount];
    Handler ratSvc;
    Handler lteCphyCa;
    Handler dataAvail;
};

RadioCtx& ctx()
{
    static RadioCtx c;
    return c;
}

bool validInstance(uint32_t instance)
{
    return instance < kInstanceCount;
}

int32_t badParam(const char* what)
{
    TAF_PROP_ERROR("%s", what);
    return TAF_PROP_BAD_PARAMETER;
}

std::optional<Envelope> syncRequest(
    const char* reqTopic,
    const char* rspSchemaId,
    nlohmann::json data = nlohmann::json::object())
{
    auto& bridge = ModemBridge::instance();
    bridge.start();

    auto prom = std::make_shared<std::promise<std::optional<Envelope>>>();
    auto fut = prom->get_future();
    auto req = makeRequestEnvelope(bridge.currentPaId(), std::move(data));

    bridge.send_request(reqTopic, rspSchemaId, std::move(req),
        [prom](std::optional<Envelope> rsp) {
            try
            {
                prom->set_value(std::move(rsp));
            }
            catch (const std::future_error&)
            {
            }
        },
        kRpcTimeout);

    if (fut.wait_for(kRpcTimeout + std::chrono::milliseconds(500)) != std::future_status::ready)
        return std::nullopt;
    return fut.get();
}

std::optional<nlohmann::json> requestData(const char* topic, const char* schema,
                                           uint32_t instance)
{
    nlohmann::json req = nlohmann::json::object();
    req["slot"] = instance;
    auto rsp = syncRequest(topic, schema, std::move(req));
    if (!rsp)
    {
        TAF_PROP_DEBUG("MQTT request %s timed out", topic);
        return std::nullopt;
    }
    if (rsp->error || !rsp->data)
    {
        TAF_PROP_ERROR("MQTT request %s failed", topic);
        return std::nullopt;
    }
    return rsp->data;
}

taf_prop_radio_Rat_t ratFromWire(const std::string& rat)
{
    if (rat == "RADIO_TECH_GSM" || rat == "RADIO_TECH_GPRS" || rat == "RADIO_TECH_EDGE")
        return TAF_PROP_RADIO_RAT_GSM;
    if (rat == "RADIO_TECH_UMTS")
        return TAF_PROP_RADIO_RAT_UMTS;
    if (rat == "RADIO_TECH_TD_SCDMA")
        return TAF_PROP_RADIO_RAT_TDSCDMA;
    if (rat == "RADIO_TECH_LTE" || rat == "RADIO_TECH_LTE_CA")
        return TAF_PROP_RADIO_RAT_LTE;
    if (rat == "RADIO_TECH_NR5G")
        return TAF_PROP_RADIO_RAT_NR5G;
    return TAF_PROP_RADIO_RAT_UNKNOWN;
}

taf_prop_radio_RatServiceStatus_t statusFromWire(const std::string& state)
{
    if (state == "NO_SERVICE")
        return TAF_PROP_RADIO_RAT_SERVICE_STATUS_NO_SERVICE;
    if (state == "LIMITED_SERVICE")
        return TAF_PROP_RADIO_RAT_SERVICE_STATUS_LIMITED;
    if (state == "IN_SERVICE")
        return TAF_PROP_RADIO_RAT_SERVICE_STATUS_SERVICE;
    if (state == "LIMITED_REGIONAL")
        return TAF_PROP_RADIO_RAT_SERVICE_STATUS_LIMITED_REGIONAL;
    if (state == "POWER_SAVE")
        return TAF_PROP_RADIO_RAT_SERVICE_STATUS_POWER_SAVE;
    return TAF_PROP_RADIO_RAT_SERVICE_STATUS_UNKNOWN;
}

bool serviceAvailable(taf_prop_radio_RatServiceStatus_t status)
{
    return status == TAF_PROP_RADIO_RAT_SERVICE_STATUS_SERVICE ||
           status == TAF_PROP_RADIO_RAT_SERVICE_STATUS_LIMITED ||
           status == TAF_PROP_RADIO_RAT_SERVICE_STATUS_LIMITED_REGIONAL;
}

bool ratSvcChangeQualifies(
    taf_prop_radio_SysInfoIndLimitMask_t limit,
    taf_prop_radio_Rat_t rat, taf_prop_radio_RatServiceStatus_t status,
    taf_prop_radio_Rat_t lastRat, taf_prop_radio_RatServiceStatus_t lastStatus)
{
    bool ratChanged = rat != lastRat;
    bool statusChanged = status != lastStatus;
    if (limit == TAF_PROP_RADIO_SYS_INFO_IND_LIMIT_NONE)
        return ratChanged || statusChanged;

    bool qualifies = false;
    if (limit & TAF_PROP_RADIO_SYS_INFO_IND_LIMIT_BY_STATE_TOGGLE)
        qualifies = qualifies || (serviceAvailable(status) != serviceAvailable(lastStatus));
    if (limit & TAF_PROP_RADIO_SYS_INFO_IND_LIMIT_BY_SRV_STATUS)
        qualifies = qualifies || statusChanged;
    return qualifies;
}

void verifySysInfoIndLimitMasks()
{
    using L = taf_prop_radio_SysInfoIndLimitMask_t;
    constexpr auto kNoSvc = TAF_PROP_RADIO_RAT_SERVICE_STATUS_NO_SERVICE;
    constexpr auto kFull = TAF_PROP_RADIO_RAT_SERVICE_STATUS_SERVICE;
    constexpr auto kLimited = TAF_PROP_RADIO_RAT_SERVICE_STATUS_LIMITED;
    constexpr auto kLte = TAF_PROP_RADIO_RAT_LTE;
    constexpr auto kNr = TAF_PROP_RADIO_RAT_NR5G;

    // NONE: unrestricted, any RAT or status change qualifies.
    assert(ratSvcChangeQualifies(L{TAF_PROP_RADIO_SYS_INFO_IND_LIMIT_NONE}, kNr, kFull, kLte, kFull));
    assert(!ratSvcChangeQualifies(L{TAF_PROP_RADIO_SYS_INFO_IND_LIMIT_NONE}, kLte, kFull, kLte, kFull));

    constexpr auto kToggle = L{TAF_PROP_RADIO_SYS_INFO_IND_LIMIT_BY_STATE_TOGGLE};
    assert(ratSvcChangeQualifies(kToggle, kLte, kFull, kLte, kNoSvc));
    assert(!ratSvcChangeQualifies(kToggle, kLte, kFull, kLte, kLimited));
    assert(!ratSvcChangeQualifies(kToggle, kNr, kFull, kLte, kFull));

    constexpr auto kSrv = L{TAF_PROP_RADIO_SYS_INFO_IND_LIMIT_BY_SRV_STATUS};
    assert(ratSvcChangeQualifies(kSrv, kLte, kLimited, kLte, kFull));
    assert(!ratSvcChangeQualifies(kSrv, kNr, kFull, kLte, kFull));

    // Both bits set: either condition is sufficient.
    const auto kBoth = static_cast<L>(TAF_PROP_RADIO_SYS_INFO_IND_LIMIT_BY_STATE_TOGGLE |
                                       TAF_PROP_RADIO_SYS_INFO_IND_LIMIT_BY_SRV_STATUS);
    assert(ratSvcChangeQualifies(kBoth, kLte, kFull, kLte, kNoSvc));
    assert(ratSvcChangeQualifies(kBoth, kLte, kLimited, kLte, kFull));
    assert(!ratSvcChangeQualifies(kBoth, kNr, kFull, kLte, kFull));
}

taf_prop_radio_RatBitMask_t ratMask(taf_prop_radio_Rat_t rat)
{
    switch (rat)
    {
        case TAF_PROP_RADIO_RAT_GSM: return TAF_PROP_RADIO_BITMASK_RAT_GSM;
        case TAF_PROP_RADIO_RAT_CDMA: return TAF_PROP_RADIO_BITMASK_RAT_CDMA;
        case TAF_PROP_RADIO_RAT_UMTS: return TAF_PROP_RADIO_BITMASK_RAT_UMTS;
        case TAF_PROP_RADIO_RAT_TDSCDMA: return TAF_PROP_RADIO_BITMASK_RAT_TDSCDMA;
        case TAF_PROP_RADIO_RAT_LTE: return TAF_PROP_RADIO_BITMASK_RAT_LTE;
        case TAF_PROP_RADIO_RAT_NR5G: return TAF_PROP_RADIO_BITMASK_RAT_NR5G;
        default: return 0;
    }
}

bool getSystemInfo(uint32_t instance, taf_prop_radio_Rat_t* rat,
                   taf_prop_radio_RatServiceStatus_t* status)
{
    auto data = requestData(topics::radio::get_system_info::req,
                            "radio.get_system_info.rsp", instance);
    if (!data)
        return false;
    if (rat)
        *rat = ratFromWire(data->value("rat", std::string("RADIO_TECH_UNKNOWN")));
    if (status)
        *status = statusFromWire(data->value("state", std::string("UNKNOWN")));
    return true;
}

uint32_t bandFromWire(const std::string& band)
{
    const std::string ltePrefix = "E_UTRA_OPERATING_BAND_";
    const std::string nrPrefix = "NR5G_BAND_";
    auto parseSuffix = [](const std::string& s, const std::string& p) -> uint32_t {
        if (s.rfind(p, 0) != 0)
            return 0;
        try
        {
            return static_cast<uint32_t>(std::stoul(s.substr(p.size())));
        }
        catch (...)
        {
            return 0;
        }
    };
    uint32_t n = parseSuffix(band, ltePrefix);
    if (n != 0)
        return n;
    return parseSuffix(band, nrPrefix);
}

taf_prop_radio_LteCphyCaBandwidth_t bandwidthFromWire(const std::string& bw)
{
    if (bw == "LTE_BW_NRB_6") return TAF_PROP_RADIO_LTE_CPHY_CA_BANDWIDTH_NRB_6;
    if (bw == "LTE_BW_NRB_15") return TAF_PROP_RADIO_LTE_CPHY_CA_BANDWIDTH_NRB_15;
    if (bw == "LTE_BW_NRB_25") return TAF_PROP_RADIO_LTE_CPHY_CA_BANDWIDTH_NRB_25;
    if (bw == "LTE_BW_NRB_50") return TAF_PROP_RADIO_LTE_CPHY_CA_BANDWIDTH_NRB_50;
    if (bw == "LTE_BW_NRB_75") return TAF_PROP_RADIO_LTE_CPHY_CA_BANDWIDTH_NRB_75;
    if (bw == "LTE_BW_NRB_100") return TAF_PROP_RADIO_LTE_CPHY_CA_BANDWIDTH_NRB_100;
    return TAF_PROP_RADIO_LTE_CPHY_CA_BANDWIDTH_UNKNOWN;
}

int parseIntString(const nlohmann::json& j, const char* key)
{
    if (!j.contains(key))
        return 0;
    if (j[key].is_number_integer())
        return j[key].get<int>();
    if (j[key].is_string())
    {
        try
        {
            return std::stoi(j[key].get<std::string>());
        }
        catch (...)
        {
        }
    }
    return 0;
}

std::optional<std::vector<nlohmann::json>> getCells(uint32_t instance)
{
    auto data = requestData(topics::radio::request_cell_info::req,
                            "radio.request_cell_info.rsp", instance);
    if (!data)
        return std::nullopt;
    if (!data->contains("cells") || !(*data)["cells"].is_array())
        return std::vector<nlohmann::json>{};
    return data->at("cells").get<std::vector<nlohmann::json>>();
}

bool computeDataAvailSysStatus(uint32_t instance, taf_prop_radio_DataAvailSysStatus_t* statusPtr)
{
    std::memset(statusPtr, 0, sizeof(*statusPtr));
    taf_prop_radio_Rat_t rat = TAF_PROP_RADIO_RAT_UNKNOWN;
    taf_prop_radio_RatServiceStatus_t svc = TAF_PROP_RADIO_RAT_SERVICE_STATUS_UNKNOWN;
    if (!getSystemInfo(instance, &rat, &svc))
        return false;
    if (!serviceAvailable(svc) || rat == TAF_PROP_RADIO_RAT_UNKNOWN)
        return true;

    statusPtr->availSysCount = 1;
    statusPtr->availSysStatusInfo[0].rat = rat;
    statusPtr->availSysStatusInfo[0].soMask = 0;

    auto dc = requestData(topics::radio::get_dc_status::req, "radio.get_dc_status.rsp", instance);
    if (dc && rat == TAF_PROP_RADIO_RAT_LTE &&
        dc->value("endcAvailability", std::string()) == "AVAILABLE")
    {
        statusPtr->availSysStatusInfo[0].soMask = TAF_PROP_RADIO_BITMASK_SO_5G_NSA;
    }
    return true;
}

bool computeLteCphyCaInfo(uint32_t instance, taf_prop_radio_LteCphyCaInfo_t* infoPtr)
{
    std::memset(infoPtr, 0, sizeof(*infoPtr));
    bool foundLte = false;
    auto cells = getCells(instance).value_or(std::vector<nlohmann::json>{});
    for (const auto& cell : cells)
    {
        if (cell.value("registered", false) && cell.contains("lte"))
        {
            const auto& lte = cell["lte"];
            infoPtr->pcellInfo.pci = static_cast<uint16_t>(lte.value("pci", 0));
            infoPtr->pcellInfo.freq = static_cast<uint32_t>(lte.value("earfcn", 0));
            infoPtr->pcellInfo.cphyCaDlBandwidth = TAF_PROP_RADIO_LTE_CPHY_CA_BANDWIDTH_NRB_100;
            infoPtr->pcellInfo.band = 1;
            foundLte = true;
            break;
        }
    }

    if (!foundLte)
        return false;

    auto rf = requestData(topics::radio::request_rf_band_info::req,
                          "radio.request_rf_band_info.rsp", instance);
    if (rf)
    {
        infoPtr->pcellInfo.freq = static_cast<uint32_t>(rf->value("channel", static_cast<int>(infoPtr->pcellInfo.freq)));
        infoPtr->pcellInfo.band = bandFromWire(rf->value("band", std::string()));
        infoPtr->pcellInfo.cphyCaDlBandwidth = bandwidthFromWire(rf->value("bandWidth", std::string()));
    }
    return true;
}

void fillRatSvcIndication(
    taf_prop_radio_Rat_t rat,
    taf_prop_radio_RatServiceStatus_t status,
    taf_prop_radio_RatSvcStatusIndication_t* ind)
{
    std::memset(ind, 0, sizeof(*ind));
    auto set = [&](taf_prop_radio_Rat_t r, uint8_t& valid, taf_prop_radio_RatServiceStatus_t& out) {
        valid = 1;
        out = (r == rat) ? status : TAF_PROP_RADIO_RAT_SERVICE_STATUS_NO_SERVICE;
    };
    set(TAF_PROP_RADIO_RAT_GSM, ind->gsmSvcStatusValid, ind->gsmSvcStatus);
    set(TAF_PROP_RADIO_RAT_CDMA, ind->cdmaSvcStatusValid, ind->cdmaSvcStatus);
    set(TAF_PROP_RADIO_RAT_UMTS, ind->umtsSvcStatusValid, ind->umtsSvcStatus);
    set(TAF_PROP_RADIO_RAT_TDSCDMA, ind->tdscdmaSvcStatusValid, ind->tdscdmaSvcStatus);
    set(TAF_PROP_RADIO_RAT_LTE, ind->lteSvcStatusValid, ind->lteSvcStatus);
    set(TAF_PROP_RADIO_RAT_NR5G, ind->nr5gSvcStatusValid, ind->nr5gSvcStatus);
}

void fireRatSvcHandler(taf_prop_radio_Rat_t rat, taf_prop_radio_RatServiceStatus_t status)
{
    Handler h;
    bool enabled = false;
    {
        std::lock_guard<std::mutex> lk(ctx().mutex);
        h = ctx().ratSvc;
        enabled = validInstance(h.instance) && ctx().instances[h.instance].sysInfoIndications;
    }
    if (!enabled || h.fn == nullptr)
        return;

    taf_prop_radio_RatSvcStatusIndication_t ind;
    fillRatSvcIndication(rat, status, &ind);
    TAF_PROP_INFO("taf_prop_radio RAT svc status indication: instance=%u rat=%d status=%d",
        h.instance, static_cast<int>(rat), static_cast<int>(status));
    reinterpret_cast<taf_prop_radio_RatSvcStatusHdlrFunc_t>(h.fn)(h.instance, ind, h.ctx);
}

void fireDataAvailHandler(const taf_prop_radio_DataAvailSysStatus_t& status)
{
    Handler h;
    bool enabled = false;
    {
        std::lock_guard<std::mutex> lk(ctx().mutex);
        h = ctx().dataAvail;
        enabled = validInstance(h.instance) && ctx().instances[h.instance].indications;
    }
    if (!enabled || h.fn == nullptr)
        return;

    taf_prop_radio_DataAvailSysStatusIndication_t ind;
    std::memset(&ind, 0, sizeof(ind));
    ind.availSysValid = status.availSysCount > 0 ? 1 : 0;
    ind.availSys = status;
    TAF_PROP_INFO("taf_prop_radio data-avail-sys-status indication: instance=%u availSysCount=%u",
        h.instance, status.availSysCount);
    reinterpret_cast<taf_prop_radio_DataAvailSysStatusHdlrFunc_t>(h.fn)(h.instance, ind, h.ctx);
}

void fireLteCphyCaHandler(const taf_prop_radio_LteCphyCaInfo_t& info, bool pcellValid)
{
    Handler h;
    bool enabled = false;
    {
        std::lock_guard<std::mutex> lk(ctx().mutex);
        h = ctx().lteCphyCa;
        enabled = validInstance(h.instance) && ctx().instances[h.instance].indications;
    }
    if (!enabled || h.fn == nullptr)
        return;

    taf_prop_radio_LteCphyCaIndication_t ind;
    std::memset(&ind, 0, sizeof(ind));
    ind.pcellInfoValid = pcellValid ? 1 : 0;
    ind.pcellInfo = info.pcellInfo;
    ind.scellInfoValid = info.scellInfoCount > 0 ? 1 : 0;
    ind.scellInfoCount = info.scellInfoCount;
    TAF_PROP_INFO("taf_prop_radio LTE CPHY CA indication: instance=%u pcellValid=%u pci=%u freq=%u",
        h.instance, ind.pcellInfoValid, info.pcellInfo.pci, info.pcellInfo.freq);
    reinterpret_cast<taf_prop_radio_LteCphyCaHdlrFunc_t>(h.fn)(h.instance, ind, h.ctx);
}

void ratPollTick(
    taf_prop_radio_Rat_t& lastRat, taf_prop_radio_RatServiceStatus_t& lastStatus, bool& haveRat,
    taf_prop_radio_DataAvailSysStatus_t& lastAvail, bool& haveAvail,
    taf_prop_radio_LteCphyCaInfo_t& lastCa, bool& haveCa)
{
    Handler ratSvc, dataAvail, lteCphyCa;
    {
        std::lock_guard<std::mutex> lk(ctx().mutex);
        ratSvc = ctx().ratSvc;
        dataAvail = ctx().dataAvail;
        lteCphyCa = ctx().lteCphyCa;
    }

    if (ratSvc.fn != nullptr && validInstance(ratSvc.instance))
    {
        taf_prop_radio_Rat_t rat = TAF_PROP_RADIO_RAT_UNKNOWN;
        taf_prop_radio_RatServiceStatus_t status = TAF_PROP_RADIO_RAT_SERVICE_STATUS_UNKNOWN;
        taf_prop_radio_SysInfoIndLimitMask_t limit;
        {
            std::lock_guard<std::mutex> lk(ctx().mutex);
            limit = ctx().instances[ratSvc.instance].limit;
        }
        if (getSystemInfo(ratSvc.instance, &rat, &status) &&
            (!haveRat || ratSvcChangeQualifies(limit, rat, status, lastRat, lastStatus)))
        {
            haveRat = true;
            lastRat = rat;
            lastStatus = status;
            fireRatSvcHandler(rat, status);
        }
    }
    if (dataAvail.fn != nullptr && validInstance(dataAvail.instance))
    {
        taf_prop_radio_DataAvailSysStatus_t avail{};
        if (computeDataAvailSysStatus(dataAvail.instance, &avail) &&
            (!haveAvail || std::memcmp(&avail, &lastAvail, sizeof(avail)) != 0))
        {
            haveAvail = true;
            lastAvail = avail;
            fireDataAvailHandler(avail);
        }
    }
    if (lteCphyCa.fn != nullptr && validInstance(lteCphyCa.instance))
    {
        taf_prop_radio_LteCphyCaInfo_t ca{};
        bool pcellValid = computeLteCphyCaInfo(lteCphyCa.instance, &ca);
        if (!haveCa || std::memcmp(&ca, &lastCa, sizeof(ca)) != 0)
        {
            haveCa = true;
            lastCa = ca;
            fireLteCphyCaHandler(ca, pcellValid);
        }
    }
}

void ratPollLoop()
{
    taf_prop_radio_Rat_t lastRat = TAF_PROP_RADIO_RAT_UNKNOWN;
    taf_prop_radio_RatServiceStatus_t lastStatus = TAF_PROP_RADIO_RAT_SERVICE_STATUS_UNKNOWN;
    bool haveRat = false;
    taf_prop_radio_DataAvailSysStatus_t lastAvail{};
    bool haveAvail = false;
    taf_prop_radio_LteCphyCaInfo_t lastCa{};
    bool haveCa = false;

    while (!ctx().pollStop.load(std::memory_order_relaxed))
    {
        ratPollTick(lastRat, lastStatus, haveRat, lastAvail, haveAvail, lastCa, haveCa);
        for (int i = 0; i < kRatPollInterval / kRatPollStep &&
                        !ctx().pollStop.load(std::memory_order_relaxed); ++i)
            std::this_thread::sleep_for(kRatPollStep);
    }
}

void ensureRatPolling()
{
    auto& c = ctx();
    std::lock_guard<std::mutex> lk(c.mutex);
    if (c.polling)
        return;
    c.polling = true;
    c.pollStop.store(false, std::memory_order_relaxed);
    c.pollThread = std::thread(ratPollLoop);
    TAF_PROP_INFO("taf_prop_radio indication poll thread started (interval=%ldms)",
        static_cast<long>(kRatPollInterval.count()));
}

void stopRatPolling()
{
    auto& c = ctx();
    std::thread t;
    {
        std::lock_guard<std::mutex> lk(c.mutex);
        if (!c.polling)
            return;
        c.polling = false;
        c.pollStop.store(true, std::memory_order_relaxed);
        t = std::move(c.pollThread);
    }
    if (t.joinable())
        t.join();
    TAF_PROP_INFO("taf_prop_radio indication poll thread stopped");
}

} // namespace

extern "C" {

int32_t taf_prop_radio_Init(void)
{
    static std::once_flag verifyOnce;
    std::call_once(verifyOnce, verifySysInfoIndLimitMasks);
    ModemBridge::instance().start();
    ensureRatPolling();
    std::lock_guard<std::mutex> lk(ctx().mutex);
    ctx().initialized = true;
    TAF_PROP_INFO("taf_prop_radio init done (MQTT bridge)");
    return TAF_PROP_OK;
}

int32_t taf_prop_radio_Deinit(void)
{
    bool wasInitialized;
    {
        std::lock_guard<std::mutex> lk(ctx().mutex);
        wasInitialized = ctx().initialized;
        if (wasInitialized)
        {
            ctx().initialized = false;
            for (auto& instance : ctx().instances)
                instance = InstanceState{};
            ctx().ratSvc = Handler{};
            ctx().lteCphyCa = Handler{};
            ctx().dataAvail = Handler{};
        }
    }
    if (!wasInitialized)
    {
        TAF_PROP_WARN("taf_prop_radio_Deinit: called before a successful Init()");
        return TAF_PROP_BAD_PARAMETER;
    }
    stopRatPolling();
    TAF_PROP_INFO("taf_prop_radio deinit done");
    return TAF_PROP_OK;
}

int32_t taf_prop_radio_InitInstance(uint32_t instance)
{
    if (!validInstance(instance))
        return badParam("taf_prop_radio_InitInstance: invalid instance");
    taf_prop_radio_Init();
    std::lock_guard<std::mutex> lk(ctx().mutex);
    ctx().instances[instance].initialized = true;
    TAF_PROP_INFO("taf_prop_radio instance %u initialized", instance);
    return TAF_PROP_OK;
}

int32_t taf_prop_radio_RegisterIndication(
    uint32_t instance,
    uint8_t registration,
    taf_prop_radio_DisableIndicationMode_t mode)
{
    if (!validInstance(instance) || registration > 1)
        return badParam("taf_prop_radio_RegisterIndication: invalid argument");
    ensureRatPolling();
    std::lock_guard<std::mutex> lk(ctx().mutex);
    auto& s = ctx().instances[instance];
    s.indications = (registration != 0);
    s.sysInfoIndications = (registration != 0) ||
        (mode == TAF_PROP_RADIO_DISABLE_IND_MODE_SKIP_NAS_SYS_INFO_IND);
    TAF_PROP_INFO(
        "taf_prop_radio_RegisterIndication: instance=%u registration=%u mode=%d -> "
        "indications=%d sysInfoIndications=%d",
        instance, registration, static_cast<int>(mode), s.indications, s.sysInfoIndications);
    return TAF_PROP_OK;
}

int32_t taf_prop_radio_SetSysInfoIndLimit(
    uint32_t instance,
    taf_prop_radio_SysInfoIndLimitMask_t limitMask)
{
    if (!validInstance(instance))
        return badParam("taf_prop_radio_SetSysInfoIndLimit: invalid instance");
    std::lock_guard<std::mutex> lk(ctx().mutex);
    ctx().instances[instance].limit = limitMask;
    TAF_PROP_DEBUG("taf_prop_radio_SetSysInfoIndLimit: instance=%u limitMask=0x%x",
        instance, static_cast<unsigned>(limitMask));
    return TAF_PROP_OK;
}

int32_t taf_prop_radio_GetSysInfoIndLimit(
    uint32_t instance,
    taf_prop_radio_SysInfoIndLimitMask_t* limitMask)
{
    if (!validInstance(instance) || limitMask == nullptr)
        return badParam("taf_prop_radio_GetSysInfoIndLimit: invalid argument");
    std::lock_guard<std::mutex> lk(ctx().mutex);
    *limitMask = ctx().instances[instance].limit;
    TAF_PROP_DEBUG("taf_prop_radio_GetSysInfoIndLimit: instance=%u -> 0x%x",
        instance, static_cast<unsigned>(*limitMask));
    return TAF_PROP_OK;
}

int32_t taf_prop_radio_GetServiceStatus(
    uint32_t instance,
    taf_prop_radio_Rat_t* servingRat,
    taf_prop_radio_RatServiceStatus_t* status)
{
    if (!validInstance(instance) || servingRat == nullptr || status == nullptr)
        return badParam("taf_prop_radio_GetServiceStatus: invalid argument");
    if (!getSystemInfo(instance, servingRat, status))
        return TAF_PROP_TIMEOUT;
    TAF_PROP_DEBUG("taf_prop_radio_GetServiceStatus: instance=%u -> rat=%d status=%d",
        instance, static_cast<int>(*servingRat), static_cast<int>(*status));
    return TAF_PROP_OK;
}

int32_t taf_prop_radio_GetServingRat(uint32_t instance, taf_prop_radio_Rat_t* ratPtr)
{
    if (!validInstance(instance) || ratPtr == nullptr)
        return badParam("taf_prop_radio_GetServingRat: invalid argument");
    if (!getSystemInfo(instance, ratPtr, nullptr))
        return TAF_PROP_TIMEOUT;
    TAF_PROP_DEBUG("taf_prop_radio_GetServingRat: instance=%u -> rat=%d", instance, static_cast<int>(*ratPtr));
    return TAF_PROP_OK;
}

int32_t taf_prop_radio_GetRatSvcStatus(
    uint32_t instance,
    taf_prop_radio_Rat_t rat,
    taf_prop_radio_RatServiceStatus_t* statusPtr)
{
    if (!validInstance(instance) || statusPtr == nullptr)
        return badParam("taf_prop_radio_GetRatSvcStatus: invalid argument");
    taf_prop_radio_Rat_t currentRat = TAF_PROP_RADIO_RAT_UNKNOWN;
    taf_prop_radio_RatServiceStatus_t currentStatus = TAF_PROP_RADIO_RAT_SERVICE_STATUS_UNKNOWN;
    if (!getSystemInfo(instance, &currentRat, &currentStatus))
        return TAF_PROP_TIMEOUT;
    *statusPtr = (rat == currentRat) ? currentStatus : TAF_PROP_RADIO_RAT_SERVICE_STATUS_NO_SERVICE;
    TAF_PROP_DEBUG("taf_prop_radio_GetRatSvcStatus: instance=%u rat=%d (serving=%d) -> status=%d",
        instance, static_cast<int>(rat), static_cast<int>(currentRat), static_cast<int>(*statusPtr));
    return TAF_PROP_OK;
}

int32_t taf_prop_radio_PerformPciNetworkScan(
    uint32_t instance,
    taf_prop_radio_RatBitMask_t bitmask,
    taf_prop_radio_PciScanInformation_t* informationPtr)
{
    if (!validInstance(instance) || informationPtr == nullptr)
        return badParam("taf_prop_radio_PerformPciNetworkScan: invalid argument");
    std::memset(informationPtr, 0, sizeof(*informationPtr));

    auto cellsOpt = getCells(instance);
    if (!cellsOpt)
    {
        TAF_PROP_DEBUG("taf_prop_radio_PerformPciNetworkScan: instance=%u -> request_cell_info timed out",
            instance);
        return TAF_PROP_TIMEOUT;
    }
    auto& cells = *cellsOpt;
    uint32_t out = 0;
    for (const auto& cell : cells)
    {
        if (out >= TAF_PROP_RADIO_PCI_SCAN_CELL_MAX_COUNT)
            break;
        taf_prop_radio_Rat_t rat = TAF_PROP_RADIO_RAT_UNKNOWN;
        const nlohmann::json* body = nullptr;
        const char* pcidKey = nullptr;
        const auto type = cell.value("cellType", std::string());
        if (type == "LTE" && cell.contains("lte"))
            { rat = TAF_PROP_RADIO_RAT_LTE; body = &cell["lte"]; pcidKey = "pci"; }
        else if (type == "NR5G" && cell.contains("nr5g"))
            { rat = TAF_PROP_RADIO_RAT_NR5G; body = &cell["nr5g"]; pcidKey = "pci"; }
        else if (type == "WCDMA" && cell.contains("wcdma"))
            { rat = TAF_PROP_RADIO_RAT_UMTS; body = &cell["wcdma"]; pcidKey = "psc"; }
        else if (type == "GSM" && cell.contains("gsm"))
            { rat = TAF_PROP_RADIO_RAT_GSM; body = &cell["gsm"]; pcidKey = "bsic"; }
        if (body == nullptr || pcidKey == nullptr ||
            (bitmask != 0 && (bitmask & ratMask(rat)) == 0))
            continue;

        auto& dst = informationPtr->pciCellInfo[out++];
        dst.cellId = static_cast<uint16_t>(body->value(pcidKey, 0));
        dst.globalCellId = static_cast<uint32_t>(body->value("ci", body->value("cid", 0)));
        dst.plmnCount = 1;
        dst.plmnId[0].mcc = static_cast<uint16_t>(parseIntString(*body, "mcc"));
        dst.plmnId[0].mnc = static_cast<uint16_t>(parseIntString(*body, "mnc"));
        dst.plmnId[0].mncIncludesPcsDigit = 0;
    }
    informationPtr->pciCellCount = out;
    TAF_PROP_DEBUG("taf_prop_radio_PerformPciNetworkScan: instance=%u bitmask=0x%x -> %u cell(s)",
        instance, static_cast<unsigned>(bitmask), out);
    return TAF_PROP_OK;
}

int32_t taf_prop_radio_GetServingCellRac(
    uint32_t instance,
    taf_prop_radio_Rat_t rat,
    uint8_t* racPtr)
{
    if (!validInstance(instance) || racPtr == nullptr)
        return badParam("taf_prop_radio_GetServingCellRac: invalid argument");
    auto cells = getCells(instance).value_or(std::vector<nlohmann::json>{});
    for (const auto& cell : cells)
    {
        if (!cell.value("registered", false))
            continue;
        const nlohmann::json* body = nullptr;
        if (rat == TAF_PROP_RADIO_RAT_GSM && cell.contains("gsm"))
            body = &cell["gsm"];
        else if (rat == TAF_PROP_RADIO_RAT_UMTS && cell.contains("wcdma"))
            body = &cell["wcdma"];
        else if (rat == TAF_PROP_RADIO_RAT_TDSCDMA && cell.contains("tdscdma"))
            body = &cell["tdscdma"];
        if (body == nullptr)
            continue;
        *racPtr = static_cast<uint8_t>(body->value("lac", 0) & 0xff);
        TAF_PROP_DEBUG("taf_prop_radio_GetServingCellRac: instance=%u rat=%d -> rac=%u",
            instance, static_cast<int>(rat), *racPtr);
        return TAF_PROP_OK;
    }
    TAF_PROP_DEBUG("taf_prop_radio_GetServingCellRac: instance=%u rat=%d -> no registered cell for that RAT",
        instance, static_cast<int>(rat));
    return TAF_PROP_FAULT;
}

int32_t taf_prop_radio_GetDataAvailSysStatus(
    uint32_t instance,
    taf_prop_radio_DataAvailSysStatus_t* statusPtr)
{
    if (!validInstance(instance) || statusPtr == nullptr)
        return badParam("taf_prop_radio_GetDataAvailSysStatus: invalid argument");
    if (!computeDataAvailSysStatus(instance, statusPtr))
        return TAF_PROP_TIMEOUT;
    TAF_PROP_DEBUG("taf_prop_radio_GetDataAvailSysStatus: instance=%u -> availSysCount=%u rat=%d soMask=0x%x",
        instance, statusPtr->availSysCount,
        statusPtr->availSysCount ? static_cast<int>(statusPtr->availSysStatusInfo[0].rat) : -1,
        statusPtr->availSysCount ? static_cast<unsigned>(statusPtr->availSysStatusInfo[0].soMask) : 0u);
    return TAF_PROP_OK;
}

int32_t taf_prop_radio_GetLteCphyCaInfo(
    uint32_t instance,
    taf_prop_radio_LteCphyCaInfo_t* infoPtr)
{
    if (!validInstance(instance) || infoPtr == nullptr)
        return badParam("taf_prop_radio_GetLteCphyCaInfo: invalid argument");
    if (!computeLteCphyCaInfo(instance, infoPtr))
    {
        TAF_PROP_DEBUG("taf_prop_radio_GetLteCphyCaInfo: instance=%u -> no registered LTE cell", instance);
        return TAF_PROP_FAULT;
    }
    TAF_PROP_DEBUG("taf_prop_radio_GetLteCphyCaInfo: instance=%u -> pci=%u freq=%u band=%u",
        instance, infoPtr->pcellInfo.pci, infoPtr->pcellInfo.freq, infoPtr->pcellInfo.band);
    return TAF_PROP_OK;
}

taf_prop_radio_RatSvcStatusHandlerRef_t taf_prop_radio_AddRatSvcStatusHandler(
    uint32_t instance,
    taf_prop_radio_RatSvcStatusHdlrFunc_t handlerFuncPtr,
    void* contextPtr)
{
    ensureRatPolling();
    std::lock_guard<std::mutex> lk(ctx().mutex);
    ctx().ratSvc = Handler{instance, reinterpret_cast<void*>(handlerFuncPtr), contextPtr};
    TAF_PROP_INFO("taf_prop_radio_AddRatSvcStatusHandler: instance=%u handler=%p",
        instance, reinterpret_cast<void*>(handlerFuncPtr));
    return nullptr;
}

taf_prop_radio_LteCphyCaHandlerRef_t taf_prop_radio_AddLteCphyCaHandler(
    uint32_t instance,
    taf_prop_radio_LteCphyCaHdlrFunc_t handlerFuncPtr,
    void* contextPtr)
{
    ensureRatPolling();
    std::lock_guard<std::mutex> lk(ctx().mutex);
    ctx().lteCphyCa = Handler{instance, reinterpret_cast<void*>(handlerFuncPtr), contextPtr};
    TAF_PROP_INFO("taf_prop_radio_AddLteCphyCaHandler: instance=%u handler=%p",
        instance, reinterpret_cast<void*>(handlerFuncPtr));
    return nullptr;
}

taf_prop_radio_DataAvailSysStatusHandlerRef_t taf_prop_radio_AddDataAvailSysStatusHandler(
    uint32_t instance,
    taf_prop_radio_DataAvailSysStatusHdlrFunc_t handlerFuncPtr,
    void* contextPtr)
{
    ensureRatPolling();
    std::lock_guard<std::mutex> lk(ctx().mutex);
    ctx().dataAvail = Handler{instance, reinterpret_cast<void*>(handlerFuncPtr), contextPtr};
    TAF_PROP_INFO("taf_prop_radio_AddDataAvailSysStatusHandler: instance=%u handler=%p",
        instance, reinterpret_cast<void*>(handlerFuncPtr));
    return nullptr;
}

int32_t taf_prop_radio_GetDataCurrRoamingStatus(
    uint32_t instance,
    taf_prop_radio_DataRoamingStatus_t* statusPtr)
{
    if (!validInstance(instance) || statusPtr == nullptr)
        return badParam("taf_prop_radio_GetDataCurrRoamingStatus: invalid argument");
    auto data = requestData(topics::data::request_roaming_status::req,
                            "data.request_roaming_status.rsp", instance);
    if (!data)
        return TAF_PROP_TIMEOUT;
    *statusPtr = data->value("isRoaming", false)
        ? TAF_PROP_RADIO_DATA_ROAMING_STATUS_ON
        : TAF_PROP_RADIO_DATA_ROAMING_STATUS_OFF;
    TAF_PROP_DEBUG("taf_prop_radio_GetDataCurrRoamingStatus: instance=%u -> status=%d",
        instance, static_cast<int>(*statusPtr));
    return TAF_PROP_OK;
}

} // extern "C"
