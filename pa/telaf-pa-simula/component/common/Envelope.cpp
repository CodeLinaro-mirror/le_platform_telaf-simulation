// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "Envelope.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>

namespace telux::common::simula {

using nlohmann::json;
using telux::common::ErrorCode;

namespace {

struct ErrorCodeEntry
{
    std::string_view wire;
    ErrorCode code;
};

// Wire-format string <-> telux::common::ErrorCode mapping. Generic set
// shared by every domain; a domain that needs an ErrorCode not listed here
// should extend this table rather than keep a parallel one.
constexpr std::array<ErrorCodeEntry, 10> kErrorCodes = { {
  { "SUCCESS", ErrorCode::SUCCESS },
  { "GENERIC_FAILURE", ErrorCode::GENERIC_FAILURE },
  { "RADIO_NOT_AVAILABLE", ErrorCode::RADIO_NOT_AVAILABLE },
  { "INVALID_ARGUMENTS", ErrorCode::INVALID_ARGUMENTS },
  { "OP_IN_PROGRESS", ErrorCode::OP_IN_PROGRESS },
  { "NOT_SUPPORTED", ErrorCode::NOT_SUPPORTED },
  { "DEVICE_IN_USE", ErrorCode::DEVICE_IN_USE },
  { "INVALID_OPERATION", ErrorCode::INVALID_OPERATION },
  { "NO_RESOURCES", ErrorCode::NO_RESOURCES },
  { "OPERATION_TIMEOUT", ErrorCode::OPERATION_TIMEOUT },
} };

int64_t
nowMs()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

std::atomic<uint32_t> g_corr_counter{ 0 };

}  // namespace

// ---------------------------------------------------------------------------
// Envelope

json
Envelope::toJson() const
{
    json j = json::object();
    j["v"] = v;
    j["corrId"] = corrId;
    j["ts"] = ts;
    j["src"] = src;
    if (dest)
        j["dest"] = *dest;
    if (data)
        j["data"] = *data;
    if (error)
        j["error"] = *error;
    return j;
}

std::optional<Envelope>
Envelope::fromJson(const json& j, std::string* err)
{
    auto fail = [err](std::string_view msg) -> std::optional<Envelope> {
        if (err)
            *err = std::string(msg);
        return std::nullopt;
    };
    if (!j.is_object())
        return fail("envelope is not a JSON object");
    if (!j.contains("v") || !j["v"].is_number_integer())
        return fail("missing/invalid field `v`");
    if (j["v"].get<int>() != 1)
        return fail("envelope `v` != 1");
    if (!j.contains("corrId") || !j["corrId"].is_string())
        return fail("missing/invalid field `corrId`");
    if (!j.contains("ts") || !j["ts"].is_number_integer())
        return fail("missing/invalid field `ts`");
    if (!j.contains("src") || !j["src"].is_string())
        return fail("missing/invalid field `src`");

    Envelope env;
    env.v = j["v"].get<int>();
    env.corrId = j["corrId"].get<std::string>();
    env.ts = j["ts"].get<int64_t>();
    env.src = j["src"].get<std::string>();
    if (j.contains("dest"))
    {
        if (!j["dest"].is_string())
            return fail("invalid field `dest`");
        env.dest = j["dest"].get<std::string>();
    }
    if (j.contains("data"))
    {
        if (!j["data"].is_object())
            return fail("invalid field `data`");
        env.data = j["data"];
    }
    if (j.contains("error"))
    {
        if (!j["error"].is_object())
            return fail("invalid field `error`");
        env.error = j["error"];
    }
    return env;
}

// ---------------------------------------------------------------------------
// Builders

Envelope
makeRequestEnvelope(std::string_view src, json data)
{
    Envelope e;
    e.v = 1;
    e.corrId = nextCorrId();
    e.ts = nowMs();
    e.src = std::string(src);
    e.data = std::move(data);
    return e;
}

Envelope
makeResponseEnvelope(
  std::string_view src,
  std::string_view corrId,
  std::string_view dest,
  json data
)
{
    Envelope e;
    e.v = 1;
    e.corrId = std::string(corrId);
    e.ts = nowMs();
    e.src = std::string(src);
    e.dest = std::string(dest);
    e.data = std::move(data);
    return e;
}

Envelope
makeErrorEnvelope(
  std::string_view src,
  std::string_view corrId,
  std::string_view dest,
  ErrorCode code,
  std::string_view msg
)
{
    Envelope e;
    e.v = 1;
    e.corrId = std::string(corrId);
    e.ts = nowMs();
    e.src = std::string(src);
    e.dest = std::string(dest);
    json err_obj = json::object();
    err_obj["code"] = std::string(errorCodeName(code));
    if (!msg.empty())
        err_obj["msg"] = std::string(msg);
    e.error = std::move(err_obj);
    return e;
}

Envelope
makeEventEnvelope(std::string_view src, json data)
{
    Envelope e;
    e.v = 1;
    e.corrId = nextCorrId();
    e.ts = nowMs();
    e.src = std::string(src);
    e.data = std::move(data);
    return e;
}

// ---------------------------------------------------------------------------
// ErrorCode mapping

ErrorCode
parseErrorCode(std::string_view wire)
{
    for (const auto& entry : kErrorCodes)
    {
        if (entry.wire == wire)
            return entry.code;
    }
    return ErrorCode::GENERIC_FAILURE;
}

std::string_view
errorCodeName(ErrorCode code)
{
    for (const auto& entry : kErrorCodes)
    {
        if (entry.code == code)
            return entry.wire;
    }
    return "GENERIC_FAILURE";
}

// ---------------------------------------------------------------------------
// corrId allocator — fixed 8 lowercase hex digits, in-process counter.

std::string
nextCorrId()
{
    uint32_t n = g_corr_counter.fetch_add(1, std::memory_order_relaxed);
    char buf[9];
    std::snprintf(buf, sizeof(buf), "%08x", n);
    return std::string(buf, 8);
}

}  // namespace telux::common::simula
