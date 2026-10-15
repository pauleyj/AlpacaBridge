# Celestron SkyPortal AUX implementation progress

## Branch and scope

Started 2026-10-13 on `driver/celestron-nexstar-aux-wifi`, stacked on
`driver/celestron-cgx-usb` at `37de2b25`. The implementation plan is
`/home/pauleyj/src/open-astro/nexstar-aux-wifi-port-plan.md`.

Current base: upstream `main` at `782294de01347d9c0ffa3259f8dd69cb429af9a4`,
after PR #1148 merged. The existing WiFi commit was replayed as `55060596`;
review changes remain uncommitted and unstaged. See the rebase record below.

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

### Updated USB dependency — historical uncommitted merge

At the user's request, fetched PR #1148's newer head
`709f1976582f7a048c22d63c5c1c41c0038a25a1` and merged it in place with
`--no-commit --no-ff`. At that checkpoint `HEAD` remained `a6a0fa2c` and
`MERGE_HEAD` recorded `709f1976`. The imported merge was staged, while the WiFi
work was reapplied unstaged and both new tests remained untracked. No commit,
push or deployment occurred during that merge; it is superseded by the rebase below.

Before integration, the binary diff, all 16 changed/new files, external plan and
source refs were backed up and hash-verified under
`/tmp/opencode/aux-wifi-before-usb-merge-20261014T232344Z`.
Reapplication was conflict-free. All 15 non-CMake work files were byte-identical
afterward; CMake retains both the new upstream registrations and WiFi test entries.
The CGX telescope and its tests now match `709f1976` exactly, including checkpoint
reads outside the state mutex, provenance revalidation, disconnect-state cleanup
and the held-checkpoint responsiveness regression. Geometry and the published USB
ConformU report also match the new dependency exactly.

The USB branch also brings newer upstream Altair, SynScan, Bisque and CI changes;
these are imported unchanged, not additional WiFi features. Local repository
gates and **82/82** web tests passed after integration. Upstream Altair ConformU
reports retain their existing whitespace; non-report staged changes and the WiFi
working diff pass whitespace checks. The post-update native Celestron-enabled
Debug CTest passed **505/505** in **141.73 s**, including the imported held-checkpoint
regression and all combined USB/TCP/HC/lifecycle tests. The same post-update native
UBSan CTest passed **505/505** in **140.21 s**, with no reported undefined behavior.
Updated TSan also passed **13 stress/harness cases / 1035 assertions** and
**136 TCP/WiFi/CGX cases / 3225 assertions**, with `halt_on_error=1` and no reported
race. Core, driver and test translation units were instrumented; system Catch2 was
not. These are Celestron-enabled, hardware-free results, not all-vendor or ASan
qualification. All use isolated build directories; the installed service and mount are untouched.

### Rebase after PR #1148 merged — 2026-10-15

PR #1148 merged as `74c1fa98` from final USB head `d22d9687`. Fetched upstream
`main` at `782294de`; local `main` and the fork's `origin/main` now match it
(the remote update was a normal fast-forward, not a force-push). Backed up the
pending merge, all changed/new files and the external plan under
`/tmp/opencode/aux-wifi-before-upstream-rebase-20261015T004420Z`, plus backup refs
`backup/aux-wifi-pre-upstream-rebase-20261015T004420Z` and
`backup/main-pre-upstream-rebase-20261015T004420Z`.

Removed the superseded pending merge after saving the work, then replayed only
the existing WiFi commit onto upstream main: `a6a0fa2c` became `55060596`.
`git range-diff` reports the replayed patch unchanged. Reapplied every uncommitted
WiFi edit without conflicts; all 15 non-CMake work files were byte-identical
before the provenance-doc updates, and the CMake test additions applied cleanly.
The index is empty and the two new tests remain untracked. No new work commit,
WiFi push, deployment or mount command was made.

