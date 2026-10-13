# Celestron SkyPortal AUX implementation progress

## Branch and scope

Started 2026-10-13 on `driver/celestron-nexstar-aux-wifi`, stacked on
`driver/celestron-cgx-usb` at `37de2b25`. The implementation plan is
`/home/pauleyj/src/open-astro/nexstar-aux-wifi-port-plan.md`.

The branch implements the shared serial/TCP AUX link and a selectable lifecycle-only
WiFi telescope; coordinate motion is still unavailable. `CelestronCgxProtocol` remains a source alias for the USB
backend; typed motor commands remain in `celestron_cgx_protocol_wrapper.cpp`,
while framing, connection ownership and bounded I/O live in
`AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp`.

Implemented:

- Numeric IPv4/IPv6 TCP connection with a bounded nonblocking handshake,
  `SO_ERROR` verification, and `MSG_NOSIGNAL` writes.
- Application source `0x20` on TCP, retaining `0x00` on serial.
- Shared HC/AUX endpoint claims, including IPv4-mapped IPv6 equivalence,
  failure cleanup and release on positive TCP transport loss.
- Fragmented/coalesced packet parsing, checksum validation, one exchange
  deadline, and a poisoned stream after an ambiguous exchange.
- Send-only axis stops on a poisoned stream, without reconnecting or claiming
  that the mount stopped.
- Validated motor warning/error acknowledgements and a bounded report queue.
  Reports queued between exchanges are parsed by the next exchange; there is
  no independent idle reader in this slice.

The transport/lifecycle fake deliberately has no motor dynamics or pointing model.
The WiFi telescope has connected contract-sweep and stress registrations.

## PR #1148 dependency reconciliation — 2026-10-14

Rebased the WiFi-only commit onto the exact open upstream CGX USB PR head,
`ac1ec57c0585291b10a155ebde5e39f39aedb32b` on `pauleyj/driver/celestron-cgx-usb`.
The CGX telescope, geometry, tests and USB ConformU report are unchanged from
that dependency. Shared-link conflict resolution retains kernel tty exclusivity,
same-owned-fd recovery, health/session generations and native SLOW GOTO support.
TCP still never autonomously reopens after desynchronization.

The earlier transport checks below predate this reconciliation and do not certify
the combined transport. Native arm64 USB/TCP regressions and sanitizer runs remain
required before deployment; no hardware commands or new ConformU run are claimed.

## Verification

Native arm64 checks ran on `pauleyj@192.168.4.44` (RPi4, aarch64,
kernel `6.18.34+rpt-rpi-v8`) in an isolated checkout at
`/tmp/AlpacaBridge-nexstar-aux`. No physical mount commands were sent.

- Celestron-enabled Debug CTest: **410/410 passed**, including the original
  seven TCP cases, HC/USB regressions, contract sweeps and lifecycle stress.
- After the two additional TCP cases and the final transport/typed-command
  split: focused `[tcp],[cgx]` run **60 cases / 653 assertions passed**.
- Standalone transport UBSan and TSan runs: **9 cases / 67 assertions passed**
  in each. The codec, AUX link, typed commands, HC wrapper and test translation
  units were instrumented; the base library and system Catch2 were not. These
  are transport checks, not an all-vendor sanitizer or driver-stress claim.
- ASan cannot run on this host: even an empty ASan program aborts in
  `sanitizer_allocator_primary64.h`, the documented Pi4 address-space
  limitation. Full ASan requires a compatible arm64 host.
- Local formatting, whitespace, docs drift, layering, contract/stress
  registration, connect-error hook, and changelog-fragment checks pass.

All nine new cases were observed failing against the production mutations below
on arm64, one mutation at a time, then the original code was restored and the
focused suite passed. Refresh line numbers before using these in a PR body.

## Falsified by

- "Celestron AUX TCP - fragmented replies use the WiFi application address": AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp:160 assign serial source 0x00 on TCP instead of 0x20.
- "Celestron AUX TCP - acknowledges coalesced controller reports without losing the reply": AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp:205 omit the controller report acknowledgement write.
- "Celestron AUX TCP - timeout poisons exchanges but transmits both axis stops": AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp:228 bypass the desynchronized exchange rejection guard.
- "Celestron AUX TCP - HC and AUX exclude the same endpoint and release claims": AlpacaCore/include/alpacacore/vendor/celestron/celestron_endpoint_claim.h:33 return from claim before inserting endpoint ownership.
- "Celestron AUX TCP - checksum failure poisons and EOF closes the link": AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp:365 omit TCP disconnect on end-of-file.
- "Celestron AUX TCP - continuous unrelated traffic cannot extend the deadline": AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp:232 use the five-second maximum instead of the requested exchange deadline.
- "Celestron AUX TCP - validates numeric endpoints before connecting": AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp:132 accept ports zero and 65536 before narrowing.
- "Celestron AUX TCP - a late reply cannot acknowledge another exchange": AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp:228 bypass the desynchronized exchange rejection guard.
- "Celestron AUX TCP - a refused connect releases endpoint ownership": AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp:276 omit the endpoint claim release during cleanup.

## Next slice and hardware gates

The Telescope V4 lifecycle-only backend, initial snapshot, fault/lost-state policy,
narrow firmware cache and catalog selection are implemented. Its legacy identity
assumptions must be reconciled with modern raw CGX replies and qualified through
SkyPortal before rig use. Next come reference/Sync and stop/motion implementation.
Connect must not manufacture a calibrated reference or guessed tracking state.

Initial qualification target is a northern CGX. Identify the SkyPortal adapter
generation/firmware during read-only rig checks. Park, SetPark and Unpark remain
unsupported until mechanical Park and unmoved power-cycle recovery are qualified.
CGX-L, southern operation, native guide behavior and branch-changing GOTO routes
still need their own evidence. ConformU was not run.

Private-protocol baseline: INDIGO version 25 at
`d7abbb7886d87cea6f48a59017ee531bf9f42c9a`,
[AUX source](https://github.com/indigo-astronomy/indigo/blob/d7abbb7886d87cea6f48a59017ee531bf9f42c9a/indigo_drivers/mount_nexstaraux/indigo_mount_nexstaraux.driver)
and [development record](https://github.com/indigo-astronomy/indigo/blob/d7abbb7886d87cea6f48a59017ee531bf9f42c9a/indigo_drivers/mount_nexstaraux/REFACTOR.md).
