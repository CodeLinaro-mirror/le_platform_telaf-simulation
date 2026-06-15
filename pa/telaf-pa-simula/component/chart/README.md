# chart — tiny HSM + Active Object framework for C++17

`chart` is a header-only C++17 library that gives you the classic hierarchical
state machine pattern (one function per state, switch on signal, default branch
delegates to super) plus an `ActiveObject` runtime, time events, pub/sub, and
event deferral — in roughly **800 lines of headers**, no external dependencies
beyond `<thread>` and `<mutex>`.

It exists because:
- Larger HSM frameworks tend to bring their own build system, port layer, and
  modeling tools — useful in some contexts, overkill when all you want is a
  small in-process state machine.
- Register-state-and-transitions style libraries scatter a state's logic across
  many small callbacks, which loses readability for hand-written charts.

If you write your charts by hand and want them to read as one function per
state with a switch on signal, this is for you.

## Hello, world

```cpp
#include <chart/chart.hpp>
#include <iostream>

using chart::Event;
using chart::Hsm;
using chart::Status;

constexpr chart::Signal Toggle_Signal{chart::User_Signal_Begin, "Toggle"};

class Light : public chart::ActiveObject { public: using ActiveObject::ActiveObject; };

static Status off_st(Hsm*, Event const*);
static Status on_st (Hsm*, Event const*);

static Status off_st(Hsm* h, Event const* e) {
    auto* self = static_cast<Light*>(h);
    switch (e->sig) {
        case chart::Entry_Signal: std::cout << "OFF\n"; return Status::HANDLED;
        case Toggle_Signal:       return self->to(on_st);
        default: return self->super(&Hsm::top);
    }
}
static Status on_st(Hsm* h, Event const* e) {
    auto* self = static_cast<Light*>(h);
    switch (e->sig) {
        case chart::Entry_Signal: std::cout << "ON\n"; return Status::HANDLED;
        case Toggle_Signal:       return self->to(off_st);
        default: return self->super(&Hsm::top);
    }
}

int main() {
    Light l("light");
    l.start_at(off_st);
    l.post_fifo({Toggle_Signal, nullptr});
    l.post_fifo({Toggle_Signal, nullptr});
    l.stop();
}
```

Build:

```sh
g++ -std=c++17 -I include hello.cpp -lpthread -o hello
```

Or via CMake (as a sub-project of yours):

```cmake
add_subdirectory(third_party/chart-cpp)
target_link_libraries(myapp PRIVATE chart::chart)
```

## API tour

| Header | What it gives you |
|---|---|
| `<chart/event.hpp>` | `Signal{id, name}`, `Event{sig, payload}`, system signals (`Entry_Signal`, `Exit_Signal`, `Init_Signal`) |
| `<chart/hsm.hpp>` | `Hsm` base class with `init` / `dispatch` / `to` / `super` / `top`. Implements Samek's LCA algorithm |
| `<chart/active_object.hpp>` | `ActiveObject` — adds a worker thread + FIFO/LIFO queue + `start_at`/`post_fifo`/`post_lifo`/`stop` |
| `<chart/time_event.hpp>` | `TimeEvent` — one-shot or periodic self-posting timers, sharing one background timer thread |
| `<chart/publish_subscribe.hpp>` | `Fabric::instance().subscribe / publish / unsubscribe` for multi-AO topology |
| `<chart/defer.hpp>` | `DeferQueue` + `defer()` / `recall()` / `recall_all()` for events you can't yet handle |
| `<chart/spy.hpp>` | `Spy` and `StateNames` for opt-in trace output |
| `<chart/instrument.hpp>` | `Instrument` interface for pluggable AO instrumentation |
| `<chart/assert.hpp>` | `CHART_ASSERT` / `_REQUIRE` / `_ENSURE` |
| `<chart/chart.hpp>` | umbrella include |

State handlers have the canonical signature:

```cpp
chart::Status state_fn(chart::Hsm* h, chart::Event const* e);
```

Inside the switch:

| Case | Idiom |
|---|---|
| `Entry_Signal` | run entry action; `return Status::HANDLED;` |
| `Exit_Signal`  | run exit action; `return Status::HANDLED;` |
| `Init_Signal`  | initial transition for composite state: `return self->to(child);` |
| user signals  | guarded transition: `return self->to(target);` or `return Status::HANDLED;` for internal |
| guard fail    | `return Status::UNHANDLED;` — dispatcher retries on super-state |
| `default`     | `return self->super(parent);` (or `self->super(&Hsm::top)` at the top) |

Signal naming convention: built-in signals are `None_Signal`, `Entry_Signal`,
`Exit_Signal`, `Init_Signal`. User signals start at `User_Signal_Begin` and
follow the same `PascalCase_Signal` convention (e.g. `Toggle_Signal`,
`Hungry_Signal`).

## What's intentionally not implemented

`chart` aims to stay small. We don't ship:

- shallow / deep history pseudo-states
- entry-point / exit-point pseudo-states
- orthogonal regions
- preemption / priorities — each AO has its own thread, OS schedules
- static event pools / zero-copy events — `std::shared_ptr` is good enough for desktop / Linux
- a graphical modeling tool / code generator

If you need those, pick a more complete state-machine framework.

## Notes for projects adopting `chart`

- Each state handler is a free function (or static member) returning
  `chart::Status` and taking `Hsm*` + `Event const*`. Cast `Hsm*` back to your
  derived AO type with `static_cast<MyAo*>(h)` — `chart::ActiveObject` is a
  sealed concrete base, not a virtual interface.
- `Event::payload` is `std::shared_ptr<void>` so any type can ride along; cast
  with `std::static_pointer_cast<MyPayload>(e->payload)` on the receiver side.
- `TimeEvent` stores a raw `ActiveObject*`; keep the target alive at least
  until the `TimeEvent` is disarmed (the destructor disarms automatically).
- Pub/sub fan-out is FIFO via `post_fifo`; ordering across subscribers is not
  guaranteed (each AO consumes events on its own thread).
- All singletons (`TimerService`, `Fabric`, `Signals`, `Spy`, `StateNames`) use
  function-local statics, so they survive across translation units in
  header-only builds.

## Building tests

```sh
cmake -S . -B build && cmake --build build -j
ctest --test-dir build --output-on-failure
```

Sanitizer pass (recommended before changes):

```sh
cmake -S . -B build-asan -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g"
cmake --build build-asan -j
ctest --test-dir build-asan --output-on-failure
```

Known sanitizer note: TSan flags the `TimerService` mutex pattern as a
"double lock" / data race when the timer worker and a user thread alternate
on `m_` around `cv.wait_until`. We've audited the code path and both accesses
are properly serialized under the same mutex; this is a known TSan limitation
on this style of singleton + cv pattern. ASan+UBSan reports the test suite
clean, and all four tests pass functionally on every build.