CGX telescope/geometry/tests and the USB report match current upstream exactly.
Compared with the tested `709f1976` snapshot, the sole Core src/include/tests
change in upstream is a CGX teardown explanatory comment; other new changes are
USB evidence/docs and ZWO SDK/report updates. Earlier native results above retain
their original source provenance; native C++ suites have not been rerun on `782294de`.
Post-rebase repository gates, whitespace checks and **82/82** web tests passed;
the external plan's 45 navigation links also validate.

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
narrow firmware cache and catalog selection are implemented. The candidate identity
profile now validates matching four-byte firmware (including the build high byte)
and exact two-byte CGX model `0x1788` on both motors. Legacy/mixed replies and
CGX-L are refused before takeover. These payloads come from USB evidence and
the fake, not a SkyPortal capture; qualification through SkyPortal remains required
before rig use. Next come reference/Sync and stop/motion implementation.
Connect must not manufacture a calibrated reference or guessed tracking state.

## Uncommitted review work — 2026-10-14

No new work commits, WiFi pushes or deployment are authorized before user review.
The USB merge was staged with explicit approval, then replaced by the requested
upstream-main rebase; all WiFi review edits are now unstaged. Only fork main was pushed.
The clean rebased baseline `a6a0fa2c` passed native arm64 Celestron-enabled Debug
CTest **480/480** in the isolated source snapshot at
`/home/pauleyj/alpaca-validation/aux-wifi-review-20261014` on the RPi4. This
baseline includes the USB protocol/telescope/telemetry/stress regressions; it
predates the working-tree changes below, so is not their test result.
The same baseline also passed Celestron-enabled native UBSan CTest **480/480**;
it is not an ASan result or evidence for the later working-tree changes.

An intermediate working-tree focused native arm64 run passed **37 cases / 841
assertions**, covering TCP safety/budgets/source selection, WiFi lifecycle/identity
and HC endpoint loss. It predates the seven additional report/frame cases; the
base library and system Catch2 were not instrumented. Before the USB update,
the complete Celestron-enabled Debug and UBSan suites each passed **504/504**.
These results include all transport/identity/ownership/serial-budget regressions
and the corrected HC fake below, but not the new USB checkpoint fix.

Working-tree changes:

- Modern both-motor identity validation and malformed/mixed/mismatched profile
  regressions; capabilities remain lifecycle-only.
- Incomplete TCP writes close and release the endpoint before another stop can
  append to a truncated command, report ACK or first-axis stop. Fully transmitted
  commands with lost replies retain the poisoned-link send-only stop escape.
- Report-overflow coverage retains the queued reports, faults the session and
  verifies both stop packets on the wire.
- Additional report/frame cases cover exact Dec/error/broadcast ACKs, unrelated
  sources/destinations, malformed payloads, post-reply reports, fatal ACK loss,
  invalid checksums and truncated final frames; all passed natively and under UBSan.
- TCP Connect/exchange/send-only stop include lock acquisition in the deadline;
  queued requests expire without dispatching or poisoning the active exchange.
  Serial keeps its qualified post-lock timing policy.
- Typed requests choose source `0x20`/`0x00` under link serialization; raw frames
  retain their explicit source. USB telescope/geometry and existing USB tests/report
  are unchanged.
- HC FIN/reset/fatal TCP I/O clears connection state and releases the shared
  endpoint claim; live response/send timeouts retain ownership. Blind response
  draining no longer swallows confirmed loss. The singleton reservation remains
  with the old HC driver until explicit teardown, preventing another HC instance
  from receiving its commands or being disconnected by its destructor. A two-driver
  regression covers this distinction and an independent AUX claim.
- A dedicated serial contention regression pins post-lock budgets for exchange
  and send-only stop without modifying the qualified CGX tests.

All **24** cases added in this review slice were observed failing against a
mutation on native arm64, in scratch copies only: 23 production-code mutations
and one helper mutation. The earlier 504-case attempts exposed incorrect short
binary firmware replies in the HC async-park fake, not a qualified USB driver
failure. The fake now sends two bytes plus a separate terminator; its new helper
case rejects the old reply. The superseded-sync-slew case joins its worker before
asserting an early start failure. No production assertions or tolerances were loosened.

