// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

// chart/spy_bootstrap.hpp — env/conf-driven Spy mode bootstrap.
//
// Header-only helper that resolves a SpyMode from an environment variable
// (preferred) with an optional KEY=VALUE conf-file fallback, then calls
// Spy::set_mode(). Both the env var name and the conf path/key are supplied by
// the caller, keeping the chart library generic — it knows nothing about any
// particular project's config layout.
//
// Recognized values (case-insensitive): "off", "on", "verbose". Anything else
// (including unset/empty) resolves to Off.
//
//   chart::init_from_env("TELAF_CHART_SPY", "/etc/sml_pa.conf", "LOG_SPY");
//
// Idempotent: guarded by std::call_once so repeated calls (e.g. from multiple
// TUs during static init) apply the mode exactly once.

#ifndef CHART_SPY_BOOTSTRAP_HPP
#define CHART_SPY_BOOTSTRAP_HPP

#include "spy.hpp"

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <mutex>
#include <string>

namespace chart {

namespace detail {

// Case-insensitive equality against an ASCII literal.
inline bool iequals(const std::string& s, const char* lit) {
    if (s.size() != std::strlen(lit)) return false;
    for (std::size_t i = 0; i < s.size(); ++i) {
        char a = s[i];
        if (a >= 'A' && a <= 'Z') a = static_cast<char>(a + ('a' - 'A'));
        if (a != lit[i]) return false;
    }
    return true;
}

inline SpyMode parse_mode(const std::string& s) {
    if (iequals(s, "on"))      return SpyMode::On;
    if (iequals(s, "verbose")) return SpyMode::Verbose;
    return SpyMode::Off;
}

// Minimal KEY=VALUE reader: strips '#' comments and surrounding whitespace,
// returns the value for `key` or empty string if absent/unreadable.
inline std::string conf_value(const char* path, const char* key) {
    if (!path || !key) return {};
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        auto hash = line.find('#');
        if (hash != std::string::npos) line.erase(hash);
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq);
        std::string v = line.substr(eq + 1);
        auto trim = [](std::string& x) {
            const char* ws = " \t\r\n";
            auto b = x.find_first_not_of(ws);
            auto e = x.find_last_not_of(ws);
            x = (b == std::string::npos) ? "" : x.substr(b, e - b + 1);
        };
        trim(k);
        trim(v);
        if (k == key) return v;
    }
    return {};
}

}  // namespace detail

// Resolve mode from env_var (case-insensitive), then conf_path/conf_key
// fallback (both may be null to skip the conf step). Calls Spy::set_mode()
// exactly once across the process lifetime.
inline void init_from_env(const char* env_var,
                          const char* conf_path = nullptr,
                          const char* conf_key  = nullptr) {
    static std::once_flag once;
    std::call_once(once, [&] {
        std::string setting;
        if (env_var) {
            const char* v = std::getenv(env_var);
            if (v && v[0] != '\0') setting = v;
        }
        if (setting.empty())
            setting = detail::conf_value(conf_path, conf_key);
        Spy::set_mode(detail::parse_mode(setting));
    });
}

}  // namespace chart

#endif  // CHART_SPY_BOOTSTRAP_HPP
