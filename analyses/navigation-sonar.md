# Rift Navigation: Sonar Ping

## Problem
A constant world-space marker is a strictly-dominant, passive, perfect signal. It captures
attention: player stares at the marker, stops reading the world, flying goes dull. It also
trivializes the task (point at diamond, hold) instead of routing through terrain.

## Fix
Replace the far-range marker with an **active, intermittent, imperfect** ping. Navigation
punctuates the flying instead of competing with it. Fits the dense fog: fog is the *reason*
the mechanic exists, not a limitation.

## Mechanic
- Press to **ping** (P). Requires a **full stop** to fire (no cooldown needed).
- **Outgoing:** a sphere expands from the ship in all directions (feedback only).
- **Return:** a directional **wavefront** (curved wall of light) sweeps at you from the
  station's bearing and passes through you. That pass-through is the read.
  - **Direction** it hits from = station bearing.
  - **Time of flight** (press -> arrival) = distance. Close = fast, far = slow. Distance
    read by feel, no number.
- **Precision falloff (optional polish):** far = wide/faint/smeared front (rough heading);
  near = narrow/bright/focused front. Very near = precise marker takes over.
- Info is transient: the wall passes and is gone; re-ping to refresh.

## Why full-stop-to-ping is the core cost
Momentum is everything and time is scored, so stopping is a real sacrifice. Every ping is a
decision: stop and ping, or trust your mental map and keep flying. Scales with skill the
right way: a good player pings less, navigating long stretches on memory + world-reading.
That is the pre-marker fly-sim feeling earned back through mastery.

## Slice scope
IN: outgoing sphere, incoming directional wall, time-of-flight = distance, full-stop to
fire, precise marker at close range only.
OUT (later): spatial audio return, terrain-bounce hazard-sense, precision falloff if fiddly.

## Test hypothesis
With the sonar in, the player spends most of a run looking at the world and only glances at
navigation right after a ping. Flying feels alive again the way it did before the marker.

## Watch during test
- Stopping in dense fog near terrain must be executable, not a punishment. Fallback: allow
  ping at near-zero speed instead of literal dead stop.
- Elevation: lean on existing altitude-band-per-station so the ping mostly answers "which
  compass direction," which is far more readable than full 3D.
- Do not add a cooldown on top of the full-stop cost (double tax).