### Review-slice observed falsifiers

Each case below passed in the clean native build and failed with the named single
mutation (Catch2 exit 42). The isolated records/logs are under
`AlpacaCore/build-review-focused/{falsifiers,extra-falsifiers}` in the remote review
snapshot. These are pre-USB-update observations; paths/lines below name the current
working tree. The repeated queued-I/O mutation after the relative-wait correction
is counted once, not as a 25th case.

- "Celestron AUX TCP - an incomplete frame closes the link before any stop can append bytes": AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp:397 disable partial-write disconnect guard.
- "Celestron AUX TCP - report overflow poisons the link without discarding pending reports": AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp:236 raise queue bound to 17.
- "Celestron AUX TCP - typed commands select the live source while raw exchanges retain their address": AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp:278 omit typed source assignment.
- "Celestron AUX TCP - queued I/O expires without dispatching or poisoning the active exchange": AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp:208 replace timed stop lock acquisition with blocking `lock.lock()`, retaining the expiry check.
- "Celestron AUX serial - queued exchange and stop retain their post-lock I/O budget": AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp:207 apply TCP timed-lock branch unconditionally.
- "Celestron SkyPortal AUX Driver - Modern firmware decodes the big-endian build": AlpacaCore/src/vendors/celestron/celestron_nexstar_aux_telescope_driver.cpp:507 discard build high byte.
- "Celestron SkyPortal AUX Driver - Malformed and mixed firmware identities refuse before takeover": AlpacaCore/src/vendors/celestron/celestron_nexstar_aux_telescope_driver.cpp:475 accept firmware payloads longer than four bytes.
- "Celestron SkyPortal AUX Driver - Mismatched and old firmware identities refuse before takeover": AlpacaCore/src/vendors/celestron/celestron_nexstar_aux_telescope_driver.cpp:486 disable firmware equality guard.
- "Celestron SkyPortal AUX Driver - Both motor models require the exact modern CGX identity": AlpacaCore/src/vendors/celestron/celestron_nexstar_aux_telescope_driver.cpp:493 check model only on RA.
- "Celestron HC TCP - FIN releases ownership during normal and binary replies": AlpacaCore/src/vendors/celestron/celestron_protocol_wrapper.cpp:1688 omit EOF loss classification.
- "Celestron HC TCP - RST send failure releases ownership for normal and blind commands": AlpacaCore/src/vendors/celestron/celestron_protocol_wrapper.cpp:1588 omit disconnect on fatal send.
- "Celestron HC TCP - RST response loss releases endpoint ownership": AlpacaCore/src/vendors/celestron/celestron_protocol_wrapper.cpp:1688 disable receive loss guard.
- "Celestron HC TCP - blind response loss is not swallowed": AlpacaCore/src/vendors/celestron/celestron_protocol_wrapper.cpp:501 omit confirmed-loss rethrow.
- "Celestron HC TCP - live send backpressure retains endpoint ownership": AlpacaCore/src/vendors/celestron/celestron_protocol_wrapper.cpp:1587 classify every send failure as fatal.
- "Celestron HC TCP - live response timeouts retain endpoint ownership": AlpacaCore/src/vendors/celestron/celestron_protocol_wrapper.cpp:1688 classify every nonpositive receive as loss.
- "Celestron HC TCP - a lost socket retains its driver's singleton reservation until teardown": AlpacaCore/src/vendors/celestron/celestron_protocol_wrapper.cpp:405 guard new connects on socket health instead of reservation.
- "Celestron AUX TCP - Dec errors and broadcast reports receive exact reversed-address ACKs": AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp:234 use application source instead of report destination in ACK.
- "Celestron AUX TCP - reports from other sources or destinations are ignored without ACKs": AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp:295 accept every report source.
- "Celestron AUX TCP - malformed controller report payloads poison the exchange before ACK": AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp:233 disable report length guard.
- "Celestron AUX TCP - a report after the reply is acknowledged and queued by the next exchange": AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp:237 omit pending report insertion.
- "Celestron AUX TCP - fatal report ACK send failure closes the link and releases endpoint ownership": AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp:391 omit fatal-send disconnect.
- "Celestron AUX TCP - a checksum-invalid final frame cannot satisfy an exchange": AlpacaCore/src/vendors/celestron/celestron_cgx_protocol.cpp:65 disable checksum mismatch guard.
- "Celestron AUX TCP - a truncated final frame closes the link and releases endpoint ownership": AlpacaCore/src/vendors/celestron/celestron_aux_link.cpp:434 omit disconnect on EOF.
- "Fake Celestron responder - firmware replies contain both bytes and a terminator": AlpacaCore/tests/test_celestron_async_park.cpp:81 return short `0#` firmware reply.

