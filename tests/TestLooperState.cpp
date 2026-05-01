#include <catch2/catch_test_macros.hpp>
#include "Model/LooperState.h"
#include "Model/LooperTracker.h"
#include "Shared/ProtocolDefs.h"

using namespace looper;

// --- LooperState comparison tests ---

TEST_CASE("LooperState comparison: equal states", "[LooperState]")
{
    LooperState a;
    a.trackId = "track1";
    a.trackName = "Guitar";
    a.deviceId = "0";
    a.deviceName = "Looper";
    a.className = "Looper";
    a.state = LooperState::Stopped;
    a.feedback = 0.5f;
    a.reverse = false;
    a.loopLengthBars = 4;

    LooperState b;
    b.trackId = "track1";
    b.trackName = "Guitar";
    b.deviceId = "0";
    b.deviceName = "Looper";
    b.className = "Looper";
    b.state = LooperState::Stopped;
    b.feedback = 0.5f;
    b.reverse = false;
    b.loopLengthBars = 4;

    REQUIRE(a == b);
    REQUIRE_FALSE(a != b);
}

TEST_CASE("LooperState comparison: different states", "[LooperState]")
{
    LooperState a;
    a.trackId = "track1";
    a.state = LooperState::Recording;
    a.feedback = 0.5f;

    LooperState b;
    b.trackId = "track1";
    b.state = LooperState::Playing;
    b.feedback = 0.5f;

    REQUIRE(a != b);
    REQUIRE_FALSE(a == b);

    // Different feedback
    LooperState c;
    c.trackId = "track1";
    c.state = LooperState::Recording;
    c.feedback = 0.7f;

    REQUIRE(a != c);

    // Different trackId
    LooperState d;
    d.trackId = "track2";
    d.state = LooperState::Recording;
    d.feedback = 0.5f;

    REQUIRE(a != d);
}

TEST_CASE("LooperState stateToString conversion", "[LooperState]")
{
    REQUIRE(LooperState::stateToString(LooperState::Stopped) == protocol::STATE_STOPPED);
    REQUIRE(LooperState::stateToString(LooperState::Recording) == protocol::STATE_RECORDING);
    REQUIRE(LooperState::stateToString(LooperState::Playing) == protocol::STATE_PLAYING);
    REQUIRE(LooperState::stateToString(LooperState::Overdubbing) == protocol::STATE_OVERDUBBING);
}

TEST_CASE("LooperState stringToState conversion", "[LooperState]")
{
    REQUIRE(LooperState::stringToState(protocol::STATE_STOPPED) == LooperState::Stopped);
    REQUIRE(LooperState::stringToState(protocol::STATE_RECORDING) == LooperState::Recording);
    REQUIRE(LooperState::stringToState(protocol::STATE_PLAYING) == LooperState::Playing);
    REQUIRE(LooperState::stringToState(protocol::STATE_OVERDUBBING) == LooperState::Overdubbing);
}

// --- LooperTracker tests ---

TEST_CASE("LooperTracker add and retrieve looper", "[LooperTracker]")
{
    LooperTracker tracker;

    LooperState state;
    state.trackId = "track1";
    state.trackName = "Guitar";
    state.deviceId = "0";
    state.deviceName = "Looper";
    state.className = "Looper";
    state.state = LooperState::Stopped;
    state.feedback = 0.5f;

    tracker.addLooper(state);

    REQUIRE(tracker.getLooperCount() == 1);

    auto retrieved = tracker.getLooper("track1");
    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->trackId == "track1");
    REQUIRE(retrieved->trackName == "Guitar");
    REQUIRE(retrieved->state == LooperState::Stopped);
}

TEST_CASE("LooperTracker update state fires change", "[LooperTracker]")
{
    LooperTracker tracker;
    int changeCount = 0;
    tracker.onStateChange([&changeCount]() { ++changeCount; });

    LooperState state;
    state.trackId = "track1";
    state.trackName = "Guitar";
    state.deviceId = "0";
    state.state = LooperState::Stopped;
    state.feedback = 0.5f;

    // addLooper should fire change
    tracker.addLooper(state);
    REQUIRE(changeCount == 1);

    // updateState with different state should fire change
    LooperState updated = state;
    updated.state = LooperState::Recording;
    tracker.updateState("track1", updated);
    REQUIRE(changeCount == 2);

    auto retrieved = tracker.getLooper("track1");
    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->state == LooperState::Recording);
}

TEST_CASE("LooperTracker update state no change", "[LooperTracker]")
{
    LooperTracker tracker;
    int changeCount = 0;
    tracker.onStateChange([&changeCount]() { ++changeCount; });

    LooperState state;
    state.trackId = "track1";
    state.trackName = "Guitar";
    state.deviceId = "0";
    state.state = LooperState::Stopped;
    state.feedback = 0.5f;

    // addLooper fires change
    tracker.addLooper(state);
    REQUIRE(changeCount == 1);

    // updateState with same state should NOT fire change (diff-based)
    tracker.updateState("track1", state);
    REQUIRE(changeCount == 1);
}

TEST_CASE("LooperTracker remove looper", "[LooperTracker]")
{
    LooperTracker tracker;

    LooperState state;
    state.trackId = "track1";
    state.trackName = "Guitar";
    state.deviceId = "0";
    state.state = LooperState::Stopped;

    tracker.addLooper(state);
    REQUIRE(tracker.getLooperCount() == 1);

    tracker.removeLooper("track1");
    REQUIRE(tracker.getLooperCount() == 0);

    auto retrieved = tracker.getLooper("track1");
    REQUIRE_FALSE(retrieved.has_value());
}

// --- Cycle Count Tests ---

TEST_CASE("LooperState comparison: same cycleCount", "[LooperState]")
{
    LooperState a;
    a.trackId = "track1";
    a.state = LooperState::Playing;
    a.cycleCount = 3;

    LooperState b;
    b.trackId = "track1";
    b.state = LooperState::Playing;
    b.cycleCount = 3;

    REQUIRE(a == b);
}

TEST_CASE("LooperState comparison: different cycleCount", "[LooperState]")
{
    LooperState a;
    a.trackId = "track1";
    a.state = LooperState::Playing;
    a.cycleCount = 3;

    LooperState b;
    b.trackId = "track1";
    b.state = LooperState::Playing;
    b.cycleCount = 5;

    REQUIRE(a != b);
}

TEST_CASE("LooperTracker::incrementCycleCount increments and notifies", "[LooperTracker]")
{
    LooperTracker tracker;
    int changeCount = 0;
    tracker.onStateChange([&changeCount]() { ++changeCount; });

    LooperState state;
    state.trackId = "track1";
    state.trackName = "Guitar";
    state.deviceId = "0";
    state.state = LooperState::Playing;
    state.cycleCount = 0;

    tracker.addLooper(state);
    REQUIRE(changeCount == 1);

    tracker.incrementCycleCount("track1");
    REQUIRE(changeCount == 2);

    auto retrieved = tracker.getLooper("track1");
    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->cycleCount == 1);

    tracker.incrementCycleCount("track1");
    REQUIRE(changeCount == 3);

    auto updated = tracker.getLooper("track1");
    REQUIRE(updated->cycleCount == 2);
}

TEST_CASE("LooperTracker::incrementCycleCount non-existent track is no-op", "[LooperTracker]")
{
    LooperTracker tracker;
    int changeCount = 0;
    tracker.onStateChange([&changeCount]() { ++changeCount; });

    // Should not crash
    tracker.incrementCycleCount("nonexistent");
    REQUIRE(changeCount == 0);
}