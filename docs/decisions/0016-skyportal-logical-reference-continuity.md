# SkyPortal logical-disconnect reference continuity

Status: accepted

## Context

The initial WiFi policy closed TCP and invalidated the measured home reference
on every disconnect. ConformU's connection tests consequently discard a pre-run
home before coordinate and motion tests. Cold encoder labels, matching firmware
or a new TCP connection cannot independently establish that reference.

The operator authorized a WiFi-specific held-session policy on 2026-10-21.
The USB policy in decision 0011 remains unchanged. AUX provides no reset counter;
this decision does not establish reset-safe calibration or exclusive AUX-bus
ownership. Intermittent home-time reply loss/corruption remains unresolved.

## Decision

Keep the existing TCP connection and endpoint reservation across a successful
logical disconnect only when an idle, healthy, calibrated session completes
verified stopping and a bounded read-only stationary checkpoint. In-flight or
failed operations do not qualify. Tracking owned by this driver is stopped
before collecting the checkpoint.

On logical reconnect, re-identify both motors and compare firmware with the
saved identity. Retain the reference only on the original healthy transport
generation and matching stationary encoders. Each checkpoint has a one-second
budget and requires three consecutive stable comparisons within two counts,
also within two counts of the saved baseline. No checkpoint sends a home or
label write. A failed state/identity check discards calibration; Connect may
still succeed uncalibrated after its normal verified takeover. A communication
failure closes the connection and refuses that Connect.

## Rules

1. Retain calibration only after a healthy idle logical disconnect and on the same held healthy TCP generation, matching motor identity/firmware and bounded read-only stationary checkpoints. Scope: `AlpacaCore/src/vendors/celestron/celestron_nexstar_aux_telescope_driver.cpp`. Checked by: `AlpacaCore/tests/test_celestron_nexstar_aux_telescope.cpp`, review.
2. Faults, ambiguous operations, failed continuity or replacement/closed transports invalidate the saved reference; restored communications do not restore calibration. Scope: `AlpacaCore/src/vendors/celestron/celestron_nexstar_aux_telescope_driver.cpp`. Checked by: `AlpacaCore/tests/test_celestron_nexstar_aux_telescope.cpp`, review.
3. Connect never homes or relabels; an uncalibrated connection requires an explicit successful FindHome. Scope: `AlpacaCore/src/vendors/celestron/celestron_nexstar_aux_telescope_driver.cpp`. Checked by: `AlpacaCore/tests/test_celestron_nexstar_aux_telescope.cpp`, review.
4. Logical disconnect clears public state before teardown and joins the I/O worker before checkpoint I/O; retained endpoint ownership ends on failed teardown or destruction. Scope: `AlpacaCore/src/vendors/celestron/celestron_nexstar_aux_telescope_driver.cpp`. Checked by: `AlpacaCore/tests/test_celestron_nexstar_aux_concurrency_stress.cpp`, review.

## Alternatives rejected

- Always discard calibration: preserves the initial policy but blocks the
  pre-homed logical reconnect workflow, including full ConformU qualification.
- Reuse calibration after closing/reopening TCP: creates an ownership gap with
  no equivalent same-session evidence.
- Implicit FindHome or adopting cold labels during Connect: unrequested motion
  or guessed calibration.
- Treat matching labels/firmware as proof of no reset: unsupported by AUX.

## Consequences

Disconnected devices continue reserving the endpoint only when continuity was
successfully saved. Destroying/removing the driver releases it. No reference
survives process restart, transport replacement, faults or failed verification.
This is a software policy pending WiFi-specific hardware qualification, not an
assertion of HC/CPWI coexistence, reset detection or solved communication faults.
Deployment and logical-reconnect hardware tests require separate approval.

## Links

- [Celestron rules](../../.github/instructions/celestron.instructions.md)
- [WiFi progress and evidence](../celestron-nexstar-aux-wifi-progress.md)
- [USB continuity, unchanged](0011-cgx-logical-reference-continuity.md)
- [Link-loss policy](0009-link-loss-and-relink-policy.md)