The initial latest-source TSan build exposed a GCC 14 libstdc++ compilation defect
in `timed_mutex::try_lock_until(steady_clock)` (`_M_clocklock` is absent in its TSan
configuration). Relative waits use the remaining original deadline and keep the
post-acquisition expiry check; a tiny instrumented relative-lock host probe passes.
After rebuilding the corrected source, the pre-USB-update Celestron-enabled TSan
run passed **13 stress/harness cases / 1035 assertions** and **135 TCP/WiFi/CGX
cases / 3203 assertions**, with `halt_on_error=1` and no reported race. Core and
driver/test translation units were instrumented; system Catch2 was not. The fake
fault warnings in these logs are not physical mount events. This snapshot predates
`709f1976`; the updated dependency's separate passing TSan result is recorded above.

Sibling sweep: the shared write helper covers normal requests, report ACKs and
both stop writes. TCP lock budgets cover Connect, exchange and send-only stop;
serial takes its existing blocking lock and starts its budget afterwards. Typed
source selection covers position GET/SET, FAST/SLOW GOTO, slew/home completion,
home start and guide start/status, leaving raw exchanges explicit. HC loss
cleanup covers normal/blind sends, ordinary/binary reads and terminator draining.
Identity validation applies to both motor versions/models and refusal cleanup.

The user approved a temporary read-only WiFi capture, with Ethernet SSH preserved
and no driver Connect/takeover. The association to `Celestron-13`
(`4C:55:CC:14:B4:13`) failed with NetworkManager `ssid-not-found`; the subsequent
fresh scan did not list it. `NCC-1701` was restored and only the newly created
temporary profile was deleted. **No AUX packets were sent**, and adapter firmware,
motor identity over WiFi and approach direction remain unresolved. Do not power
cycle or move the mount remotely to unblock this check.

On the user's renewed approval to continue read-only hardware tests, a fresh
RPi4 WiFi scan again did **not** list `Celestron-13` or its recorded BSSID.
Ethernet SSH and the original `NCC-1701` WiFi profile remain active. No temporary
profile, association attempt, AUX packet or USB query was made in this retry.
Capture is blocked on making the adapter reachable; ask the operator to confirm
power and Direct Connect/AP mode (or supply its current infrastructure address).

The user must be present for the first supervised motion-affecting test, including
Connect/takeover stops. No real tracking, Sync, Home, GOTO, guiding, manual motion,
Park or ConformU operation is authorized by these software checks.

Initial qualification target is a northern CGX. Identify the SkyPortal adapter
generation/firmware during read-only rig checks. Park, SetPark and Unpark remain
unsupported until mechanical Park and unmoved power-cycle recovery are qualified.
CGX-L, southern operation, native guide behavior and branch-changing GOTO routes
still need their own evidence. ConformU was not run.

Private-protocol baseline: INDIGO version 25 at
`d7abbb7886d87cea6f48a59017ee531bf9f42c9a`,
[AUX source](https://github.com/indigo-astronomy/indigo/blob/d7abbb7886d87cea6f48a59017ee531bf9f42c9a/indigo_drivers/mount_nexstaraux/indigo_mount_nexstaraux.driver)
and [development record](https://github.com/indigo-astronomy/indigo/blob/d7abbb7886d87cea6f48a59017ee531bf9f42c9a/indigo_drivers/mount_nexstaraux/REFACTOR.md).
