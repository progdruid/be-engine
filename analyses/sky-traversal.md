# Rift: Sky Traversal

## Problem
The open sky is an empty, frictionless route. Climb above terrain, point at the target, hold
for a minute of blue screen. Zero cost, zero risk, zero disorientation (ground still visible).
It is the boring-optimal route, so the player has to *try* to fly the fun way. Sonar helps but
does not eliminate it: you can ping, pick a heading, and cruise low over everything.

## Principle
The enemy is not altitude, it is the **empty frictionless route**. Flying high is fine; flying
through *nothing* is not. Goal: **the optimal route must also be the interesting route**, so the
fun way wins on its own and roleplaying is unnecessary. Aim for *multiple* interesting routes,
not one mandatory low one.

## Two categories of fix
Kill an empty route either by making the empty medium cost something, or by filling it.

**Systemic** (cheap, global, no content authoring):
1. **Ground effect (primary).** Speed scales with terrain proximity: skim low = fast, open sky
   = draggy/slow. Since time is scored, low terrain-hugging flight becomes the *fast* line. A
   positive incentive that reuses existing scoring, needs no new UI, reads as real aerodynamics,
   and makes the flying we already love the optimal play.
2. **Turbulence at altitude.** High = crosswinds/turbulence shove the ship; the straight line
   is not easy. Low = terrain shelters you (windbreak). Keeps skill demand alive in open sky.

**Content** (expensive, per-area, needs new area design):
3. **Sky as a second terrain layer.** Floating structures, arches, debris fields, aerial
   canyons, fog banks to thread. Same "thread the gap, do not collide" challenge, elevated.
   Makes the sky positively interesting, turning altitude into a real route *choice*
   (fast-and-low vs. rich-but-slower-high). Scaffold on existing altitude-band-per-station:
   ground terrain low, rich flyable band mid, thin/empty/dangerous up top.

## Slice scope
IN: ground effect. Add turbulence if cheap in the current control model.
OUT (later): sky level design (needs new area authoring, exactly what a slice avoids
committing to before the loop is validated).

## Test hypothesis
With ground effect in, low terrain-hugging flight is the fastest route, so the player chooses
it for the score, not out of self-restraint. The blue-screen highway is no longer optimal.
