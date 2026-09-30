// =============================================================================
//  GameClock.cpp - a skeleton. Every function is here with the right signature
//  and an empty body. GameClock.h is the specification; read it first.
// =============================================================================

#include <engine/core/GameClock.h>
#include <engine/core/Log.h>

#include <algorithm>

namespace eng {

// Starts the clock from zero and empties the accumulator.
void GameClock::Init() {
    m_accumulator = 0.0;
    m_realSeconds = 0.0;
    m_gameSeconds = 0.0;
    m_ticks = 0;
}

// Sets how much game time one simulation step represents. Every step is this
// long on every machine, which is what makes the game behave the same on all
// of them.
void GameClock::SetFixedStepSeconds(float seconds) {
    m_fixedStep = std::clamp(seconds, 1.0f / 1000.0f, 1.0f);
}

// Speeds game time up or slows it down. It does not change the size of a step -
// only how many of them a second of real time is worth.
void GameClock::SetTimeScale(float scale) {
    m_timeScale = std::clamp(scale, 0.0f, 16.0f);
}

// Caps how many steps one frame may run. Without a ceiling, a frame that runs
// slow asks for more steps, which makes it slower still, and the program locks
// up and never recovers.
void GameClock::SetMaxStepsPerFrame(int steps) {
    m_maxSteps = std::clamp(steps, 1, 60);
}

// Adds this frame's real time to the accumulator and returns how many whole
// fixed steps the simulation now owes. Usually 0, 1 or 2.
int GameClock::BeginFrame(double realDeltaSeconds) {
    // Real time moves forward no matter what: not affected by pause, by the
    // time scale, or by the limit below. That is what makes it the timeline
    // you can trust when measuring how long something took.
    m_realDelta = static_cast<float>(realDeltaSeconds);
    m_realSeconds += realDeltaSeconds;

    if (m_paused) {
        // Single step: exactly one tick, and the accumulator is left untouched
        // so that pressing Play afterwards does not suddenly owe a burst of
        // steps. Adding to the accumulator instead would make each press
        // advance a slightly different amount.
        if (m_singleStepRequested) {
            m_singleStepRequested = false;
            return 1;
        }
        return 0;
    }

    // Collect this frame's real time, adjusted by the time scale.
    m_accumulator += realDeltaSeconds * static_cast<double>(m_timeScale);

    // Take out as many whole steps as fit.
    int steps = 0;
    while (m_accumulator >= static_cast<double>(m_fixedStep)) {
        m_accumulator -= static_cast<double>(m_fixedStep);
        ++steps;
        if (steps >= m_maxSteps) {
            // Cap the number of steps so a slow frame does not make the next
            // frame even slower.
            break;
        }
    }

    // THE LIMIT. If there is still more than a step's worth left after
    // stopping, the surplus is thrown away and the simulation falls behind
    // real time. Reported every time, because a game quietly running in slow
    // motion is its own confusing bug.

    if (m_accumulator >= static_cast<double>(m_fixedStep)) {
        ENGINE_LOG_WARN(Channels::kCore, 
                        "this frame hit the limit of {} simulation steps; {:.1f} ms of "
                        "time was discarded and the simulation is now behind real time",
                        m_maxSteps, m_accumulator * 1000.0);
        m_accumulator = 0.0;
    }

    return steps;
}
// Records that one owed step has been run, taking its time back out of the
// accumulator.
void GameClock::OnStepConsumed() {
    m_gameSeconds += static_cast<double>(m_fixedStep);
    ++m_ticks;
}

} // namespace eng
