// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
//
// Envelope.hpp - MQTT message envelope shared by every simula-pa domain.
//
// Envelope is the transport/routing wrapper: who sent it, when, how to
// correlate a response to its request, and (for shared rsp topics) who the
// response is actually for. Business content lives in `data`/`error`, whose
// shape is defined per-method by the sml/simula_shared payload schemas —
// Envelope itself is domain-agnostic and hand-written (not code-gen'd).

#ifndef TELUX_COMMON_SIMULA_ENVELOPE_HPP
#define TELUX_COMMON_SIMULA_ENVELOPE_HPP

#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <telux/common/CommonDefines.hpp>

namespace telux::common::simula {

// Envelope models the common MQTT body wrapper.
//
// On wire:
//   req: {"v":1,"corrId":"0000000a","ts":<ms>,"src":"<paId>","data":{...}}
//   rsp: {"v":1,"corrId":"0000000a","ts":<ms>,"src":"<mpssId>",
//         "dest":"<paId>","data":{...}}                    // success
//   rsp: {"v":1,"corrId":"0000000a","ts":<ms>,"src":"<mpssId>",
//         "dest":"<paId>","error":{"code":"...","msg":"..."}}  // failure
//   ind: {"v":1,"corrId":"0000000b","ts":<ms>,"src":"<mpssId>","data":{...}}
//
// `dest` is only present on rsp; it echoes the requester's `src` so a PA
// subscribed to a shared rsp topic can discard responses meant for other
// PA processes before even consulting its corrId in-flight table.
//
// Constraint (not enforced at parse time): rsp messages carry exactly one
// of {data, error}; req and ind messages always carry data.
struct Envelope
{
    int v = 1;
    std::string corrId;
    int64_t ts = 0;
    std::string src;
    std::optional<std::string> dest;
    std::optional<nlohmann::json> data;
    std::optional<nlohmann::json> error;

    nlohmann::json toJson() const;
    static std::optional<Envelope> fromJson(const nlohmann::json& j, std::string* err = nullptr);
};

// Envelope builders. Caller supplies `src` (the paId, from
// ModemBridge::currentPaId()), `data` (the per-method payload), and — for
// response/error envelopes — the request's `corrId` and `dest` (the
// requester's `src`, for shared-topic misdelivery filtering).

Envelope
makeRequestEnvelope(std::string_view src, nlohmann::json data);

Envelope
makeResponseEnvelope(
  std::string_view src,
  std::string_view corrId,
  std::string_view dest,
  nlohmann::json data
);

Envelope
makeErrorEnvelope(
  std::string_view src,
  std::string_view corrId,
  std::string_view dest,
  telux::common::ErrorCode code,
  std::string_view msg = {}
);

Envelope
makeEventEnvelope(std::string_view src, nlohmann::json data);

// Map a wire-format ErrorCode string into the corresponding telux enum
// value. Unknown wire strings map to GENERIC_FAILURE for forward
// compatibility.
telux::common::ErrorCode
parseErrorCode(std::string_view wire);

// Inverse: map a telux ErrorCode enum back to its wire string. Returns
// "GENERIC_FAILURE" for any code with no wire mapping.
std::string_view
errorCodeName(telux::common::ErrorCode code);

// Allocate a fresh correlation id: fixed 8 lowercase hex chars, monotonic
// in-process counter. Only needs to be locally distinguishable — cross
// process disambiguation on shared rsp topics is `dest`'s job, not
// corrId's, so this deliberately does not chase global uniqueness (no
// UUID).
std::string
nextCorrId();

}  // namespace telux::common::simula

#endif  // TELUX_COMMON_SIMULA_ENVELOPE_HPP
