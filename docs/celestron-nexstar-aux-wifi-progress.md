# Celestron SkyPortal AUX implementation progress

## Current checkpoint — 2026-10-21

Local implementation checkpoint, **not WiFi ConformU qualification**. AMW006
lost required Home exchanges on both AUX ports and both tested client addresses;
runtime stopping was unverified and operator-confirmed power-off ended each
failed run. Mount remains OFF on AUX2; all diagnostics, captures and software
jobs are closed. Production uses source20, strict parsing and original deadlines;
private address/pacing experiments are not production changes.

Operator has ordered a new generation-3 SkyPortal adapter. It has not arrived
or been tested; verify its actual model/firmware before any qualification claim.
Next hardware work is a powered-off adapter replacement, fresh identity/captures
and bounded Home/branch repeatability checks before broader guiding, HTTP and
full ConformU gates. No PR, push, deployment or release is implied by this local
checkpoint. Earlier entries below are historical and superseded by later results.

## Upstream integration — 2026-10-20

**Offline CFM inspection complete — no supported update/recovery fix found:**
read already downloaded CFM 3.0.6196 JAR `481de000` without executing target
code. AMW006 installs `NullProgrammer`/`TcelBridge`; boot/package/read/export
methods explicitly throw `Not supported`. This package provides no AMW006
update implementation; do not substitute AMW007/106 firmware. Version-aware
AMW006 queue adds 1,500 ms wait for app >=2.0.0, 3,000 ms for older/unknown,
confirmed milliseconds through underlying buffer. This is RESPONSE WAIT, not
mandatory send pacing. CFM retry loops are not proof of safe motion/fresh-read
recovery; none adopted. No required raw-TCP setup or flow-control fix established.

Static evidence/asserted bytecode checks: `/tmp/opencode/skyportal-cfm-offline/`;
local support brief updated. No hardware power-on, config/baud write, firmware
flash or external submission. Mount remains OFF on AUX2, production unchanged,
all jobs/controllers/captures closed. WiFi qualification still blocked.

**Both cadence experiments vetoed; all software jobs closed:** Home-only fake
timing/Abort check passed **1 case / 7 assertions**, negative control failed its
two intended gap checks, and real-clock Home/continuity/full branch/rehome passed.
These do not preserve travel observability: an assert-checked illustrative
reversal trajectory shows slower sampling can accumulate triple actual travel
before the same observed limit. This is NOT measured CGX dynamics. Reject
Home-only slower sampling for hardware, just as blanket pacing was rejected for
continuity. Deliberately stopped both remaining software-only suites, preserved
partial logs and `software-safety-veto.json`; **full suites incomplete, no pass
claimed**. Shell exit255 is cancellation, not a new hardware event.

No clearly redundant Home exchange identified without compromising current
state/safety observation. Production code, deadlines, decision records and travel
limits unchanged. Mount remains OFF on AUX2; no live jobs/captures/controllers.
Next escalation artifact prepared locally: `/tmp/opencode/skyportal-support-brief.md`,
privacy-scrubbed findings and questions on AMW006 forwarding/arbitration/cadence,
safe fresh-read recovery and supported firmware. **Nothing sent or published.**
Full WiFi ConformU remains blocked pending a defensible fix/hardware evidence.

**Blanket pacing rejected; narrower software-only Home-cadence study running:**
focused pacing checks passed **3 cases / 24 assertions**, unpaced control failed
as intended. Real-clock fake confirmed a contract regression: four six-query
continuity samples ×50 ms exceed the unchanged one-second checkpoint budget;
unpaced logical reconnect passed, blanket pacing timed out. No hardware use or
deadline expansion. Its fake STOP_UNVERIFIED is not a new physical event.
Broader unchanged blanket-pacing suite still running.

Separate private driver copy leaves ORIGINAL transport and ordinary/continuity/
stop/guide paths alone, reduces Home-only polling frequency with 250 ms queue/
ownership checks retained. Focused sample-gap/Abort timing, negative control,
real-clock branch/continuity rehearsal and unchanged suite are running; no result
yet. **Slower encoder observations weaken between-sample travel visibility even
with identical numerical guards; safety review required before any hardware.**
No new mount power-on or controller hardware launch. Production unchanged.
Pi `logs/private-home-cadence-preparation/`; investigation notes in
`/tmp/opencode/private-pacing-investigation.md`.

**Software-only cadence experiment running; mount remains OFF:** continued
review found pinned INDI AUX sender `52e469ee` waits 50 ms after successful
sends, including network use, versus our captured ~20 ms exchange cadence.
Swanson also suggests lower SkySafari readout rates. Neither establishes a
required spacing or explains this failure. Earlier "only replacement/electrical
capture can advance" assessment was too strong: a private cadence comparison
is still possible, but no production timing change is justified yet.

Private generated link adds bounded 50 ms post-send wait for ordinary TCP
exchanges, retains ORIGINAL deadlines, source20, parser/no-retry and arrival
policy. Zero-rate/motion stops bypass wait; send-only emergency stops/report
ACKs/serial unchanged. Three focused tests plus unpaced negative control and
UNMODIFIED driver/stress/TCP/report tests are running; no result yet. No paced
hardware controller built/launched or mount power-on requested. Any test failure
blocks hardware use; do not weaken assertions/deadlines to admit it. Evidence:
`/tmp/opencode/private-pacing-investigation.md`, Pi `logs/private-pacing-preparation/`.

**No spare cable/adapter available; WiFi hardware qualification blocked:**
operator reports no replacement hardware. Mount remains OFF on AUX2; no further
identical motion pilot scheduled. Final offline check confirms TCP's valid Dec
switch frame at +16.007 ms versus USB's damaged-looking span at +5.773 ms,
followed by a valid frame on both around +106 ms. This does not establish bus
electrical ordering/collision/repair. Required Dec position echo/reply is missing
on both; changing noise parsing cannot supply that telemetry. Next useful
hardware evidence needs qualified AUX electrical capture or access to known-good
cable/adapter. No speculative address/parser/deadline/retry change, no live jobs.
Endpoint analysis is complete to that limit; full WiFi ConformU remains blocked.

**AUX2 failure closed after operator-confirmed power OFF:** operator stated
"powered off". Confirmation recorded in
`logs/aux2-production-wh2xrdh2/operator-power-off-confirmed.json`; controller
and USB observer released using explicit confirmation, tcpdump closed with
**1,926 packets / zero kernel drops**. No live diagnostic/capture or software
job remains, no reconnect/further test performed. Mount remains OFF, adapter
on AUX2. Runtime stopping was never verified; physical power removal ended
the hazard. Missing exchanges on both ports/addresses remain unresolved.

**AUX2 failed — operator-confirmed power-off and closure recorded above:**
operator completed move then explicitly confirmed powered up/ready. Fresh
`aux2-production-wh2xrdh2` session ran exact production-source20 controller
`874a8377`, with raw receive-only USB and wlan1 capture. Cold takeover/public
Home passed (measured RA=3006 / Dec=54393). Normal GOTO passed on third SLOW
round, RA 2.204707″ / Dec 0.005150″, side0 and tracking off. Outbound internal
Home lost **Dec position 0x01** at 14:05:36.220 local. No return/final Home sent.
**STOP_UNVERIFIED: operator asked immediately to CUT MOUNT POWER, subsequently
confirmed power off; diagnostics closed as recorded above.** No further tests
or reconnect. No software jobs running.

Capture: 631 requests, exactly one missing echo/reply (`3b 03 20 11 01 cb`),
complete request TCP-ACKed +16.007 ms. USB damaged Dec-switch-like bytes at
+5.773 ms, valid switch frame/ACK around +106 ms; TCP has a valid switch frame
in that window. Whole streams are **not identical** (unlike earlier source0
failure). Preserve both; no repairs/substitutions. Stop/rate-zero ACKs present
but not proof of standstill. AUX2 is not a sufficient workaround. Further
identical WiFi repeats should give way to known-good cable/adapter isolation or
qualified electrical AUX evidence, after confirmed power-off.
Evidence `/tmp/opencode/aux2-production-wh2xrdh2/`; Pi
`logs/aux2-production-wh2xrdh2/fault-checkpoint/`. Production code unchanged;
full WiFi ConformU still blocked.

**Operator completed AUX1 → AUX2 move; power-on pending:** operator replied
"done" to moving the same SkyPortal to AUX2 while OFF, keeping cable/USB.
Production controller `874a8377` and receive-only observer hashes verified;
stage monitor adapted to a fresh port-comparison log pointer and self-check
passed. Exact controller's fake arrival-failure and lost-GOTO-ACK rehearsals
passed: healthy failure verifies a new stop; ambiguous ACK loss remains
STOP_UNVERIFIED/retained. No hardware opened or TCP controller connected.
Next: operator power-on, then fresh USB identity/exclusivity/route/capture checks,
verified cold takeover and bounded Home/Normal/BeyondPole/return/rehome,
stopping on the first failure. Hardware power-on has not been reported.

**Port-comparison software prepared, hardware NOT started:** current production
driver/link source hashes match the workstation; built controller `874a8377`
using unchanged source20 code (not experimental source0/report-filter objects).
Fake Home/Normal/BeyondPole/return/rehome passed with correct branch checks.
Pi artifacts: `logs/aux-port-comparison-preparation/`. No code changes, hardware
connect, new power-on or port move performed. Requires operator's physical
AUX1 → AUX2 move while OFF; leave mount OFF until fresh launch readiness.

**Closed-capture follow-up / next hardware isolation:** adapter TCP cumulatively
ACKed the entire failed source0 Dec query in **6.219 ms**, versus **16.844 ms**
for the earlier source20 failure. No corresponding AUX echo/reply on USB/TCP.
TCP-stack acceptance is not AUX forwarding/execution; native switch traffic
near each failure is correlation, not proof of collision. Neither address
substitution nor malformed-byte repair explains/solves both captures.
Proposed next hardware change: with power OFF, move the same SkyPortal AUX1 →
AUX2, same cable and USB observer; leave power off while preparing the unchanged
production-source20 bounded comparison. This isolates port/contact only (same
shared bus); a passing repeat does not prove reliability. No hardware move or
next power-on has been reported. If inconclusive, compare a known-good cable/
adapter or obtain qualified electrical bus evidence; do not add retries/drains.
Packet timings: `/tmp/opencode/source0-driver-v2-repeat-fault-xdxe2o_2/cross-address-timing.json`.

**Operator confirmed mount power OFF; failed source0 session closed:** after
the repeat's STOP_UNVERIFIED, operator stated "power is off". Recorded in Pi
`logs/tcp-source0-readonly-xdxe2o_2/operator-power-off-confirmed.json`, then sent
only the diagnostic release confirmation to the controller and USB observer.
Both exited; tcpdump closed with **11,726 packets captured, zero kernel drops**.
No reconnect, further motion test or hardware change performed. No live test/
capture or software verification job remains. Runtime stopping was never
verified; operator-confirmed physical power removal ended the hazard. Mount
remains powered OFF, and the source0 missing-exchange finding remains unresolved.

**CURRENT SAFETY HOLD — source0 full-driver repeat lost Dec 0x27:** private v2
`734186b6` passed Debug/UBSan/TSan (each **61 cases / 1,794 assertions**) and the
fake full branch route. First real Home/Normal/BeyondPole/return/rehome passed;
all three GOTOs needed three SLOW rounds, final RA 2.754″ / 2.908″ / 2.500″.
First capture: 1,629 requests answered, 3,306 incoming frames exactly mirrored
on USB, no noise. Second same-session Normal GOTO passed (RA 2.929″), then
outbound internal Home lost Dec guide-status 0x27 at 13:36:46.612. No return or
final Home was sent; no retry/reconnect. **STOP_UNVERIFIED: operator was asked to
cut mount power, then confirmed power off; sessions closed as recorded above.**

Fault capture: 2,132 requests; exactly one missing bus echo/reply, Dec 0x27
`3b 03 00 11 27 c5`, despite TCP client transmission. Both incoming TCP and USB
are strictly clean; all 29,366 server bytes match USB. Emergency stop/rate-zero
ACKs are present but do not verify standstill. This disproves source0 as a
demonstrated fix and shows malformed bytes are not required for the failure;
it does not identify the internal loss mechanism or justify retries.
Evidence: `/tmp/opencode/source0-driver-v2-repeat-fault-xdxe2o_2/`; first cycle
`/tmp/opencode/source0-driver-v2-branch-xdxe2o_2/`; Pi parent
`logs/tcp-source0-readonly-xdxe2o_2/source0-full-driver-v2/`.
Production source20/defaults/parser/deadlines/arrival policy remain unchanged.
No software job or retained hardware/capture process remains running.

**HTTP qualification staging prepared, not started:** isolated Pi working
directory `logs/http-conformu-staging-j2f93n5q/` contains only the WiFi telescope
configuration with the privately captured GPS latitude/longitude/MSL elevation.
Directory mode 0700, device JSON 0600; site round-trip checked without printing
coordinates. Separate port 6900, discovery disabled, motion watchdog retained.
No server started, mount command sent or installed service/config changed.
Discovery-off is not loopback-only: the existing server binds all interfaces.
An experimental source0 server is not selected by config; exact executable and
remaining hardware gates must be qualified before a handoff. Targeted v2
UBSan/TSan job remains the prerequisite for the pending bounded driver pilot.

**Private source0 v2 Debug verification passed:** legacy APP20 report negative
control failed both cases as intended; fixed report cases passed 2 / 21.
Full v2 driver/explicit-stress suite passed **61 cases / 1,794 assertions**, and
the fake Home/Normal/BeyondPole/return/rehome cycle passed. Debug controller
`734186b6` retains legacy motor-error visibility while issuing source0 commands.
Targeted UBSan/TSan rebuild/tests are now running; no sanitizer pass claimed yet.
No live controller replacement or overlapping hardware motion.

**ConformU environment refreshed:** current upstream and installed tool are
4.5.2, build 56139.4ccb591. Chrony offset ~0.108 ms, leap Normal. Fresh 3D GPS
latitude/longitude/MSL elevation captured privately with mode 0600; no exact
site values printed/committed or substituted. Site has not yet been applied to
the future server. These close environment-preparation checks, not motion/HTTP
qualification. Artifacts: Pi `logs/conformu-readiness-current/`.

**Private full-driver source0 verification:** address-only variant passed 59
cases / 1,773 assertions including explicit stress, plus the fake complete
near-pole branch cycle. It has NOT replaced the live raw Home diagnostic.
Review identified a safety concern: a source0 client must not ignore motor
warnings/errors still addressed to legacy APP20. Private v2 retains both current/
broadcast and APP20 motor reports with the original payload/queue/ACK/error
guards; the new regressions, negative control, full v2 tests/fake rehearsal
subsequently passed as recorded above. UBSan/TSan is now running; ASan host
limitation unchanged. No hardware motion overlaps these heavy jobs.

Current mount remains POWERED/stopped, raw TCP Home probe/USB observer retained.
After v2 verification, ongoing authorization permits the same bounded
Home/Normal/BeyondPole/return/rehome driver comparison without another approval
or hardware change. No old raw Home is adopted as driver calibration. Details:
`/tmp/opencode/source0-driver-v2-readiness.md` and Pi versioned preparation logs.
Production addresses/code and no-retry/framing/arrival policies are unchanged.

**Latest hardware — first WiFi/source0 Home passed cleanly (diagnostic only):**
after two 80-cycle source0 stationary passes, private TCP Home probe `73d60e49`
passed six exact TCP-backed fake safety cases and then one real bounded Home in
`tcp-source0-readonly-xdxe2o_2/source0-tcp-home`. Actual executable/blocking stdin
and fresh stationary admission verified. Eight samples, final RA=0 /
Dec=16776028, STOP_VERIFIED. All 100 source0 requests/echoes/replies plus eight
switch frames match USB exactly; no malformed bytes or replayed prefix. Powered
mount stopped, TCP probe/USB observer retained. No GOTO/guide/production reference.

**Startup backlog finding:** first source0 stationary connection prefixed 216
valid earlier frames, including all 208 frames of the previous USB source20
Home, plus terminal-looking bytes/association notice. Those earlier frames did
NOT appear on contemporaneous USB. Current 644 source0 request/reply pairs
matched live USB, excluding the old source20 replies by address. A second
same-address warm connection passed another 80 cycles with no prefix/noise and
exact USB/TCP bytes. Do not generalize that as a proven safe reconnect/drain
policy or call the whole first stream clean. Parent/reconnect analysis and raw
evidence: `/tmp/opencode/tcp-source0-readonly-xdxe2o_2/` and
`/tmp/opencode/tcp-source0-reconnect-xdxe2o_2/`.

Operator now authorizes tests on unchanged hardware without per-test approval;
notify for hardware changes, still stop on first error and require power-off
for unverified stopping. Production source20/defaults, parser and no-retry policy
are unchanged. Next work is private full-driver source0 preparation/tests before
the previously failing branch route; one clean diagnostic Home is not a fix or
ConformU readiness. Home evidence: `/tmp/opencode/tcp-source0-home-xdxe2o_2/`.

**Latest hardware — source20 USB with adapter fitted/NO TCP passed cleanly:**
operator confirmed off/on recabling and approved one Home. Same private probe
`a00f6b4b`, session `usb-source20-adapter-present-no-tcp-oejjotwx`, completed eight
samples, RA=2 / Dec=16775971, STOP_VERIFIED. All 100 source20 requests answered
plus eight valid switch frames; no malformed bytes. Mount remains POWERED and
stopped; probe retained. No WiFi client or other motion was started.

The six-cell USB/source/adapter/TCP matrix is complete. Only source20 with a
receive-only TCP client produced binary noise in these one-off Homes; different
seek histories and small samples do not prove causality. Generic NexStar Site
documentation describes AUX simultaneous-transmission corruption/requery, but
does not prove this mechanism here or authorize retries. Production framing,
addresses and no-retry policy remain unchanged. Matrix/evidence:
`/tmp/opencode/usb-source20-no-tcp-oejjotwx/` and Pi session.

**Next no-motion comparison prepared, not launched:** private TCP-source0
read-only probe `44f37ca3` changes only diagnostic source selection; no production
edit. Six native fake scenarios plus mixed-source negative control passed.
Proposal: 80 stationary WiFi GET cycles with receive-only USB capture, no motion,
as an admission test before any Home at this unqualified address/transport.
Fresh stopped probe-handoff/no-motion approval required; no recabling needed.
Readiness: `/tmp/opencode/tcp-source0-readonly-readiness.md`, Pi preparation logs.

**Latest hardware — source20 USB Home with adapter ABSENT passed cleanly:**
operator confirmed powered-off removal then USB-only power-on. Same checked
private probe `a00f6b4b`, session `usb-source20-adapter-absent-kpylkaq5`, completed
one Home in nine samples, RA=16776044 / Dec=1267, STOP_VERIFIED. All 108 source20
requests answered plus eight valid native switch frames; no malformed bytes,
checksum error or missing reply. Mount remains POWERED/stopped, probe retained.
Prior paired diagnostics/capture closed after confirmed setup change (1,260
packets captured, zero drops).

Source20 USB has now completed with shared `c8 fd` when adapter/passive TCP was
present, and cleanly when adapter was absent. This does not establish cause or
reliability; starting seek/history differ. Remaining discriminating cell:
source20 USB with adapter FITTED but NO TCP client, same probe/bounds. No new
binary needed; fresh power-off recabling/power-on and one-Home approval required.
No further motion sent. Full matrix/limits/evidence:
`/tmp/opencode/usb-source20-adapter-absent-kpylkaq5/` and Pi session.

**Latest hardware — source20 USB Home completed WITH shared binary noise:**
approved checked diagnostic `a00f6b4b` ran in paired session `g2i2s8a_/source20-usb`,
with blocking stdin/process identity verified and the same passive TCP observer.
Home completed in eight samples; final RA=16776355 / Dec=1459, STOP_VERIFIED.
Mount remains POWERED/stopped; both sessions retained. No further Home/GOTO/guide.

All 100 USB requests were source0x20 and answered. Eight native switch frames
also arrived. Raw audit nevertheless found **`c8 fd` on BOTH USB and TCP** between
Dec status replies. This phase's 208 valid TCP frames exactly match combined USB
TX/RX and request/reply order; TCP sent zero application payload. No required
reply was lost. WiFi-originated commands are therefore not necessary for this
noise event; source-address causality and relation to prior lost-query failures
remain unproven. Do not change production addresses or claim clean communication.

Next discriminating proposal: SAME source20 USB diagnostic/Home with SkyPortal
removed while power OFF; requires approved release, recabling and power-on.
No new binary needed, no automatic test. Removing the adapter also removes TCP,
so one clean result would not independently isolate those factors. Evidence:
`/tmp/opencode/usb-source20-home-g2i2s8a_/` and Pi paired `source20-usb/`.

**Address-controlled USB diagnostic ready, not launched:** a private source-0x20
USB build reuses the existing typed wrapper's validators through explicit raw
caller-source exchanges; production USB/WiFi defaults and parser/deadlines are
unchanged. Six native fake healthy/halt/lost-ACK/lost-query/travel/deadline cases
passed; independent address auditing rejected a mixed-source build. Experimental
probe `a00f6b4b`, sources/results under Pi `logs/usb-source20-home-preparation/`.
Serial send-only fault escape remains source0 and never proves stopping.

Next proposed gate is one source20 USB Home with existing adapter/passive TCP
listener retained, separating address from request origin. No recabling/power
cycle needed; await fresh stopped USB-probe handoff/motion approval. Correct
blocking FIFO and actual process-hash checks remain mandatory. Current source0
probe/TCP listener remain powered/stopped/retained; no experimental hardware
command sent. Readiness: `/tmp/opencode/usb-source20-home-readiness.md`.

**Latest hardware — USB-primary Home with receive-only TCP passed:** approved
corrected restart retained the existing listener/capture and used the same
probe `0c5e77e3`, with blocking stdin verified before spawn and in the actual
child. One USB Home completed in nine samples; final RA=16775152 / Dec=2730,
tracking/guide/motion off. Mount remains POWERED, both diagnostics retained.
Raw USB: 108 requests answered plus eight switch frames, no malformed bytes.
TCP contains the exact combined USB TX/RX frame multiset (224 valid frames),
matching application request/reply order. Observer sent zero application payload;
raw TCP equals captured payload. One Pi association notice, no binary corruption.

The three source-0 USB-controlled cells now pass (adapter absent, fitted/no TCP,
fitted/passive TCP); seek lengths differ and this is not a reliability estimate.
Remaining distinction is request origin AND source address versus failed WiFi
source-0x20 internal home. Next useful software preparation is a diagnostic-only
source-0x20 USB comparison preserving validation/bounds; production addresses
stay unchanged. No new motion authorized. Evidence:
`/tmp/opencode/usb-home-passive-tcp-g2i2s8a_/` and Pi `corrected-usb/`.

**Latest gate — passive-TCP comparison refused before Home:** approved pairing
started receive-only TCP and the same USB probe in `usb-home-passive-tcp-g2i2s8a_`,
but the new staged launch gave the probe a nonblocking control FIFO. Empty input
caused immediate `Home not dispatched` refusal, followed by verified stop/rate
cleanup. No Home/GOTO/guide was sent. Mount remains POWERED, stopped, with USB
probe, TCP observer and capture retained. The sequence ended at the launch error.

The correction is blocking `O_RDWR` probe stdin, matching the existing successful
launcher, with a blocking-mode assertion before spawn. Native fake red/green
reproduced the old refusal and proved corrected readiness waits without motion,
then accepts exactly one explicit Home. No binary/production change. Await
approval for a corrected stationary USB-probe restart while retaining TCP,
then the same one-Home/no-WiFi-writes scope. Evidence/correction:
`/tmp/opencode/passive-tcp-home-launch-correction.md` and Pi preparation logs.

**Latest hardware — USB Home with SkyPortal fitted/no TCP client passed:** after
operator setup/power-on confirmation and stopped probe release, identical USB
probe `0c5e77e3` completed one Home in `usb-only-home-H9grDfkP`. Final labels
RA=16775778 / Dec=1059, cumulative travel ~0.4233° / 0.2928°, nine home samples.
Raw capture: 108 valid source-0 requests/replies plus eight valid native switch
frames; zero malformed bytes/checksum errors/count mismatches. Mount remains
POWERED, stopped; probe/capture retained. No TCP client or WiFi command was used.

Both absent/present-no-client USB tests passed, but physical seek/travel differed
substantially; do not call starting geometry matched or infer adapter innocence
from one sample per setup. Next proposed cell: same USB-source-0 Home with an
already-tested receive-only TCP observer, NO TCP AUX writes. Existing checked
probe/observer suffice; no production change. No power cycle/recabling needed,
but stationary probe-handoff/one-Home approval is required before execution.
Evidence/limitations: `/tmp/opencode/usb-only-home-H9grDfkP/` and Pi session.

**Latest hardware — adapter-absent USB-only Home passed:** operator removed
SkyPortal while powered off, retained direct USB/no HC, then confirmed power-on
for one bounded test. Session `usb-only-home-7Qx0Xq5N`, checked actual/recorded
probe `0c5e77e3`, completed Home with RA=2109 / Dec=95666 after 43 home samples.
Observed cumulative travel RA 25,067 / Dec 536,199 counts (~0.538° / 11.506°),
below the diagnostic 15-degree limit. Raw capture: 380 valid source-0 requests,
380 matching replies plus 12 valid native switch frames; no malformed bytes,
checksum error or request/reply mismatch. No GOTO/guide/tracking-on/relabeling.
The mount remains POWERED and stopped, probe retained with STOP_VERIFIED; no
production-driver reference or optical accuracy is claimed.

This is one clean USB baseline, not adapter causality or WiFi qualification.
Next controlled comparison proposed: same USB probe/source/query sequence with
SkyPortal fitted in AUX1 but NO TCP client, changing adapter presence only.
Requires separately approved stopped release, power OFF before recabling and
operator setup/power-on confirmation. No new software is needed and no further
motion is scheduled. Evidence: `/tmp/opencode/usb-only-home-7Qx0Xq5N/` and Pi
session. Earlier shared-bus communication failures remain unresolved.

**Previous USB-only preparation (executed above):** a diagnostic using the existing
strict serial AUX link/source 0x00 is ready for one near-home attempt with
SkyPortal physically removed while power is OFF. It verifies a stationary
baseline and awaits an explicit Home command; it never homes at startup. New
15-degree cumulative travel and 70-second operation bounds are diagnostic-only,
not a production USB-driver change or physical stopping-distance guarantee.
Native fake healthy/halt/lost-home-ACK/lost-query/travel/deadline checks passed,
including one-home/no-query-retry auditing and unverified-stop retention. A
noncumulative-travel mutation failed to detect reversal travel; raw serial trace
and launcher refusal/syntax rehearsals passed. Probe `0c5e77e3`, pinned launcher
`103bc2f8`, preserved under Pi `logs/usb-only-home-preparation/`.

Mount remains powered OFF. Await operator confirmation of adapter removal,
near-home bare/clear setup and approval/power-on for this one USB-only test.
No new hardware connection or motion was made. A clean USB-only result would
not alone prove adapter causality: transport/source also change; adapter-present
USB-primary comparison would be the controlled follow-up if needed. Readiness:
`/tmp/opencode/usb-only-home-readiness.md` and Pi preparation evidence.

**Latest state — powered OFF after dual-observed branch fault:** after software
verification, the approved retained-session cycle passed Normal Dec-88.5 GOTO
(final SLOW RA 2.735178 arcsec), then outbound BeyondPole failed during internal
home on missing Dec `0x27`. No return/final rehome followed. STOP_UNVERIFIED
required operator power-off, confirmed before releasing both diagnostics.
Closed captures: driver 28,025 packets, paired session 33,438; zero kernel drops.

The failed request was TCP-ACKed in 16.844 ms but has NO AUX echo/reply on either
USB or TCP. Both instead contain `b3 2a 31 11 09 00 00 b0`; valid Dec switch
traffic follows at +103.868 ms. Entire incoming TCP stream (130,007 bytes)
matches USB unchanged, including corruption and emergency ACKs. All four stop/
rate-clear ACKs arrived within 80.352 ms of first emergency send, but standstill
was not runtime-verified. This is not solely a TCP/parser/reply-deadline issue.
Shared-bus/transmission arbitration is a hypothesis, not a proven mechanism or
identified faulty device. No framing, pacing, retry or arrival policy changed.
Do not repeat this WiFi motion pilot without a discriminating setup change.
Potential next isolation: separately approved bounded direct-USB Home with
SkyPortal removed while power off. Nothing is armed; hardware remains off.
Full analysis/evidence: `/tmp/opencode/dual-home-bMiQEClI/branch-fault-analysis.md`
and both closed Pi sessions.

**Current integrated software verification completed:** Debug CMake build passed;
CTest **635/635** in 708.65 s, explicit Celestron stress **12 cases / 43 assertions**
in 32.8 s. Six static/registration/changelog gates passed. Not a fresh full
all-vendor preflight/ASan pass, and not evidence that the hardware fault is fixed.

**Readiness work continuing:** the exact retained `f841343e` binary passed a
native fake home → Normal → BeyondPole → return → rehome rehearsal. No hardware
motion was sent. Current docs, contract/stress registration, layering,
connect-error and changelog checks passed. Comparison found the integrated
CMake source snapshot predating the standalone continuity candidate; differing
files were backed up before updating it. The integrated Debug build/CTest/
explicit Celestron stress run subsequently passed as recorded above; it did not
replace the live static controller.

Next hardware gate is a separately approved near-pole three-GOTO/return/rehome
cycle using the existing home/off driver and raw USB observer, without any
disconnect/handoff. It targets the previous failed return-home leg; no guides,
broader targets or ConformU included. Detailed scope, 20/45/45/70-second
watchdogs, different public/internal-home travel guards and first-error stop
criteria: `/tmp/opencode/dual-observed-branch-readiness.md` and Pi preparation.
Await explicit operator motion approval; do not overlap with heavy test jobs.

**Latest hardware — single dual-observed FindHome passed, 2026-10-21:** after
corrected-handoff approval, the older halted controller was released and pinned
launcher `81cfdb62` started `stationary-idle-bMiQEClI`. Recorded hash AND actual
running `/proc/<pid>/exe` matched `f841343e` before one public Home dispatch.
The existing raw/exclusive USB listener stayed open. Home completed with settled
labels RA=1171 / Dec=115008, home=true, slew/tracking/guide=false. Driver and USB
observer remain retained, with full captures active; no GOTO or guide followed.

Checkpoint: 480 outgoing / 970 incoming valid frames; all application requests
have matching echoes/replies. Four non-frame bytes `94 04 00 ae` occurred during
home. Crucially, the entire incoming TCP stream (6,588 bytes), including these
bytes and surrounding traffic, appears unchanged in USB. USB recorded the span
about 12.448 ms before TCP capture (different receive boundaries, not wire
latency). This was not canonical-TTY startup echo: USB was already held raw.
The shared bytes rule out our TCP parser/network-only forwarding as the sole
source, but do not identify the AUX participant or prove a collision/adapter/
firmware mechanism. No driver reply was lost in this Home; prior intermittent
failure remains unresolved. Do not loosen framing or infer reliability from one
pass. Evidence: `/tmp/opencode/dual-home-bMiQEClI/` and Pi session. No additional
motion authorized; full WiFi ConformU gates remain open.

**Latest gate — halted before Home, launcher mismatch:** operator approved one
public FindHome and stopped WiFi handoff with the raw USB observer retained.
The completed query probe released, but the remote legacy launcher selected the
older matrix binary `0c0028aa`, not the separately checked `f841343e`. Session
`stationary-idle-b09x5t1p` cold-connected idle/off; the readiness monitor refused
before sending Home because its expected status field was absent. Inspection
of the actual process, recorded hash and remote script identified the mismatch.
No Home was sent. Explicit halt completed, and controller/USB observer remain
retained. The hardware sequence ended; no automatic retry or continuation.

The intended binary had been checked without verifying that the remote launcher
would use it. A separate prepared launcher now hard-selects that binary and
checks its pinned SHA-256 before any network/controller side effects. Wrong-hash,
argument and syntax checks passed; native prepared-script checks passed. New
launcher `81cfdb62` has not run on hardware. Future readiness must independently
check recorded binary identity and `/proc/<pid>/exe` before dispatch. Details:
`/tmp/opencode/home-launcher-mismatch.md` and Pi preparation evidence. Await
explicit corrected stopped-handoff/single-home approval; no motion qualification
result from this gate.

**Latest hardware — reverse stationary comparison passed, 2026-10-21:** after
explicit approval, the completed `i2sD6SI4` diagnostics were released while
stopped and their capture closed (1,692 captured / 1,696 received / zero kernel
drops). New `wifi-passive-usb-9afIyMLl` passed all 80 stationary WiFi query cycles,
RA/Dec 1/1 throughout with clear slew/guide flags. All 644 source-0x20 requests,
644 echoes and 644 replies matched dispatch order; incoming TCP and passive USB
are byte-for-byte identical (8,700 bytes / 1,288 checksum-valid frames), without
non-frame bytes. No motion initiated; both new sessions and capture remain held.

Both stationary directions now pass and USB demonstrably observes source-0x20
traffic on this rig. Home-associated corruption remains unresolved. Next useful
gate is one separately approved public FindHome with the existing passive USB
observer held open, unchanged driver `f841343e`, public 15-degree cumulative
travel guard and 70-second diagnostic watchdog. No GOTO/guide/flip follows.
Operator presence/near-home setup and stopped WiFi-controller handoff approval
are required; no new motion has been authorized. Evidence/next-gate details:
`/tmp/opencode/wifi-passive-usb-9afIyMLl/analysis.md` and Pi session.

**Previous reverse stationary preparation — 2026-10-21:** a
diagnostic-only candidate sends bounded source-0x20 WiFi GETs while direct USB
is exclusively held raw and receive-only. Six native arm64 fake probe scenarios
passed (USB/TCP healthy and halt, USB lost reply, TCP corrupted reply), with
read-only/address auditing and no query retry after corruption. TCP/USB observers
passed exact capture, zero outgoing bytes, forbidden-command refusal and retained
transport loss; USB also checks raw/no-echo settings and explicit exclusive
claim/release. Wrong-TCP-source and missing-raw-mode mutations were caught.
Launcher syntax passed; reverse live orchestration is not yet qualified.
Final probe `2ae8e691`, observer `bb084a21`, launcher `05e4b2fa`; readiness
is `/tmp/opencode/reverse-observation-readiness.md` and Pi preparation evidence.
Current stopped diagnostics remain retained. Await explicit stopped-handoff and
80-cycle stationary-test approval; no new hardware command sequence has run.
USB may filter source-0x20 traffic, so absence of mirrored frames would not alone
identify physical bus corruption. No production policy changed.

**Previous stationary dual-interface result:** operator
confirmed CGX USB connected, SkyPortal in AUX1, HC absent and power on. Session
`usb-passive-observation-i2sD6SI4` completed all 80 USB-driven read-only cycles;
RA/Dec stayed at 1/1 with slew/guide flags clear. USB and passive TCP sessions
remain retained. No home/GOTO/guide or further controller handoff is authorized.

All 644 USB requests (source 0x00) and 644 replies appeared unchanged in TCP,
in matching request/reply order. The observer sent zero payload bytes; its
8,779-byte raw stream matches reassembled pcap. This is a bounded stationary
USB-driven pass, not WiFi-source-0x20, homing or ConformU qualification.

Before the first USB request, a separate 17-byte malformed startup span followed
28 valid switch-traffic bytes. The initial USB termios had canonical/ECHO/IXON/
ISIG flags before raw setup. Local PTY reproduction produced both malformed
spans exactly from the preceding Dec switch frames using those flags. This
strongly supports terminal echo processing for this startup pattern only;
earlier WiFi-only failures remain unexplained. No framing/deadline/retry or
production behavior changed. Raw checkpoints and analysis are in
`/tmp/opencode/usb-passive-i2sD6SI4/` and the Pi session. Complete capture remains
active; no further hardware query sequence is scheduled.

**Previous state — powered off after return-home fault:** the approved near-pole
cycle in `y9OgW4RV` passed Normal and outbound BeyondPole GOTOs, including
internal home/verified label restoration and tracking-off. Final SLOW RA errors
were 2.607771 / 2.791900 arcsec. The return leg failed during internal home on
RA guide-status `0x27`; no return-target GOTO or concluding rehome followed.
Stop remained unverified. Operator confirmed power off before controller release.
Closed capture: 15,906 packets, zero drops; 5,321 outgoing / 10,694 incoming
checksum-valid frames. Detailed failure analysis: `branch-return-failure.md` in
the Pi session and `/tmp/opencode/arrival-pilot-y9OgW4RV/`.

The failed request was TCP-ACKed in 13.936 ms and echoed at +25.095 ms, followed
immediately by malformed bytes `02 10 04 3a 02 a5` instead of a valid motor reply.
Valid switch traffic continued. Emergency stop/rate-clear ACKs arrived within
87.969 ms of the first emergency send, but no stop-verification samples could be
collected on the poisoned stream. Do not equate those ACKs with standstill or
repair/retry the malformed reply. Earlier slow/spaced/quiet-home experiments
already failed to establish reliability; no pacing change was reintroduced.

**Previous power-up preparation (subsequently executed above):** stationary direct-CGX-USB queries with a
receive-only SkyPortal TCP observer and simultaneous USB/TCP raw captures. This
requires attaching the CGX USB cable while power is off; its known by-id node is
currently absent. No homing/GOTO/Sync/tracking-on/guide commands in this scope.
Native fake USB checks passed healthy, halt and lost-reply cases, including a
read-only/source-0x00 command audit and no retry. Passive observer checks passed
zero transmitted bytes, exact fragmented capture, motion-command refusal and
retained peer EOF. Launcher syntax passed; live orchestration remains untested.
Final probe `51297a76`, observer `8caceddd`; the native strace rehearsal also
confirmed timestamped source-0 TX/RX and exclusive serial claim/release. The
observation budget is passed through each query's remaining deadline. Full
readiness/limitations are recorded in
`/tmp/opencode/usb-observation-readiness.md` and Pi preparation evidence. Await
operator setup/power-on approval; nothing is armed to run automatically.

**Previous hardware outcome — instrumented repeat passed:** after fresh operator
power-on/approval, `stationary-idle-y9OgW4RV` on `f841343e` completed home,
logical reconnect, one same-branch Dec 88.5° GOTO, explicit verified tracking-off
and rehome. Driver-reported SLOW RA errors were 35.091615 → 3.775843 → 2.768615
arcsec; final reported RA error ~2.236889 arcsec, Dec ~0.005150 arcsec. No
tolerance/correction limit changed. The prior ~3.055-arcsecond failure remains
unresolved. Latest state is home/off, controller/capture retained; no branch
change or guide sent.

Checkpoint: 917 outgoing / 1,861 incoming checksum-valid frames with matching
driver request/echo/reply counts. Another 21 non-frame binary bytes occurred;
RA switch messages/valid motor ACKs were 9/6, Dec 4/4. The ASCII association
notice identifies the Pi's own wlan1 station, not another client. This does not
resolve adapter/AUX corruption. Fresh rehome labels differ by −134 RA / +99 Dec
counts from initial home; do not infer optical home repeatability or reset-safe
calibration. Evidence: `arrival-checkpoint.pcap`, `arrival-diagnostic-results.json`,
`analysis.md` in the Pi session and `/tmp/opencode/arrival-pilot-y9OgW4RV/`.

**Previous hardware state — operator power cut confirmed:** the approved stopped
handoff to `stationary-idle-u7gK7OX8`, binary `d9427c23`, passed cold connect,
FindHome and one healthy logical reconnect with retained reference/tracking off.
The first same-branch Dec 88.5° GOTO then exhausted all three SLOW rounds. No
branch change or return leg was attempted. Harness reported STOP_UNVERIFIED and
retained the session; operator cut power and confirmed it before controller
release. Hardware is now off. Capture preserved: 4,887 packets, zero kernel drops,
1,638 outgoing / 3,286 incoming checksum-valid frames, no non-frame bytes.

Offline reconstruction from settled home labels and captured response completion
times gives RA arrival errors ~34.823, 3.802 and 3.055 arcsec over the three SLOW
rounds; final Dec error ~0.0052 arcsec. Driver timestamps are slightly later than
capture times. Keep the 3-arcsecond gate, three-round bound and no-compensation
policy unchanged. This was not the earlier home-time transport timeout and does
not establish that logical reconnect caused the miss.

Driver stop/rate-clear cleanup is visible in the capture. Afterward, 138 inspected
encoder replies over ~78 seconds stayed at RA=1117/Dec=68713, with motion/guide
flags clear. The runtime harness nevertheless could not verify stopping: its
full status call reread the retained motion error during pending cleanup and
threw immediately. That conservative runtime outcome is not retroactively marked
verified from the wire evidence. A fake red/green diagnostic-halt regression
reproduced the old immediate STOP_UNVERIFIED outcome, then passed with cleanup
that waits on Slewing before reading error-retaining status. On a healthy link,
it acknowledges the original completed-operation error once with a fresh verified
stop; known communication faults never take this path. Candidate diagnostic hash
`337bd7c39b56e7051ae3ffd78ff38bda524c5a275b5456b3838421b7c7ac24f9`.
Not deployed; no widened arrival tolerance or additional motion command planned.
Follow-up fake checks also passed both sides of the diagnostic distinction:
healthy arrival failure produces STOP_VERIFIED after explicit cleanup; a lost
GOTO ACK remains STOP_UNVERIFIED without entering the healthy-error acknowledgement
path. Diagnostic-only snapshot hash `1f8473fe1333c0b308204375c74fb6aa40ad24588cf1192e14737b4abbf8567d`.
The next diagnostic source adds AlpacaCore DEBUG records of each SLOW round's
RA/Dec error and raw counts; the harness enables that existing logging sink.
No motion algorithm, arrival tolerance, retry policy or correction count changed.
Focused GOTO tests and normal/failure/lost-ACK fake harness rehearsals are active
for that instrumented build. Focused Debug GOTO tests passed 10 cases / 208
assertions; normal, arrival-failure and lost-GOTO-ACK harness rehearsals all
subsequently passed. Instrumented controller SHA-256:
`f841343eee3d1732058ab25e10d5548bef51ad29f95841bc415a7cb45fcf5d9b`.
No algorithm/tolerance changes; hardware remains powered off.
Proposed next rig scope is a single unchanged same-branch GOTO with these DEBUG
errors/counts captured, rehoming only on success. No branch-changing leg, guide
compensation or arrival-policy change is proposed; a healthy failed arrival must
retain the diagnostic session after verified cleanup, and ambiguous stopping
still requires operator power-off. Ready for this scoped repeat; await new
operator power-on confirmation and approval before any hardware command.
Details: `analysis.md` and capture in `/tmp/opencode/continuity-pilot-u7gK7OX8/`,
also retained in the Pi session. Full ConformU remains blocked.

**Current candidate — guarded WiFi logical continuity authorized:** operator
approved a separate WiFi decision and implementation, recorded in decision 0016;
USB decision 0011 is unchanged. Healthy idle calibrated disconnect now verifies
stopping, takes a one-second read-only checkpoint and can retain the same TCP
session/endpoint claim. Logical reconnect verifies motor identity/firmware and
the original generation with three stable encoder comparisons within two counts.
Failed state/identity continuity discards calibration and uses normal verified
takeover; I/O failure closes/refuses the connection. No Connect homes or writes
labels. In-flight operations do not qualify. A poll fault during disconnect also
invalidates the candidate even though public state was already cleared before
joining. Cold/faulted/replaced-session references remain invalid.

The reconnect regression first failed four assertions against the prior policy.
Focused and full Debug/UBSan/TSan checks for the new policy are active, including
endpoint ownership/release, Sync continuity, firmware/count mismatches, checkpoint
deadline and a deterministic fault-during-disconnect case. Continuity-focused
Debug and UBSan each passed 5 cases / 88 assertions; combined driver/TCP/explicit-
stress Debug passed 73 cases / 2,011 assertions. Full UBSan and TSan subsequently
also passed 73 cases / 2,011 assertions each, with no sanitizer findings; all three
focused continuity runs passed 5 cases / 88 assertions. These are targeted
candidate-source runs against retained per-mode libraries, not full preflight.
A separate fake harness rehearsal passed logical reconnect
at home, after a Normal-branch GOTO and on BeyondPole, then returned and rehomed.
Harness SHA-256 `d9427c23b62b6ca48b5df9e9b679c532332538519ea0dc206a8f931441fdc761`;
log `logs/continuity-pilot-preparation/self-test.log` on the Pi. Not deployed.
Latest live `fwaBUrr8` read-only status remains home/off on `9300ab72`.
New controller hash was rechecked after these runs. Next gate is separate
operator approval for a stopped controller handoff and supervised logical
reconnect/near-pole branch-change/return pilot. Internal meridian homing has a
broader 180-degree travel guard than public FindHome's 15-degree limit; its
diagnostic branch-leg watchdog is 45 seconds. No new controller is deployed yet.

**Branch-route v3 completed:** Debug, UBSan and TSan each passed **69 cases /
1,976 assertions**, explicitly selecting stress. All four mutation trials failed
at the intended assertions: missing home dispatch, retained tracking/reference
during home Abort, incorrect restored Sync labels, and absent travel/stall errors.
Integrated CMake `alpacacore_tests` target rebuild passed; its complete runtime
suite was not rerun. The later continuity change is outside these v3 results.

**Branch-route verification update — 2026-10-21:** expanded v2 Debug passed
65 cases / 1,943 assertions (driver/TCP), including both branch directions,
exact restored Sync labels and the travel guard. Newer v3 Debug explicitly
selected `[flip],[celestron][stress]` and passed 8 cases / 88 assertions,
including no-progress stall cleanup and meridian-home lifecycle contention.
All v3 Debug/UBSan/TSan binaries compiled; combined driver/TCP/explicit-stress
runs are now active with filter `[nexstar-aux],[aux][tcp],[celestron][stress]`.
No complete v3 sanitizer result is claimed yet.
Four fake-only mutation trials are active: omit home routing, retain the
unverified home reference, discard Sync's home-label shift, and disable the
internal home travel/stall guards. Each must produce an ordinary regression
assertion failure; compilation/runtime crashes are not accepted as evidence.
The updated driver and three test/header files were also synced into the owned
Pi validation source for an integrated CMake `alpacacore_tests` target rebuild.
That build is active; it does not deploy or replace the retained controller.

Diagnostic branch pilot preparation adds explicit `goto-flip`/`goto-return`
commands restricted to Dec 88.25–88.75°, the expected starting branch and a
one-degree RA-axis envelope around the measured home meridian (with the
12-hour branch mapping). A measured home hour angle prevents saved sky RA age
from widening or spuriously tripping that physical envelope. Each leg has a
45-second diagnostic watchdog, verifies destination branch and explicitly
turns tracking off. Fake rehearsal is running; new pilot binary is not deployed.
Any branch-changing rig test or controller handoff still requires approval.
The full fake harness rehearsal subsequently passed home → small GOTO → branch
change → return branch → home, with explicit tracking-off and destination-side
checks on every GOTO. Candidate harness SHA-256:
`3d7e01a1f6cc7f40f0880f6b02c80504feda8da5b00b5fa5de7a1c39c8c5661a`.
Evidence: `logs/branch-route-pilot-preparation/self-test.log` on the Pi.
Hardware gates remain closed pending the full candidate sanitizer runs and
separate operator approval; no hardware command was sent for this rehearsal.

**Branch-changing GOTO software candidate — 2026-10-21:** the initial regression
failed against the prior branch refusal. Candidate routes a branch change through
both home switches on the existing I/O worker, verifies settled completion, then
restores and verifies Sync-adjusted home labels on the original healthy session
before the existing FAST/SLOW/arrival sequence. Reference is invalid during home
and label restoration; Abort in that interval never enables tracking. Internal
route shares the 180-second slew deadline, ten-second progress stall bound and
a 180-degree cumulative travel guard per axis. Public FindHome retains its
unchanged 15-degree near-home guard. None of these new bounds has rig evidence.
Initial focused Debug passed 3 cases / 37 assertions, including lost home/label
ACKs and home-stage Abort. Expanded tests add exact Sync-label preservation,
return branch and travel-limit cleanup; Debug/UBSan/TSan checks are running.
The next source snapshot (`branch-route-v3-preparation`) additionally injects
zero home encoder progress for the ten-second stall regression and adds a
meridian-home section to the existing active-GOTO lifecycle stress case. Run
that snapshot explicitly with `[celestron][stress]`; it is newer than the
currently running v2 checks. Formatting, docs drift, stress/contract registration
and whitespace checks pass locally. Production/stress sources remain uncommitted.
The candidate is not deployed; live `fwaBUrr8` remains on `9300ab72`, home/off.

**Coverage correction:** the earlier 61-case Debug/UBSan/TSan command selected
`[nexstar-aux],[aux][tcp]`. The concurrency source compiled but its registrations
have `[celestron][telescope][stress]`, so that command did NOT execute the stress
cases. Do not cite the 61-case results as concurrency-stress coverage. An explicit
stress run, with added meridian-home ownership section, is required for this
candidate. Earlier separately reported stress runs retain their own scope.

**Two additional bounded repeats passed — 2026-10-21 UTC:** after operator
requested continued testing, `fwaBUrr8` completed two more unchanged near-home
GOTO → verified tracking-off → rehome cycles without a runtime fault. RA target /
result pairs were 19.664108495663 / 19.664155949062 h and 19.669594488045 /
19.669637340028 h (about 2.5625 and 2.313 arcsec unprojected error); both Dec
results were 88.500001430511°. Last rehome completed epoch_ms 1792557714414;
latest state remains home/off, connected. Saved `bounded-repeat-checkpoint.pcap`
and `bounded-repeat-results.json`, copied locally with session log. No guiding,
larger route or branch-change test performed. These repeats do not establish
a fix for the earlier communication failures.

**Readiness priority:** stop repeating the already-passing near-home gate.
Full ConformU still requires implementation and fake-backed verification of the
branch-changing GOTO route, separate operator-approved broader-motion hardware
qualification, and resolution or explicit disposition of communication/guide
findings. Current initiator explicitly rejects a target on the other branch;
do not remove that safety refusal without implementing the complete route.

**Approved GOTO/off/rehome passed — 2026-10-21 UTC:** `fwaBUrr8` completed the
bounded Dec 88.5° GOTO with RA target/result 19.527518965087/19.527560251144 h,
Dec result 88.500001430511° (about 2.2294 arcsec unprojected RA / 0.0052 arcsec
Dec). Explicit tracking-off completed. The external sequence monitor initially
refused rehome because its last log status predated tracking-off; fresh status
confirmed idle/off, then the already-approved rehome was dispatched. This was
a monitoring admission pause, not a driver fault or additional motion scope.
Rehome completed at epoch_ms 1792557217237; latest state remains home with
tracking/slewing/guiding off, controller/capture retained.

`goto-rehome-checkpoint.pcap` reassembles to 11,539 outgoing / 23,107 incoming
checksum-valid frames, with matching driver request/echo/reply counts. New stray
bytes `c4 ff` occurred since the home checkpoint; prior seven-byte noise remains.
Thus the successful sequence does not resolve corruption or qualify reliability.
No guiding, branch change or full ConformU run performed. Evidence copied to
`/tmp/opencode/stationary-pilot-fwaBUrr8/` and retained in the Pi session.

**Cross-capture follow-up — 2026-10-21:** strict TCP/AUX analysis compared five
captures. `MpWUYz7g` failed home without non-frame noise; `Kvo8z4ae`, `LkbNrhl5`
and `fwaBUrr8` completed their approved sequences despite noise. Thus stray bytes
are neither necessary nor sufficient for the driver timeout. The malformed
switch-like sequences in `q4fysIKG` and `Kvo8z4ae` precede intact matching switch
requests/ACKs by 98.558 and 100.308 ms respectively. `fwaBUrr8` repeats a Dec
switch request after 100.022 ms, then receives its ACK. This does not identify
the faulty component or justify protocol-policy changes. Detailed comparison:
`cross-capture-comparison.md` beside the retained q4fysIKG evidence. Latest live
read-only status remains home with tracking/slewing/guiding off; no further
motion dispatched. Proposed next motion gate is one bounded GOTO, explicit
verified tracking-off and one rehome, subject to operator approval.

**Single approved FindHome passed — 2026-10-21 UTC:** in `fwaBUrr8`, command
status at epoch_ms 1792556038454 through completion at 1792556042960 showed
successful home, then home=1, slewing/tracking/guiding off. No GOTO or guide
dispatched; connection/capture retained. Home checkpoint has 4,942 outgoing /
9,901 incoming checksum-valid frames, all driver request/echo/reply counts
matching. However additional non-frame bytes `c8 bd a7 a0 14 4a b2` appeared
since the stationary checkpoint. Successful home therefore does not resolve
corruption or qualify reliability. Evidence: `home-checkpoint.pcap` beside
the stationary checkpoint. Further motion requires separately approved scope.
Timestamp follow-up places the seven bytes at epoch 1792556040.337884,
1.884 seconds after the home-request status, immediately after a valid Dec
switch `31->11 09 04 00` frame. The same switch request recurs 100.022 ms later,
then receives `11->31 09`. The checkpoint has four switch requests versus three
motor acknowledgements on that address pair. This is evidence of a disrupted
switch exchange during home, not proof of which component corrupted it; all
driver-addressed transactions still completed. No hardware commands were sent
for this offline analysis.

**Stationary observation passed — 2026-10-21 UTC:** 600.127 seconds monitored,
614 status samples, no faults or unexpected state. `stationary-idle-fwaBUrr8`
remains connected, home=0, tracking/slewing/guiding off at the latest check.
Checkpoint reassembly found 3,331 outgoing / 6,668 incoming checksum-valid AUX
frames, with an adapter association notice as non-frame ASCII. The checkpoint
ends with one RA guide-status request in flight; subsequent status remained
healthy. No malformed switch-like bytes were found in this checkpoint. This
does not prove a repair or isolate the prior home-associated failures. No motion
test performed. Evidence: `stationary-checkpoint.pcap`, `stationary-observation.json`
in the Pi session and `/tmp/opencode/stationary-pilot-fwaBUrr8/`.

**Stationary hardware observation setup:** operator confirmed power-up. Restored
the existing wlan1 profile and verified the route to 1.2.3.4; cold connect passed
in `stationary-idle-fwaBUrr8` with the prepared `9300ab72` controller. Initial
state: home=0, slewing=0, tracking=0, guiding=0. Detached capture/journal and a
ten-minute status monitor are active; monitor rejects faults, unexpected state
or status older than five seconds. No home/GOTO/guide/tracking-on command sent.
Outcome pending; session will remain retained after observation.

**Ready for a stationary power-up test:** timestamp-level analysis of `q4fysIKG`
found the missing RA `0x01` request at epoch 1792552286.475663 was TCP-acknowledged
in 1.801 ms. Malformed switch-like bytes followed at +21.268 ms; valid switch
traffic at +119.826 ms. Emergency stop/rate-clear ACKs all arrived within
88.006 ms of the first emergency send. This does not establish physical stopping
or distinguish adapter forwarding from AUX-bus corruption/arbitration. No retry,
timeout or parser policy changed. Detailed analysis and the next-test procedure:
`/tmp/opencode/abort-pilot-q4fysIKG/timing-analysis.md`, also copied to Pi evidence.

Prepared next step is a supervised **ten-minute stationary cold-connect** capture
using unchanged `9300ab72`: no home, GOTO, Sync, guide or tracking-on commands.
Connect retains its existing stop/rate-zero takeover, so this is not passive.
Any fault/unexpected motion/missing fresh status ends testing; retain the session
until verified stopping or operator-confirmed power-off. Success retains the
stopped controller for analysis, not automatic progression to motion. Pi checks
found no competing controller/TCP session, and verified capture privilege, WiFi
profile and binary hash. A 20-second fake stationary rehearsal passed at least
15 healthy samples, diagnostic halt and stopped release. Await operator power-up
confirmation; no hardware command has been sent during this preparation.

**Powered-off verification:** fresh combined WiFi driver and AUX TCP checks
passed Debug, UBSan and TSan, each **61 cases / 1,889 assertions**,
with no sanitizer findings. Expected injected timeout/error logs are not hardware
traffic. Logs: `logs/power-off-verification/{debug,ubsan,tsan}-tests.log` in the Pi
validation root. These runs compiled current driver/test sources against the
retained per-mode libraries, not a fresh whole-repository rebuild. Documentation
drift and layering checks also passed. Mount remains powered off; hardware reply
loss/corruption and branch-changing GOTO remain unresolved ConformU blockers.

**Latest hardware state — powered off after failed rehome:** the operator-approved
stopped handoff to `stationary-idle-q4fysIKG`, binary `9300ab72`, completed after
the final Abort candidate passed Debug/UBSan/TSan (47 cases / 1,651 assertions
each; idle-Abort regression failed three assertions against the preceding code).
Initial FindHome and the bounded active-GOTO Abort passed. The log records
`motion_pending=1`, then `slewing=0 tracking=1 guiding=0`, followed by the
harness's verified explicit tracking-off. Subsequent rehome failed on RA encoder
query `0x01`: 2,892 ms elapsed against a 2,890 ms remaining response budget at
23:11:29 local time. The harness reported `HALT_UNVERIFIED` and retained the
session. The operator confirmed power off and observed that the mount appears
home; only then was `power-off-confirmed` sent to release the controller.
No valid home reference or physical stop before power-off was established.

Closed capture: 1,391 packets, zero kernel drops. Strict reassembly found 453
outgoing / 924 incoming checksum-valid AUX frames. RA encoder queries have
61 requests but only 60 echoes and 60 replies; all emergency stop/rate-clear
requests have corresponding replies. Those ACKs do not prove standstill.
Stray incoming bytes were `44 ae`, `c8 fd`, and `db 2a 31 11 09 04 00 ac`.
This repeats the missing-request-echo/reply pattern during home observed in
`MpWUYz7g`, now on a different axis/command; root cause remains unresolved.
Evidence is preserved in the Pi session and `/tmp/opencode/abort-pilot-q4fysIKG/`.
Keep hardware off pending investigation; full ConformU remains blocked.

**Previous session — repeated-direction pilot completed:** approved stopped
handoff released `stationary-idle-Kvo8z4ae` and started `stationary-idle-LkbNrhl5`,
binary SHA-256 `173cfaf8b20ab913cf23c06cc54406afeff0da919ca9b810d3ea58f07208ab62`.
The harness refreshes home RA only when AtHome verifies the mechanical position,
without widening its physical envelope. A 260-second fake delay reproduced the
old admission failure; the corrected harness passed delayed GOTO and N/N/S/S.
The approved bare-mount home → Dec 88.5° GOTO → one-second half-sidereal N/N/S/S
→ rehome sequence passed without a runtime fault. Last observed state was home,
tracking off, not slewing or guiding; controller and capture remain retained.
Measured Dec displacements were +8.4972, +7.5703, −7.7248 and −7.5703 arcsec
versus 7.5205 nominal. These bounded encoder results do not qualify optical
accuracy or identify the first/reversal excess mechanism. No compensation added.
`guide-repeat-checkpoint.pcap` has 1,164 outgoing and 2,348 incoming checksum-valid
frames, plus stray incoming bytes `48 ff`; adapter/AUX corruption remains open.
Local evidence: `/tmp/opencode/guide-pilot-LkbNrhl5/`.

**ConformU preparation:** Pi ConformU 4.5.2 build 56139.4ccb591 installed with the
pinned release checksum verified; chrony reported about 0.7 ms offset. The
attached u-blox GPS, read through the existing gpsd service, supplies a current
3D fix and sea-level elevation for site configuration (exact location retained
outside committed reports). Full motion qualification is blocked by the
unimplemented branch-changing GOTO route and remaining guide qualification.
The operator approved correcting healthy interrupted equatorial GOTO Abort to
restore tracking after verified stopping, retaining its measured reference.
Diagnostic halt must explicitly turn Tracking off after that normal Abort.
The candidate passed 25 assertions in two focused cases, including lost stop
and restoration ACKs; the prior implementation failed four assertions. Broader
tests remain pending. The diagnostic harness rehearsal also passed an active-GOTO
Abort followed by explicit tracking-off; candidate harness SHA-256
`40744b75e282d9a0a5fa508e63a9f78f1a759c94697d54ead1d364b41d4ab9a5`.
The cancellation review checked stop ownership, refusal of competing initiators,
disconnect's state clear before worker join, and fault-path restoration-flag resets.
This candidate is not deployed to the retained hardware session. Documentation
drift checks pass; corrected the outstanding schema include-order formatting.
The same contract review found idle tracking must survive a no-motion Abort;
that sibling correction and its tracking/reference/no-write regression are now
under red/green Debug, UBSan and TSan checks. Diagnostic halt still explicitly
turns Tracking off. The operator approved a stopped handoff and bounded
home/GOTO/Abort/full-stop/rehome hardware pilot after these checks pass.
The updated harness passed both idle-tracking halt and active-GOTO Abort/full-stop
rehearsals, including rehome after each, against the fake. Candidate binary:
`9300ab728045f94bd149f80f49184c970e3178d1bc298824ac6c1104a9c1d659`.
Debug/UBSan/TSan results for the idle-Abort sibling change are still pending;
this rehearsal alone does not release the approved hardware gate.
The preceding GOTO-Abort restoration candidate passed Debug, UBSan and TSan,
each with 47 cases / 1,649 assertions and no sanitizer findings. These results
predate the idle-Abort sibling correction; its separate checks remain running.

**Previous retained state:** the same-direction Dec follow-up was blocked before
GOTO dispatch by the harness's near-home envelope at epoch_ms 1792549692974.
About 280 seconds had passed since its saved home RA; stationary home RA advances
with sidereal time. The harness halted, reported `STOP_VERIFIED`, and retained the
diagnostic session. No repeat guide pulses were sent. Fix/verify home-pointing
freshness in the harness before another delayed pilot; this was not an AUX timeout.
The completed guide capture's ACKs were 14–32 ms, first inactive samples about
1.165–1.180 s after send, with flat sampled Dec baselines. Earlier USB Stage 3
also showed first/reversal Dec excess and near-nominal repeats; that resemblance
does not establish its cause or justify compensation.

**First completed guiding pilot:** operator authorized continuation unless error
in `stationary-idle-Kvo8z4ae`, binary `ffd1f7d0`. FindHome, Dec 88.5-degree GOTO,
four one-second guides (North/South/East/West), verified tracking-off after each,
and rehome completed without a runtime fault. GOTO arrival error was about
2.143 arcsec unprojected RA / 0.0052 arcsec Dec. The controller remains connected
at home, tracking off. Analysis found anomalies, so further motion is held:
North/South displacement was +9.5015/-8.5745 arcsec against 7.5205 nominal;
East/West was +7.4788/-7.5010 arcsec. All direction signs and wire payloads were
correct (signed 50%, 100 centiseconds, one packet/ACK each). Independent raw Dec
deltas were -123/+111 counts, with 12 identical post-inactive Dec samples after
each pulse. No Dec tail was observed in those windows. The capture also contains
seven stray bytes during first-home switch traffic, despite all decoded AUX
frames having valid checksums. Neither guide accuracy nor adapter reliability is
qualified. Detailed evidence: `guide-checkpoint.pcap` and `analysis.md` in the
session directory; local copy `/tmp/opencode/guide-pilot-Kvo8z4ae/`.

**Latest stationary follow-up:** after operator-confirmed power-on, restored the
existing wlan1 profile and connected the same `ffd1f7d0` guide controller in
`stationary-idle-Kvo8z4ae` (TCP client port 39706). Two minutes of explicit
monitoring passed; 140 status samples from epoch_ms 1792548826980 through
1792548966209 reported `home=0 slewing=0 tracking=0 guiding=0`. No home, GOTO or
guide command was sent. Controller and capture remain live. The copied capture
checkpoint reassembles with valid checksums (793 outgoing / 1,590 incoming AUX
frames); it ends during an in-flight RA guide-status query, with later status
logs still healthy. Stationary success does not resolve the earlier home-time
failure or prove switch traffic caused it. Further home motion awaits operator
confirmation of the next bounded step.

**Latest hardware outcome:** after the operator returned, the approved stopped
handoff from `stationary-idle-fP3kj2sx` completed to guide binary `ffd1f7d0`, session
`stationary-idle-MpWUYz7g`. Cold connect correctly reported no reference. FindHome
then failed on Dec `0x27` (guide status), a 2,929 ms response budget timeout at
controller log time 21:34:22.292. Abort could not verify stopping; the harness
retained the faulted session and requested physical power-off. The operator
confirmed power off, after which `power-off-confirmed` released the controller.
No GOTO or guide pulse was sent. Capture closed with 376 packets and zero kernel
drops; no TCP session remains. Physical stopping before power-off is unverified;
root cause remains unresolved. Leave hardware off. Initial strict TCP reassembly
found 120 outgoing and 246 incoming AUX frames, all checksum-valid, with no
capture gaps. Dec `0x27` has 17 outgoing requests but only 16 incoming echoes and
16 replies. Both axes' emergency `0x06` rate clears and `0x24` stops have matching
reply counts; these acknowledgements do not establish physical standstill.
The capture is also preserved locally under `/tmp/opencode/guide-pilot-MpWUYz7g/`.

Offline timing analysis (`analysis.md`, `tcp-timeline.txt`, `tcp-hex.txt` beside
the capture) confirms the adapter TCP-acknowledged the missing request in
23.970 ms while forwarding four valid switch/motor `0x09` frames. No matching
Dec `0x27` reply or request echo followed. All four emergency stop/rate-clear
commands received AUX replies within 74 ms of the first stop send. TCP remained
responsive to keepalives; the driver reset it after its existing 30-second fault
window, while the harness process stayed retained. Thus retaining the harness
did not retain the socket indefinitely. Beacon loss occurred much later, around
power-off. Adapter forwarding versus AUX arbitration remains unresolved; no
timeout/retry policy was weakened. Four targeted transport regressions passed
(`MpWUYz7g-transport-failure-checks.log`). Next hardware step is supervised cold
connect and stationary polling, to distinguish idle communication from home
switch activity, before any further home/GOTO/guide command.

Updated vendors-OFF integrated verification passed **301/301** in 54.19 seconds
in a private network namespace (`snapshot-integrated-no-vendors-tests.log`).

**Hardware hold:** the operator stepped away after approving the guiding pilot.
That approval is paused until their return. Preserve the retained controller and
connection; no controller handoff or new motion while they are away.

Software-only follow-up for #1441 separates stale telemetry reads from control
freshness checks in the existing shared accessor. Sibling control sites retained:
Tracking, TrackingRate, FindHome, idle Abort, PulseGuide (including zero duration),
asynchronous equatorial GOTO and Sync; target variants delegate to those checks.
Getters retain connection/fault/reference/operation guards and snapshot timestamps.
The fake-clock regression holds an in-flight reply while the old frame ages:
**14 failed assertions** on the old driver, **52 passing assertions** on the
candidate, including DeviceState age and unchanged wire traffic on refused
commands (`snapshot-policy-red.log`, `snapshot-policy-green.log`). A separate
candidate binary leaves the sanitizer job's source/build inputs untouched.
The independent candidate passed **46 focused cases / 1,627 assertions**
(`snapshot-policy-focused-debug.log`), including existing fault/reference-loss
coverage. The repaired candidate subsequently passed **TSan: 46 cases / 1,631
assertions**, with halt-on-error and no sanitizer findings
(`snapshot-policy-repaired-home-tsan-tests.log`). Integrated verification remains
pending; this is not hardware qualification.

The first focused candidate UBSan run finished **45/46 cases**, with two failed
assertions in ambiguous tracking cleanup and no undefined-behavior diagnostic.
The test inspected the fake's receive log immediately after send-only cleanup;
socket send completion does not synchronize the fake's consumer. Added a bounded
condition-variable wait for the new Dec stop, also replacing the insufficient
request-count barrier in the ambiguous-home sibling. Both cases inject 10 ms
reply-processing delay so consumption is observably asynchronous. Original red
log: `snapshot-policy-focused-ubsan.log`. Repaired Debug passed **46 cases /
1,631 assertions** (`snapshot-policy-repaired-debug-tests.log`). Repaired UBSan
finished **45/46 cases, 1,626/1,627 assertions**: the lost-guide-ACK test did not
observe the published link fault within its three-second wait (line 222), with
no UB diagnostic. The tracking/home receive-barrier checks passed. An isolated
guide-case rerun passed **1 case / 6 assertions**, with immediate fault publication
after the timeout (`snapshot-policy-ubsan-guide-isolated.log`). The failing run
had about 16 seconds between timeout and fault-publication log entries; scheduling
contention is suspected, not established. The full UBSan repeat with unchanged
assertions passed **46 cases / 1,631 assertions**, with halt-on-error and no UB
diagnostics (`snapshot-policy-repaired-home-ubsan-repeat.log`). Debug, UBSan and
TSan now each have a passing focused candidate run; the earlier timing failure
remains unresolved and preserved in `snapshot-policy-repaired-home-ubsan-tests.log`.
Native cppcheck 2.17.1 passed 11 changed production files before this cleanup;
web tests passed 104/104. Clang-tidy found unchecked site/reference optionals,
uninitialized aggregate declarations, an oversized private enum and a copied
exception_ptr in the candidate. Site reads now share a checked accessor, queued
GOTO/guide reference reads check their invariant explicitly, aggregates are
initialized, and the private enum/exception argument are corrected. The repeat
passed with the source filter, so unchanged upstream headers are not treated as
branch findings (`snapshot-policy-clang-tidy-final.log`). The merged-source
all-vendor build passed (`upstream-697bceb5-all-vendors-build.log`). Integrated
the snapshot driver, repaired fake/tests and contract-sweep formatting into the
validation source tree. The integrated all-vendor build passed; CTest passed
**2,724/2,725 tests** in 1,693.71 seconds. Only `test_discovery` failed at its
expected-port assertion (line 139); its isolated-network rerun passed **1/1**
in 3.49 seconds, preserving the host network and mount connection
(`snapshot-integrated-all-vendors-build.log`, `snapshot-integrated-all-vendors-tests.log`,
`snapshot-integrated-discovery-isolated.log`). This is a full run plus one isolated
rerun, not a single clean full-suite run. The updated vendors-OFF build/test run
is underway in a private network namespace.
The retained hardware controller was not replaced.

Merged upstream `697bceb5` in local merge commit `0992c494`; published HEAD
remains `0b9aa128`. Restored the uncommitted implementation/tests/docs after the
merge, retaining the safety stash. The shared AUX link now also preserves the
upstream focus motor's 9600-baud option and serial-only `resync()`; TCP cannot
clear a poisoned stream through that API. Both WiFi and focuser catalog/build/
contract registrations remain present. Added baud-selection and poisoned-TCP
resync regressions. Native arm64 merged-source Debug passed **625 cases /
11,800 assertions** (`upstream-697bceb5-full-debug.log`). Merged-source Celestron
**UBSan and TSan each passed 305 cases / 6,975 assertions**, with halt-on-error
and no sanitizer findings (`upstream-697bceb5-home-ubsan-tests.log`,
`upstream-697bceb5-home-tsan-tests.log`). These runs predate the snapshot and
receive-barrier follow-ups above; those candidates have separate verification.

Reviewed new decisions 0013 (designs to avoid), 0014 (UTC offset) and 0015
(SDK seams), plus the new scoped Rules/adoption gates in existing records.
With operator authorization, registered WiFi TaskClock/AsyncOperation adoption
and **partial** StateSnapshot adoption. [Issue #1441](https://github.com/open-astro/AlpacaBridge/issues/1441)
tracks the existing stale-only telemetry refusal against decision 0010; the
same helper gates motion, so removing its stale guard is not a merge fix.
Decision-record changes still need the repository's PR authorization/review
checks. Documentation drift, layering and stress registration pass.

The retained hardware controller still reports `home=1 slewing=0 tracking=0`.
No controller handoff or guide pulse was sent during the merge.

The operator subsequently approved the guide-controller handoff and bounded
four-direction pilot while supervising the bare, clear mount, after merged-source
checks pass. Merged guide harness `ffd1f7d0a4ceffbf7854d6aa2fa1583fe0d7cb5a6fe12a6a5d9e5d2d98e36712`
passed the same fake rehearsal (cold refusal, home, GOTO, four one-second guides,
verified off after each, rehome): `upstream-697bceb5-guide-self-test.log`.
The existing matrix controller still owns the hardware connection.

## Resumed session — 2026-10-19 UTC

### Same-branch GOTO candidate — software verification in progress

#### Latest state: directional GOTO matrix and active Abort/rehome passed

Native pulse-guide implementation is now under software verification, not on
the retained hardware controller. It uses the existing I/O worker, fixed 50%
sidereal rates, signed direction from the northern measured frame, nearest
centisecond rounding (nonzero minimum one), and at most 255 centiseconds per
packet. IsPulseGuiding covers queued work and segment gaps; overlap and other
motion initiators are refused. Abort/disconnect cancel remaining packets and
retain the existing verified-stop policy. A lost guide ACK faults the session
without retry or reconnect; no coordinate compensation is applied. Initial
three-case fake-backed run passed 98 assertions (`native-guide-tests.log`);
the expanded timeout/concurrency and sanitizer results follow below. The
hardware remains on `0c0028aa` in `stationary-idle-fP3kj2sx`, home/tracking-off.

The expanded initial Debug selection passed 48 cases / 1,586 assertions
(`native-guide-full-debug.log`, before the final disconnect case and RA
displacement assertions). A separate UBSan disconnect regression then failed:
the 3.5-second takeover sub-budget could expire just before the second unchanged
encoder sample after a full 2.55-second native packet. The candidate now lets
takeover use its existing four-second I/O deadline, without removing guide-active
or settled-count checks. Red evidence: `native-guide-drain-regression.log`.
Final full Celestron-enabled Debug, focused UBSan and focused TSan runs completed;
no new binary has been handed the hardware connection.

Final Celestron-enabled Debug completed: **566/566**, 229.92 seconds
(`native-guide-final-full-debug.log`), including the formerly failing native
packet drain test. Focused UBSan passed **49 cases / 1,591 assertions** and
TSan passed **49 cases / 1,593 assertions**, with halt-on-error enabled and no
sanitizer finding (`native-guide-home-ubsan-tests.log`,
`native-guide-home-tsan-tests.log`). These are pre-merge results.

Guide harness `a2bb13a38d12aa4ab6a6d4a9db12e9db605f4527eca6cdf42b7ac56aa10955af`
passed its fake rehearsal: cold refusal, home, small GOTO, all four one-second
guide directions with verified tracking-off after each, and rehome
(`native-guide-controller-self-test.log`). It has not taken the mount connection;
handoff and bounded guiding require operator supervision and approval.

The guide polarity regression was mutation-verified with a separate linked test
binary: reversing the North/South signed-byte choice failed 15 assertions,
including both independent sky-Dec displacement checks
(`native-guide-sign-mutation-red.log`). Production sources/archives were not
mutated. Catch2 emitted raw uint8 character values in the failure output, so
the evidence reader needed byte-safe decoding; this was a log-reader issue,
not a compiler or driver failure.

The operator approved broader bounded same-branch testing. Harness candidate
`skyportal-driver-control-matrix`, SHA-256
`0c0028aaa275b093702d3d0233a59405c8dd95437e5bce28c760eac0b8816811`,
keeps the same production driver and adds fixed targets relative to that
controller's completed home: RA offsets -0.5/+0.5/0 degrees, Dec 88.5/88/89.
It refuses a cold target or a current position outside its near-home envelope,
uses the existing 20-second GOTO watchdog and automatic verified tracking-off,
and schedules Abort 1.5 seconds after the fourth GOTO request. The exact sequence
passed the actual-driver/fake rehearsal (`logs/matrix-controller-self-test.log`);
the fake's Abort leg holds completion busy so the check cannot pass as an idle
no-op (the initial instant-completion rehearsal correctly failed that check).

Following a verified stopped handoff, hardware session
`logs/stationary-idle-fP3kj2sx`, TCP `1.2.3.5:56342 -> 1.2.3.4:2000`, completed
home, the three targets in sequence, active Abort on a fourth target, verified
halt, then rehome. Requested/reported pairs:

| Target | Requested RA (h) | Reported RA (h) | Requested/reported Dec (degrees) |
| --- | --- | --- | --- |
| West/south | 13.568595227816 | 13.568635420998 | 88.5 / 88.500001430511 |
| East/south | 13.635261894483 | 13.635302668748 | 88 / 87.999994754791 |
| West/north | 13.601928561150 | 13.601969337246 | 89 / 89.000008106232 |

RA errors are about 2.170/2.202/2.202 arcsec; absolute Dec errors about
0.005/0.019/0.029 arcsec. All three used three RA SLOW rounds and one Dec round.
The fourth request targeted RA 13.568595227816 h / Dec 88 degrees; timed Abort
reported `motion_pending=1`, verified halt completed, and recovery home completed
in about nine seconds. Final status is `home=1 slewing=0 tracking=0`; session
retained, no further motion queued. This is bounded same-branch evidence only.

Capture `matrix-checkpoint.pcap` (local `/tmp/opencode/skyportal-direction-matrix.pcap`)
has 1,218 outgoing / 2,464 incoming checksum-valid AUX frames and matching
application request/echo/reply counts, plus stray `00 ae`. The harness source is
saved alongside it as `controller-source.cpp`. Guiding, manual motion, Park,
branch-changing routes and full ConformU remain unfinished/unqualified.

#### Previous state: updated normal-controller pilot passed; session retained stopped/off

All-vendor validation is now complete for this Debug source snapshot. The Pi
needed `libltdl-dev` and JPEG development headers; those build prerequisites
were installed, and the pinned libgphoto2 bundle was built into private
`AlpacaCore/build-gphoto-stage` using the repository script. No server deployment
or system libgphoto2 installation was performed. The combined HTTP/Core tree is
`AlpacaHTTP/build-all-vendors-validation`, all vendors ON, built with two jobs.
The registered-test floors passed. The full run passed 2,584/2,585 tests in
1,550.62 seconds; only `test_discovery` failed on a malformed probe. Host UDP
32227 was also held by the installed AlpacaBridge and Alpaca simulator.
Rerunning that unchanged test in a private network namespace with loopback up,
as the ordinary user, passed in 3.52 seconds. Thus every case has passed, with
discovery requiring isolation from the running responders; do not describe the
original shared-network run as wholly green. Logs: `all-vendors-build.log`,
`all-vendors-tests.log`, `all-vendors-discovery-isolated.log`, plus configure and
bundle logs. Final observed mount status remains stopped/tracking-off; disk has
7.5 GB free. ASan, complete preflight and WiFi ConformU remain outstanding.

The complementary vendors-OFF Debug build also passed: 274 Core + 21 HTTP
tests, 295/295 in 55.12 seconds, with the complete suite run in a private network
namespace to isolate discovery from host responders. Logs:
`no-vendors-configure.log`, `no-vendors-build.log`, `no-vendors-tests.log`.
The retained hardware session still reports stopped/tracking-off afterward.

After operator power-on approval, normal controller `2e78f84b` (including
early-tracking arrival and emergency zero-rate cleanup) opened
`logs/stationary-idle-u3SbjEYV`. Guarded home completed in about five seconds;
the same small GOTO requested RA 10.451783280199 h / Dec 88.5 degrees and
reported RA 10.451822756541 h / Dec 88.500001430511 degrees, approximately
2.132 arcsec RA / 0.005 arcsec Dec error. Automatic tracking-off verified
stopping. Final status remains `home=0 slewing=0 tracking=0`; connection retained.
Evidence: `normal-pilot-checkpoint.pcap`, also copied to
`/tmp/opencode/skyportal-emergency-rate-normal.pcap`. No fault was injected in
this normal-controller pilot. Before opening it, the old PID 1106120 was checked:
it holds only FIFO/log descriptors, no socket, and no mount TCP session existed.

#### Previous state: emergency writes observed on hardware; powered off and released

At the operator's request, old Pi CMake build outputs were cleaned: 3,586
generated ELF/archive files removed, about 8.3 GiB reclaimed. Root free space
rose from about 1 GB to 9.4 GB (67% used). Sources, logs/captures and the current
WiFi/ZWO build trees were retained. Removal manifest and linked-artifact hashes
are under `/home/pauleyj/alpaca-validation/cleanup-20261020T192653Z`.
This clears the disk-space blocker, not the ASan kernel limitation. Process
inspection also found old PID 1106120 (`build-geometry-check/skyportal-home-session`)
still present; it and its current-project files were excluded from cleanup.
The recent fault-test controller/relay had exited as recorded below; the older
process means the earlier broad claim of no controller process was too strong.

Post-pilot repository checks passed: `git diff --check`, clang-format over all
changed/untracked C++ files, layering, connect-error hooks, stress registration,
contract-sweep registration, changelog fragments and docs drift. Review of the
control-request and polling/motion fault-publication paths found no additional
confirmed defect in this pass. Full preflight/all-vendor and ASan were not run:
the local workstation is x86_64, and the arm64 Pi has about 1 GB free plus its
documented ASan address-space limitation. Final socket check shows no remaining
mount TCP connection. The external plan's top checkpoint now reflects the current
state instead of the superseded tracking-only checkpoint.

The operator approved a single lost-reply injection during brief tracking.
The loopback relay was rehearsed with the actual driver and fake mount first
(`logs/emergency-fault-rehearsal-lx87we4w`): it suppressed exactly one RA 0x13
reply, preserved echoes/other replies, forwarded both emergency motion stops
and zero rates, and retained the fault. Its framing self-check covers every
two-chunk split of a coalesced stream and the unarmed pass-through. An earlier
rehearsal checked for emergency sends before the in-flight timeout had expired;
the final rehearsal waits for that existing deadline rather than assuming the
harness's stale-snapshot alarm means cleanup has already run.

After verified stopped-controller handoff, hardware candidate
`skyportal-driver-control-emergency-fault`, SHA-256
`2494423a9b00e87532d7ca42ec219d76c78a7d05de070b8c2a552ebe64ebc91a`,
ran guarded home and tracking through the relay in
`logs/emergency-fault-hardware-MY7aqoeT`, mount TCP
`1.2.3.5:53542 -> 1.2.3.4:2000`. The relay dropped RA status at epoch
1792523402.588591; after the three-second transaction timeout it forwarded RA
motion stop at 1792523405.562767 and RA zero rate / Dec motion stop / Dec zero
rate at 1792523405.585582. The driver/harness reported fault and stop unverified,
without reconnecting or claiming idle success. Operator confirmed power cut;
controller and relay then exited. No live controller remains; the socket was
only in kernel FIN-WAIT-1 at the final check after peer power removal.

Final `traffic.pcap` is also `/tmp/opencode/skyportal-emergency-fault-final.pcap`.
Capture closed with 1,001 packets and zero kernel drops. The analyzer found
277 outgoing and 562 incoming checksum-valid AUX frames, matching application
request/echo/reply counts including emergency writes, with no stray spans in
this capture. Relay/controller/launcher/rehearsal sources are preserved alongside
the capture and journal. This establishes emergency command delivery after an
artificially lost reply on a working radio link; there are no post-fault encoder
samples proving standstill, and it does not establish delivery during total
WiFi loss. The four earlier GOTO passes and software gates remain separate
evidence. Hardware stays off pending the next approved test.

#### Previous state: emergency TCP stop also clears tracking; software gates passed

Rebuilt hardware candidate:
`source/AlpacaCore/build-debug/skyportal-driver-control-emergency-rate`, SHA-256
`2e78f84ba1a3e732dcb74beee7ed051b8bdc7f088e7214b09a942b32f1a3c3a1`.
The controller's fake-backed home/tracking/Abort/rehome/GOTO/automatic-off
rehearsal passed (`logs/emergency-rate-controller-self-test.log`). That rehearsal
does not itself inject a fault; the transport suite above covers the send-only
failure path. The existing live controller remains stopped/off and has not been
replaced. A hardware emergency-path test would require a separately approved
reply-suppression test while briefly tracking, preserving outbound stop delivery
and retaining the poisoned session until operator power-off. That is expanded
fault-injection scope, not another ordinary GOTO repeat; approval is pending.

Review of fault cleanup found `CelestronAuxLink::send_stop()` still sent only
`MC_MOVE_POS(0)`, unlike the corrected verified-stop path. TCP send-only cleanup
now also writes zero 24-bit guide rate (0x06), under the same lock and absolute
deadline. It remains poisoned/unhealthy and never claims physical stopping;
an unavailable radio link cannot guarantee delivery. Serial behavior is unchanged.
All three WiFi callers share the fix: acknowledged-stop fallback, control-request
failure and polling/motion failure cleanup.

The existing timeout regression failed before the fix because neither zero-rate
frame was sent (`logs/emergency-rate-red.log`). Timeout and report-overflow
cases now assert both axes' motion-stop and zero-rate frames. Partial-frame
coverage also truncates the zero-rate write and verifies closure prevents any
later stop from appending bytes. Debug/UBSan/TSan each passed 66 cases / 2,104
assertions, including transport and driver cases; full Debug passed 560/560 in
401.41 seconds (`logs/emergency-rate-*`). An initial fixture count adjustment
was reverted: idle/off report faults intentionally send no emergency stops.

The new emergency-path change is not deployed to hardware. Existing `721cb5a9`
session `stationary-idle-V6v0u2Fg` remains connected, stopped/tracking-off on the
final passive status check. No new hardware motion or WiFi interruption occurred.
Four bounded arrival passes below remain the latest motion evidence. Broader
motion/failure cleanup, unexplained WiFi dropout, corrupted spans, ASan, final
preflight/all-vendor checks and WiFi ConformU remain open.

#### Previous state: four consecutive early-tracking arrivals passed; session retained

Offline investigation of the prior `rPWZK7Bv` idle fault found a lower-layer
loss: the last complete RA encoder reply arrived at 13:53:22.497693 UTC;
the next RA slew-status request at 13:53:23.498447 was retransmitted at
23.712242 and 24.156247 without a captured reply. The journal records
`CTRL-EVENT-BEACON-LOSS` at 13:53:24.444545 and locally generated WiFi
disconnection at 24.595552, followed by failed authentication and
`ssid-not-found` at 13:53:39. This explains the transaction timeout without
implicating AUX reply parsing. The operator is not aware of any mount power-off
during that pause (09:53 local). Treat this as an unexplained WiFi dropout,
not a proven SkyPortal hardware failure. A read-only follow-up found no USB
reset/disconnect or undervoltage entry in the saved 13:52–13:53 journal window.
Current `V6v0u2Fg` has no beacon-loss/disconnection entry, reports a -42 dBm
signal and runtime WiFi power saving off, and remains stopped/tracking-off.
These current values do not establish the radio conditions at the earlier
failure. The saved profile uses default power-saving policy and autoconnect off;
no network setting was changed and no further motion was commanded.

The operator approved three further guarded rehome/same-small-GOTO repeats on
`stationary-idle-V6v0u2Fg`. All three completed home, full GOTO and verified
automatic tracking-off. Requested/reported RA pairs in hours:

| Repeat | Requested RA | Reported RA | RA error (arcsec) |
| --- | --- | --- | --- |
| 1 | 9.0481157137081 | 9.0481582124407 | 2.295 |
| 2 | 9.0533240434714 | 9.0533764096598 | 2.828 |
| 3 | 9.0588960007144 | 9.0589382306052 | 2.280 |

Each requested Dec 88.5 degrees and reported 88.500001430511 degrees
(0.005 arcsec error). Home took about eleven to twelve seconds per repeat.
The final passive status check reports `home=0 slewing=0 tracking=0`; no
further motion is queued and the connection remains retained.

Evidence: `arrival-repeats-checkpoint.pcap`, copied locally to
`/tmp/opencode/skyportal-early-tracking-repeats.pcap`. Across the first pilot
and these repeats it contains 2,555 outgoing and 5,165 incoming checksum-valid
AUX frames, matching application request/echo/reply counts, eleven RA SLOW
commands and four Dec SLOW commands. Additional stray spans `48 ff` and
`94 00 b2` occurred. Four passes qualify this bounded sequence only: the
2.828-arcsecond arrival still leaves a narrow margin, and intermittent traffic,
idle link loss, real failure-cleanup and broader motion qualification remain open.

#### Previous state: first early-tracking arrival pilot passed on hardware

After operator confirmation of power-on/clear bare mount, candidate `721cb5a9`
opened `logs/stationary-idle-V6v0u2Fg`, TCP
`1.2.3.5:42150 -> 1.2.3.4:2000`. Guarded FindHome completed in about twelve
seconds. The same small GOTO requested RA 9.000995010448 h / Dec 88.5 degrees
and reported RA 9.001034290654 h / Dec 88.500001430511 degrees: approximately
2.121 arcsec RA and 0.005 arcsec Dec error. Automatic tracking-off completed
with verified standstill; the session remains connected, stopped/off, not home.

`arrival-checkpoint.pcap` (local `/tmp/opencode/skyportal-early-tracking-pilot.pcap`)
contains three RA SLOW commands and one Dec SLOW command: the pass used the full
three-round budget, not a single-round arrival. After final tracking-off, all
ten encoder samples per axis over 6.5 seconds are identical (RA 2,583; Dec 336).
The analyzer found 518 outgoing and 1,053 incoming checksum-valid AUX frames,
matching application request/echo/reply counts, plus stray binary spans and the
adapter's ASCII association notice. Corrupted/non-AUX traffic remains unresolved.
This is one passing bounded hardware pilot, not arrival-repeatability or full
driver qualification. No further motion is queued.

#### Previous state: power cut confirmed; candidate ready for supervised pilot

On resuming, the retained `stationary-idle-rPWZK7Bv` controller was found
faulted during the pause: stale telemetry, unverified halt, then an RA 0x13
transaction timeout at local 2026-10-20 09:53:26.500. The preceding recorded
state was stopped/tracking-off; the fault does not prove new physical motion.
No new hardware commands were sent during preparation. The operator confirmed
power cut, and `power-off-confirmed` released the controller; its process exited
and no TCP session remains. Final capture/journal remain in the session directory;
capture closed with 57,309 packets and zero kernel drops, also copied locally to
`/tmp/opencode/skyportal-stop-rate-final.pcap`. Cause of the idle link loss has
not been established.

The arrival-timing candidate controller is
`source/AlpacaCore/build-debug/skyportal-driver-control-early-tracking`, SHA-256
`721cb5a9dea62433f36f1b84580bd71a86c802ef1d9361e42cd852e6e01bd4cc`.
Its fake-backed home/tracking/Abort/rehome/GOTO/automatic-off rehearsal passed
(`logs/arrival-early-tracking-controller-self-test.log`). Launcher selects that
binary and passes shell syntax checking. Hardware remains powered off pending
operator confirmation for guarded home and the same bounded small GOTO.

#### Previous state: arrival timing candidate passed software gates; paused at operator request

Increasing the existing WiFi-latency regression from 20 ms to 25 ms per reply
reproduced the final tracked-arrival failure before the fix
(`logs/arrival-early-tracking-red.log`). The candidate now resumes RA tracking
after SLOW completion, guide inactivity and report checks, before reading the
encoder pair. Arrival corrections therefore use measured tracked coordinates.
Before another correction round it suspends tracking again; FAST remains
untracked. The three-round limit, independent three-arcsecond gates, final
settling check and shared deadline remain unchanged. The lost tracking-off ACK
fixtures now target cleanup after the three tracked correction rounds.

Shared sampling callers were checked: normal status/control/takeover sampling
retains its original report-check ordering; only SLOW completion restores
tracking before the encoder reads. An initial full focused run caught changed
report-check ordering, which was corrected before the final runs.

Final Debug/UBSan/TSan each passed 43 cases / 1,478 assertions; full Debug
passed 560/560 in 400.11 seconds (`logs/arrival-early-tracking-*-tests.log`
and `logs/arrival-early-tracking-full-debug.log`). The operator requested a
stopping point after these tests. No candidate controller was deployed and no
additional hardware motion was commanded. Existing `67eef682` session
`stationary-idle-rPWZK7Bv` remains connected; the final passive log check reports
`home=0 slewing=0 tracking=0`. Changes remain uncommitted. Next step requires
approval for a controller handoff and a bounded hardware arrival pilot; this
candidate is software-verified only. ASan, final preflight/all-vendor checks and
WiFi ConformU remain outstanding.

#### Previous state: explicit-rate Abort verified on hardware

The next operator-approved guarded rehome/full Dec 88.5 GOTO passed on the same
connection. Home completed in about four seconds; requested RA
3.8740055776512 h, reached 3.8740593910217 h / Dec 88.500001430511 degrees,
approximately 2.906 arcsec RA and 0.005 arcsec Dec error. Automatic tracking-off
verified stopping. The retained session is now stopped/off after GOTO, not at
home. Capture: `goto-stop-rate-checkpoint.pcap`.

Offline comparison with the prior failed GOTO estimates the RA error at the
last pre-tracking encoder sample and final tracked sample using the last RA
SLOW target, its TCP dispatch timestamp and sidereal encoder drift. The failed
run is approximately 2.832 -> 3.160 arcsec; this passing run is approximately
2.555 -> 2.870 arcsec. The ~0.32 arcsec increase is consistent with the
tracking-restart latency. These are capture-derived estimates (dispatch is not
the driver's exact target-computation timestamp), not replacement coordinates
or an independent optical measurement. This identifies the small arrival margin
as a remaining timing problem; no tolerance change or speculative compensation
has been implemented.

After operator power-on confirmation (HC unplugged, SkyPortal on AUX1), the
first WiFi association attempt could not find the SSID. A later directed scan
found the same BSSID on channel 6 and association/ping succeeded; no controller
had been started during the failed attempt. Fixed controller `67eef682`
opened `logs/stationary-idle-rPWZK7Bv/`, TCP
`1.2.3.5:40316 -> 1.2.3.4:2000`. Guarded FindHome completed in about eleven
seconds. The approved two-second sidereal tracking interval followed by Abort
completed with verified stopped/off state. The session remains retained;
Abort invalidated the pointing reference, so AtHome is false.

The `stop-tracking-checkpoint.pcap` shows the exact stop sequence at
2026-10-20 12:54:34 UTC: RA motion stop at .301796, RA zero guide rate at
.328345, Dec motion stop at .350429 and Dec zero guide rate at .370287.
Every subsequent position pair in the checkpoint is identical: RA 16,762,505,
Dec 16,709,068, from 12:54:34.75 through 12:54:46.05. This is encoder evidence
that the explicit-rate stop arrested the RA drift seen in the earlier failed
cleanup, not merely successful ACKs or state flags. The checkpoint contains
392 outgoing and 792 incoming checksum-valid complete AUX frames, matching
application requests/echoes/replies, plus stray `62 ff` and `c4 ff`.

This qualifies one explicit Abort from tracking on this rig. Full GOTO-failure
cleanup with the fixed controller, GOTO arrival repeatability and the
intermittent home/traffic faults remain open. No further motion is queued.

#### Previous state: tracking-preserving stop reproduced and fixed in software

The fixed hardware controller has been rebuilt as
`source/AlpacaCore/build-debug/skyportal-driver-control-stop-rate`, SHA-256
`67eef682d4b20b46d1841ad489860a42829010da7dc702ea2463ec39014fb426`.
Its fake-backed rehearsal passed FindHome, two seconds of sidereal tracking,
explicit Abort with verified halt, rehome, full bounded GOTO and automatic
tracking-off (`logs/stop-tracking-controller-self-test.log`). The launcher now
selects this binary and passed shell syntax checking. No hardware connection
was opened during preparation; no TCP session remains. Next supervised pilot
is home/tracking/Abort only, with encoder/capture verification of standstill,
awaiting the operator's power-on confirmation.

Three more same-session guarded GOTO-Abort/rehome repetitions passed, each
rehome about eleven seconds, bringing the meaningful guarded recovery count to
four. The following full Dec 88.5 GOTO requested RA 20.141915070328 h and
failed its final tracked-arrival check. Cleanup then failed to verify stopping;
the operator cut power and confirmed it, observing little visible motion. The
faulted `KHpL8fFS` controller was explicitly released. No active controller
session remains; leave hardware off pending a separately approved pilot.

Final capture is retained as `logs/stationary-idle-KHpL8fFS/traffic.pcap`
(local `/tmp/opencode/skyportal-guard-final.pcap`). Sidereal rate was restored
at 2026-10-20 05:13:51.151943 UTC. After the arrival refusal, RA/Dec
`MC_MOVE_POS(0)` stops at 05:13:52.683342/52.703902 were acknowledged, but
RA position continued increasing from 2,517 at 05:13:53.111817 to 3,189 at
05:13:56.569648: approximately 15.01 arcsec/s. Dec remained at 69,766.
Both slew-done replies were 0xff and guiding replies zero throughout that
interval. Thus ACK + slew-done did not mean zero tracking; the encoder-based
settling refusal was correct. The stop deadline expired at 05:13:56.820.

The fake incorrectly cleared its tracking flag on `MC_MOVE_POS(0)`. Keeping
the tracking register alive reproduces the hardware failure in the existing
GOTO restoration-drift case (`logs/stop-tracking-red.log`): failed stop
verification and a latched link fault. The shared `stop_axes()` now sends
both the motion stop and explicit zero 24-bit guide rate (0x06) on each axis,
attempting every write even if another fails, then keeps the existing settled
verification. Sibling paths covered: Connect/disconnect takeover, explicit
Abort, home timeout/travel-limit cleanup and healthy-link GOTO failure cleanup.
Ambiguous transport failure still uses the bounded send-only escape and does
not claim verification; it may still require power-off.

Lost RA/Dec tracking-off ACK cases were added to the existing GOTO-failure
test. The Connect wire-order fixture now checks both explicit zero-rate writes.
Debug/UBSan/TSan each passed 43 cases / 1,478 assertions; full Debug passed
560/560 in 401.66 seconds (`logs/stop-tracking-*`). The stop-timeout message
now says both axes could not be verified stopped, rather than wrongly naming
FindHome for GOTO cleanup. No hardware pilot has used this fix. The final
GOTO arrival failure and intermittent home/traffic faults remain unresolved.

#### Previous state: guarded near-home FindHome passed after HC restoration

The operator restored physical home using the hand controller, unplugged it,
then confirmed a power cycle with SkyPortal remaining on AUX1. The old faulted
controller was released after that confirmation. The guard controller
`e4fcd4b9b0b794376add745b7918a1bebaf3e67ca6f7b2902404f3c536ae9225`
passed its fake-backed home-Abort/rehome check and opened
`logs/stationary-idle-KHpL8fFS/`, TCP `1.2.3.5:58816 -> 1.2.3.4:2000`.
One supervised FindHome completed in about four seconds, with home true,
slewing false and tracking off. The connection remains retained; no further
motion is queued.

The checkpoint has 152 outgoing and 318 incoming complete AUX frames with
valid checksums; it ends during an idle status exchange, so the last request is
not evidence of a lost reply. It still contains stray `2a 30 10 09 04 00 ae`.
Across the captured encoder samples, cumulative absolute wrapped travel was
about 0.395 degrees RA and 0.334 degrees Dec; initial labels were RA 1 / Dec 0,
final labels RA 1,172 / Dec 16,777,024. A measured home therefore still cannot
be replaced by assuming zero labels. This establishes successful near-home
operation with the guard, not hardware exercise of its limit-trip path or a
fix for corrupted traffic or intermittent failed seeks.

An approved follow-up requested `home-abort` then `home` on this same session.
Because `AtHome` was already true, FindHome returned idempotently before the
500-ms timed halt (`home_pending=0`); the next FindHome was also a no-op. The
observer printed a pass based on completion markers, but this is **not** evidence
of aborting a live seek or recovering from one. The mount remains stopped at
home. A meaningful recovery trial must first leave the established home state
under separately approved bounded motion; do not count these markers as a
hardware recovery pass.

The operator then explicitly approved bounded Dec 88.5 GOTO, halt after about
0.5 seconds, verified stop, then guarded FindHome. This exercised real motion:
GOTO requested RA 20.095612147109 h / Dec 88.5 and published `home=0 slewing=1`;
Abort completed with stopped/off state. Recovery FindHome began at
2026-10-20 05:11:03.888443 UTC and completed in about ten seconds. Its fresh
pre-command labels were RA 16,732,001 / Dec 23,101; final labels were
RA 16,777,143 / Dec 16,777,048. Observed accumulated wrapped travel was
5.719 degrees RA and 2.017 degrees Dec, below the guard on both axes.

`guard-recovery-checkpoint.pcap` contains 3,412 outgoing and 6,852 incoming
complete checksum-valid frames, with all application request/echo/reply counts
matching. The sole stray span is the one already present in the initial home
checkpoint; no additional stray span appeared in this recovery trial. This is
one successful guarded GOTO-Abort/rehome observation, not limit-trip hardware
qualification or proof the intermittent missed-home/corruption faults are fixed.
The same `KHpL8fFS` session remains connected at home, tracking off.

#### Previous state: reduced-load home trial failed with excessive Dec travel

Software follow-up adds a conservative near-home seek guard to the restored
250-ms polling baseline: each FindHome reads fresh encoder positions before
sending either home command, resets per-axis travel counters, and accumulates
absolute wrapped deltas on every homing sample. At 15 degrees observed travel
on either axis, the driver invalidates the reference, retains an axis-specific
error, and queues the existing acknowledged/settled two-axis stop path. A
reversal cannot subtract from travel, and crossing count zero is not a full turn.
This is a driver-imposed restriction for the incomplete near-home WiFi profile,
not a Celestron specification or an absolute physical stopping distance: sampling,
I/O and deceleration add travel before standstill. The 60-second bound remains.

The fake CGX profile now starts at its default home labels rather than the generic
transport fixture's arbitrary 0x123456, so normal tests no longer teleport about
25.6 degrees during a simulated near-home seek. The regression deliberately
scripts out-of-bound movement on each axis, negative travel across wrap and
reversal; it checks below-limit continuation, both stop commands, retained errors,
loss of reference, explicit Abort clearance and a fresh bounded home. It fails
when the guard is raised to 180 degrees (production mutation at the
`home_travel[i] >= 16777216ULL / 24` condition in the driver).
Debug/UBSan/TSan each passed 43 cases / 1,456 assertions; full Debug passed
560/560 in 393.00 seconds. Evidence: `logs/home-travel-*`. The first focused
run corrected the test's expected coordinate-read error to the existing retained
`DriverException` before Abort, rather than `InvalidOperation`; the production
failure contract was not changed for the assertion.

No hardware test or controller handoff has used this guard. The retained
`6LugpHJv` session still runs the failed pacing experiment at verified stop.
The underlying missed-home and corrupted-traffic mechanisms remain unresolved;
the guard addresses excessive travel only.

The operator approved a software-tested reduced-load home-poll comparison and
stationary handoff on AUX1. The candidate retained the 250 ms worker tick but
left 500 ms after each homing sample before the next, preserving stop handling
and the existing 60-second home deadline. Its virtual-clock regression failed
on the preceding code and passed on the candidate; Debug/UBSan/TSan each passed
43 cases / 1,229 assertions and full Debug passed 560/560 (436.72 seconds).
Two existing virtual-time home fixtures needed enough ticks for the slower
polling; the first combined Debug attempt exposed the second fixture before
its correction. Logs: `logs/home-quiet-*`.

Controller SHA-256
`55e17ce451e038ddc37d9de5bc66fa17a82152774bd73f34dd77deea2ce8c7b3`
passed fake home-Abort/rehome/GOTO/automatic-off checks. The approved handoff
closed `n5JTiPLN` while stationary and opened `logs/stationary-idle-6LugpHJv/`,
TCP `1.2.3.5:44944 -> 1.2.3.4:2000`. The first timed home Abort verified
stopping. Recovery FindHome then ran to its 60-second deadline: RA reported
home, Dec did not. Cleanup and explicit Abort verified stopping with tracking
off; the remaining two trials were cancelled. The operator observed the large
travel. The failed controller remains retained at verified stop, not at home;
no more motion is authorized by the completed comparison.

The checkpoint has 908 outgoing and 1,829 incoming complete AUX frames, all
checksums valid with matching application request/echo/reply counts. Stray
binary spans remain: `82 df 1a 10 30 09 b4`, `02 b0`, `8a 09 00 00 b0`.
Recovery Dec home was sent at 2026-10-20 04:12:58.760173 UTC; all 90 subsequent
home-status replies were zero. The encoder moved from 16,676,127 at
04:12:59.144112 to a final settled 8,992,735: about -164.868 degrees net,
with a reversal near 189 degrees before the final 192.963-degree encoder label.
The timeout stop writes occurred at 04:13:59.006124/04:13:59.035021 UTC;
the retained `STOP_VERIFIED` followed the explicit Abort cleanup.

This is an unsuccessful home seek with working application replies, not a
communication timeout. It does not prove slower polling caused it, but slower
polling did not eliminate corruption or qualify recovery. The experimental
source/test changes were reverted locally; their exact sources are preserved
as `logs/home-quiet-candidate.cpp` and `logs/home-quiet-candidate-tests.cpp`.
The 60-second-only home bound permits excessive travel and must be strengthened
before further home trials. A tighter travel guard needs to handle encoder wrap,
direction reversals, initial unknown position and abort/rehome ownership; do not
silently assume the last physical home survives an Abort or power cycle.

#### Previous state: AUX1 comparison passed motion checks, stray bytes remain

Offline follow-up: the transport suite `[celestron][aux][tcp]` passed 20 cases /
496 assertions on the existing native Debug build (`logs/aux-parser-offline-review.log`).
Inspection of `read_frame_locked()` and the failed session's independently
reassembled TCP stream found no missing reply for the parser to recover: the
Dec home-status echo/reply never arrived in that capture. The parser skips
non-frame prefixes, validates complete frames through the decoder and retains
one absolute exchange deadline. This does not prove every parser edge case.

The pinned INDIGO reference's `nexstar_aux_commands_10.pdf`, chapter 4,
explicitly warns that back-to-back slew-completion queries can make a motor
controller miss its destination and continue rotating. The document does not
specify a 500 ms minimum; INDIGO's implementation chooses 500 ms. Our 20 ms
SLOW completion wait and 250 ms home-worker tick therefore remain a pacing
qualification question, despite the successful bounded runs. The missing
home reply is not proof of that mechanism, and the earlier spaced-query trials
had different WiFi/failure conditions. A reduced-load home-poll comparison on
the current hardware is the next discriminating experiment; it must preserve
stop responsiveness, settled-reference checks and absolute deadlines. Do not
extend the GOTO cadence blindly: at 500 ms the stopped RA loses about 7.5 arcsec,
already beyond the unchanged three-arcsecond arrival gate. No retry, timeout
extension or runtime code change was made during this review.

The operator identified the original connection as SkyPortal plugged directly
into AUX2, with only mount power otherwise attached. At the requested power-off
handoff they moved it directly to AUX1 and powered on. The old `mZBDjDpQ`
session faulted during that deliberate power cycle and was released afterwards;
that shutdown fault is not an independent reliability incident.

The unchanged `af5cbf6d` controller opened `logs/stationary-idle-n5JTiPLN/`,
TCP `1.2.3.5:35494 -> 1.2.3.4:2000`. A 500-ms homing Abort followed by
rehome passed, then all three bounded GOTO/Abort/rehome repetitions passed.
The session remains connected at home, tracking off, with no motion queued.

`aux1-recovery-checkpoint.pcap` contains 1,050 outgoing and 2,160 incoming
complete AUX frames, all checksums valid and matching application
request/echo/reply counts. It nevertheless contains binary stray bytes `00 ae`
at 2026-10-20 03:37:45.639747 UTC, between a Dec home-status echo and reply;
100 ms later an intact `3b 05 30 10 09 04 00 ae` switch message arrives.
An ASCII association notice for the Pi's wlan1 MAC also appears at
03:37:33.308608, near session startup, and is separate from the binary stray
bytes. Moving from AUX2 to AUX1 did not eliminate the phenomenon. These short
runs neither isolate the adapter versus the mount nor establish relative port
reliability. No pacing or retry change has been made.

#### Previous AUX2 session: fresh-session home Abort/rehome passed

The operator confirmed that only power and SkyPortal are attached, then powered
on and explicitly approved FindHome, an intentional halt during homing, and
recovery FindHome after verified stopping. The unchanged `af5cbf6d` controller
opened `logs/stationary-idle-mZBDjDpQ/`, TCP
`1.2.3.5:37734 -> 1.2.3.4:2000`. The timed halt was requested 500 ms into
FindHome; verified completion followed about 3.2 seconds later. Immediate
recovery FindHome completed in about nine seconds. Idle samples report
`AtHome=true`, `Slewing=false`, `Tracking=false`; the connection remains retained.

The `home-recovery-checkpoint.pcap` contains 320 outgoing and 660 incoming AUX
frames, all checksums valid, no stray bytes, and matching request/echo/reply
counts. This is one successful home-Abort/rehome sequence after a power cycle,
not a reproduction or resolution of the prior GOTO-Abort/rehome failure.

The operator then approved three same-session bounded GOTO/Abort/rehome
repetitions. All three verified stopping and completed recovery FindHome;
the connection remains retained at home with tracking off. The subsequent
`abort-rehome-checkpoint.pcap` contains 2,122 outgoing and 4,300 incoming
complete AUX frames, all checksums valid and matching application
request/echo/reply counts. However, it also contains stray bytes
`82 09 04 00 ae`: successful recovery did not eliminate the corrupted bus
traffic. No firmware, pacing, retry or driver change separates these runs
from the failed recovery. Completed post-Abort GOTO remains untested in this
fresh session.

Offline inspection of that prior failure places the stray span
`db 2a 30 10 09 04 00 ae` at 03:20:17.093025 UTC, followed at
03:20:17.196028 by the intact switch message `3b 05 30 10 09 04 00 ae`
and its motor acknowledgement. The shared suffix suggests corrupted switch
traffic, but neither proves bus contention nor identifies the faulty component.

#### Previous session: intentional Abort passed; recovery home lost a reply

The operator explicitly approved a same-session home/GOTO, intentional halt
about 0.5 seconds after GOTO initiation, then rehome/GOTO recovery. The halt
completed with `Slewing=false`, `Tracking=false` and `HALT_COMPLETE`. During
the following FindHome, the Dec `MC_LEVEL_DONE` request timed out. The controller
attempted halt but could not verify it; the operator cut power and confirmed it.
The faulted `P404Pt9H` controller was then explicitly released. The mount is
powered off; no controller session remains. The recovery GOTO was never sent.

Final capture: 21,144 packets, zero kernel drops, 7,104 outgoing AUX frames and
14,270 incoming complete frames, all complete-frame checksums valid, plus
non-AUX spans `08 ae`, `94 04 00 ae`, `db 2a 30 10 09 04 00 ae`.
At 2026-10-20 03:20:17.076284 UTC the Pi sent Dec `MC_LEVEL_DONE`; the adapter's
next TCP segment acknowledged all six request bytes but carried unrelated
non-AUX bytes instead. The request's AUX echo and reply are both absent from
the reassembled capture (122 requests, 121 echoes and 121 replies for that
address/command). Other bus traffic arrived during the 2.833-second remaining
poll budget. Both send-only stops at 03:20:19.910/19.916 were echoed and ACKed
by 03:20:19.954, but there were no post-stop position/status reads to prove
stopping after the stream was poisoned. The earlier power-cut request was
therefore appropriate despite those ACKs.

No deauthentication was logged at the failure; beacon loss was recorded later,
at 03:21:49 after the operator's power cut. This incident is missing AUX
forwarding/reply delivery on an active TCP connection, not the earlier
reason-7 association failure. It does not identify the failing adapter, bus
or controller mechanism. Four bounded near-home GOTOs and the intentional
GOTO abort passed, but same-session abort/rehome recovery and broader link
reliability remain unqualified. No automatic reconnect or retry was attempted.

#### Busy-tail candidate: first bounded hardware GOTO pass

Following explicit approval of the stationary handoff, the verified-stop
`QgC24qlH` session was closed and controller `af5cbf6d` opened
`logs/stationary-idle-P404Pt9H/` (PID 1227999, TCP
`1.2.3.5:42884 -> 1.2.3.4:2000`). FindHome completed in about eleven seconds.
The bounded GOTO requested RA 17.961082050111 h / Dec 88.5 degrees and completed
at RA 17.961134843683 h / Dec 88.500001430511 degrees: approximately
2.851 arcsec unprojected RA error and 0.00515 arcsec Dec error. SlewSettleTime
and final tracked-arrival verification completed, then the controller switched
tracking off and verified stopping. Subsequent idle samples report
`Slewing=false`, `Tracking=false`, `AtHome=false`.

The session and continuous capture remain retained, with no further motion
queued. Checkpoint `goto-checkpoint.pcap` contains 695 outgoing AUX frames and
1,402 incoming frames (echoes/replies and other bus traffic), all complete-frame
checksums valid and no non-AUX byte spans. It shows one FAST command per axis,
three SLOW commands on RA and one on Dec. This is one successful near-home
same-branch encoder/control observation on the tested northern CGX, not optical
accuracy, repeatability, meridian routing, broader WiFi reliability or complete
driver qualification. The RA result is close to the unchanged three-arcsecond
limit; repeat and abort/recovery qualification remain open.

The operator confirmed normal visible movement/stopping and approved three
additional same-session home/GOTO/automatic-off repetitions. All three passed:
RA targets/readbacks were 18.026434581572 / 18.026484207095,
18.031685193442 / 18.031738827067 and 18.03699209538 / 18.037042391673 hours.
Dec readbacks were 88.500001430511, 88.500022888184 and 88.500022888184 degrees
against 88.5. Across all four successes, unprojected RA errors were approximately
2.68–2.90 arcsec and Dec errors 0.005–0.082 arcsec. Each repetition explicitly
re-established home, completed the one-second settle with tracking restored,
then verified tracking-off before the next request. The same session remains
retained and stationary. This supports repeatability only for this bounded
near-home route; intentional GOTO Abort/recovery still needs qualification.

#### Resumed supervised timing pilot: verified stop, arrival still fails

After returning, the operator explicitly approved a fresh connection, FindHome
and the bounded Dec 88.5 GOTO. The rebuilt actual-driver controller
(`528376e9ab5c032e373565f6a7a5fbf44b01079ec09ac847fc5ae205658d900e`)
passed its fake-backed home/GOTO/automatic-off test. The Pi had lost its WiFi
association; explicit profile activation restored wlan1 routing and 5/5 ping
replies before opening the driver session.

Previous session (closed during the approved handoff above): `logs/stationary-idle-QgC24qlH/`, TCP
`1.2.3.5:38692 -> 1.2.3.4:2000`. FindHome completed in about eleven seconds.
GOTO requested RA 17.361882100875 h / Dec 88.5 and again exhausted three SLOW
rounds. The new healthy-link cleanup verified stopping before exposing the
retained original error. The controller's explicit Abort then also completed
with `Slewing=false`, `Tracking=false`, `HALT_COMPLETE` and `STOP_VERIFIED`.
The connection and continuous capture remain retained; no further motion is
queued. This is a passing bounded hardware stop-recovery observation, not a
passing GOTO or broader reliability qualification.

Checkpoint capture: 589 outgoing frames and 1,195 incoming frames, all complete
frame checksums valid. Three incoming non-AUX byte spans were also present
(`62 06 10 30 09 b4`, twice `62 6f 1a 10 30 09 b4`); the earlier corruption
question remains open. The last two RA SLOW commands were 16775817 at
2026-10-20 02:27:30.057367 UTC and 16775871 at 02:27:30.337526 UTC. Their
post-completion samples were 16775815 at 02:27:30.337284 and 16775871 at
02:27:30.617238: about 280 ms per refinement, still roughly 4.2 arcseconds of
sidereal drift while tracking is off. Each first status read said busy even
though the position near the end of that sample had reached the requested
counts; a second six-exchange sample then consumed another ~120 ms. Reducing
the inter-poll sleep removed much of the old 575 ms delay but did not remove
this remaining observation latency. The fake's instantaneous completion on its
first status query misses that busy-to-complete sequence; reproduce it before
the next correction-scheduling change. Do not widen the arrival tolerance.

The busy-tail regression now models one busy status read before completion,
with positions already at the commanded target, plus 20 ms processing delay
per request. It reproduced the same exhausted-corrections failure against the
hardware-tested timing candidate. The revised worker polls only the commanded
axes' completion status on the short path; full guiding/position telemetry is
sampled at completion or roughly every 250 ms during longer motion for progress
and guide checks. Non-commanded axes were stopped in the preceding completed
leg; the final all-axis sample after tracking restoration still verifies both
before reporting successful completion.

This candidate passed the isolated GOTO suite (5 cases / 111 assertions), then
native Debug, UBSan and TSan (42 cases / 1,192 assertions each), and the full
Debug suite (559/559, 393.15 seconds). Logs are `logs/goto-busy-*`.
The new controller `source/AlpacaCore/build-debug/skyportal-driver-control-busy`
has SHA-256 `af5cbf6d0d266becb1e34ec57557d16c889ce966a5af45967d830aebe328b238`
and passed its fake-backed home/GOTO/automatic-off test. Hardware testing awaits
operator approval of the stationary handoff from the retained verified-stop
session; no replacement connection or additional motion has been requested.

#### First hardware GOTO: encoder arrival, failed sky-time refinement

After a fresh WiFi association, the approved network-only check in
`logs/network-only-Zsih8A1w/` returned 60/60 ICMP replies with zero loss
(1.211/2.631/17.448 ms minimum/average/maximum), power saving off, and no logged
deauthentication. Capture used `tcpdump -p`. The operator then confirmed the
mount was clear and ready for tests.

The new actual-driver session `logs/stationary-idle-6H1U49Gd/` connected and
FindHome completed in about five seconds. The bounded pilot targeted
RA 15.068080342095 h, Dec 88.5 degrees. FAST approach and three SLOW rounds ran,
with the last two correcting RA only. The driver refused completion with
`slew exhausted three slow-approach/correction rounds`; its fault handler sent
both stops, and the controller reported an unverified halt. The operator cut
power and the faulted controller was then explicitly released. No session is
retained; the mount remains off.

Final capture: 913 packets, zero kernel drops, 304 outgoing AUX frames and 628
incoming frames (including echoes and other bus traffic), all complete-frame
checksums valid with no stray bytes. No deauthentication was logged. Each SLOW
target was attained in encoder readback: RA 16772752, 16773263 and 16773393;
Dec reached 69120 on its sole SLOW leg. Final RA dispatch was at
2026-10-20 00:10:16.026598 UTC; its refreshed position reply matched 16773393
at 00:10:16.601256 UTC. The approximately 575 ms elapsed corresponds to about
8.6 arcseconds of sidereal motion while tracking is off, already beyond the
three-arcsecond sky-coordinate gate despite reaching the commanded encoder
position. The 250 ms wait and repeated six-exchange samples contribute to this
latency. This is evidence to repair the correction/observation schedule, not
permission to widen tolerance or substitute synthetic coordinates.

A second software issue is exposed by this failure path: the outer worker
catch always calls `send_stop`, which deliberately poisons the stream even for
a semantic arrival failure on an otherwise healthy connection. The resulting
halt cannot verify stopping. Healthy-link failures need bounded acknowledged
stops and real stopped/settled evidence while retaining the original operation
failure; ambiguous transport failures still require the send-only escape.
Both fixes require fake-backed regressions before another approved hardware run.

Stop-cleanup follow-up is implemented but not yet hardware-qualified. A typed
GOTO failure distinguishes driver-detected stall, overall timeout, excessive
arrival error, exhausted correction rounds and post-restoration drift from
transport/controller-report failures. While the link remains healthy, those
failures hand off to the existing StopRequested/Stopping owner, preserving the
message in `motion_error_`, invalidating the reference and requiring the usual
three stable comparisons before completing cleanup. `Slewing` stays true during
cleanup and then reports the retained original error. Explicit Abort can clear
that error through another verified stop without reconnecting. A failed cleanup
still takes the existing bounded send-only/latched-fault path.

The strengthened existing GOTO failure case first failed on all three healthy
semantic failure sections against the old code, then the focused GOTO run passed
4 cases / 88 assertions with the fix. It requires at least four position pairs
after the last stop, retained failure, tracking off and explicit-Abort recovery
on the same connection. Two further sections cover lost RA/Dec cleanup ACKs.
The stop-only focused Debug run passed 41 cases / 1,185 assertions. UBSan
failed the stalled-axis section's `REQUIRE(failed)` after its 16-second test
wait; this interrupted that batch before TSan or full Debug ran. Repeating
that exact section under UBSan with the same seed passed 13 assertions, with
no code or assertion change. The intermittent observation deadline failure
remains recorded rather than counted as a clean gate. The previous hardware
GOTO is still a failure.

Timing follow-up candidate: status/guide flags are now sampled before encoder
positions, with RA last and timestamped on receipt. A completed GOTO therefore
needs no second six-exchange sample before testing arrival. SLOW rounds use a
20 ms wait between complete polls (FAST remains 250 ms); each poll still pays
its actual six exchanges. After verified GOTO arrival, tracking restoration
omits the redundant Dec zero-rate write: that rate was zeroed before the slew
and Dec completion and guide inactivity were just read. Ordinary Tracking and
Sync retain their two-axis write sequence. The three-arcsecond coordinate gate,
three-round budget and final tracked-arrival check remain required.

A new test injects 20 ms latency before processing every fake request. It
failed on the prior implementation with post-restoration arrival drift, then
the isolated candidate GOTO suite passed 5 cases / 111 assertions, including
the healthy-stop and lost-cleanup-ACK sections. The isolated driver-file suite
then passed 39 cases / 1,180 assertions; the delayed-reply case also passed ten
consecutive repeats while the native combined build was running. This fake still completes GOTO
on its next status query: it exposes network scheduling latency, not physical
motor dynamics. The candidate was linked separately in tmpfs so the ongoing
stop-only validation source/build remained intact. The combined timing/stop
candidate is synced to the native source tree. Sequential focused Debug,
UBSan and TSan each passed 42 cases / 1,192 assertions, including lifecycle
stress; the full Debug suite passed 559/559 in 391.58 seconds. Evidence is under
`logs/goto-timing-*`. These runs include the previously intermittent stalled-axis
section; its earlier failure remains recorded above. A new approved hardware
pilot remains pending.

The operator subsequently powered the mount on. No driver connection or motion
has been requested since the failed pilot; it remains outside software tests.
The operator then stepped out and explicitly prohibited hardware testing until
returning. Hardware testing is paused; only software work is authorized meanwhile.

The retained tracking controller faulted at log wall time 19:05:54 (23:05:54 UTC
in the packet capture) on an RA slew-status
request timeout. Its last successful status was tracking off and not slewing;
its subsequent halt was unverified. The operator reported the mount appeared
at home and confirmed a power cycle. That appearance does not establish a
reference: the next hardware session requires a fresh connection and completed
FindHome. The old session log and capture are retained as failure evidence.

After the operator's confirmed power cycle, the faulted controller was explicitly
released to finalize its capture, without reconnecting. Strict TCP reassembly of
the final capture found 2,842 outgoing AUX frames and 5,692 incoming frames
(echoes, replies and other bus traffic), all complete-frame checksums valid,
with one stray incoming `04` byte. The final RA `MC_SLEW_DONE` request was sent
at 23:05:53.622908 UTC and its echo plus valid `ff` reply arrived at
23:05:54.627910 UTC: 1.005002 seconds later. This explains that timeout against
the controller harness's explicit 1,000 ms override; it does not identify the
cause of the delay or resolve earlier association loss/corruption.

The controller override has been removed. The next pilot uses the driver's
normal configured 5-second response timeout, still limited by the existing
per-operation budget (the slew/idle sampling stages allow at most 3 seconds).
No production deadline, frame validation or motion retry policy was changed.
The rebuilt controller passed its fake-backed home/GOTO/automatic-off check;
SHA-256 `07074cc9ec44c1421660364210760d167fb20d06c55e63ec042bed8fa4519f92`.

The initial GOTO-only native run passed 4 cases / 53 assertions. That run
predated the final post-restoration arrival check and active-GOTO stress case;
the updated Debug, UBSan and TSan filters each passed 41 cases / 1,139 assertions.
TSan's archive build failed when the Pi root filesystem filled, before running
tests. Completed generated mutation objects/executables were moved to tmpfs
(source, logs and captures retained), restoring about 406 MiB; the retried TSan
build and focused run passed with `halt_on_error=1`.

The full Debug suite passed 557/558 in 400.86 seconds. The unchanged direct-USB
case `explicit stop displaces a draining guide handoff` failed the 100 ms
displaced-initiator/zero-pulse readiness checks and dependent pending-state
check. It then passed ten consecutive focused repeats in 18.26 seconds without
code or assertion changes. That is evidence of intermittency, not a proven
cause; the full-suite repeat passed 558/558 in 369.66 seconds and the original
failure remains recorded.
Formatting, stress registration, documentation drift, layering and changelog
checks pass. All five GOTO mutations failed their intended tests: tracking
admission, selective correction, settling duration, post-restoration arrival
and stalled-axis detection. The selective-correction mutant initially failed
compilation on an unused parameter; it was corrected and observed failing at
runtime. The stall mutant initially ran out of disk during linking; it was
relinked in tmpfs and observed failing at runtime. Neither infrastructure
failure was counted as a killed mutation.

The retained controller's new `goto-small` command has passed a fake-backed
home/GOTO/automatic-tracking-off/close self-test. It accepts only a completed
northern FindHome with tracking off, targets the current RA and Dec 88.5,
uses one second of SlewSettleTime and a 20-second harness watchdog, and turns
tracking off after verified GOTO completion. This is preparation for a bounded
pilot, not hardware evidence; the candidate binary is
`source/AlpacaCore/build-debug/skyportal-driver-control-goto` on the Pi.

The operator explicitly approved a fresh session, FindHome and one bounded
same-branch GOTO to Dec 88.5 with automatic tracking-off, conditional on the
remaining software checks passing, and confirmed bare-mount supervision and
clear cables. The launcher selects the default-timeout GOTO binary; route
inspection still selects `wlan1` and there is no established old AUX connection.
After those checks passed, the approved fresh session was attempted; its failure
is recorded below.

#### Approved pilot blocked before AUX traffic

Session `logs/stationary-idle-bu9GTmW5/` began at 23:37:46 UTC with the
default-timeout GOTO controller. Connect returned `Celestron AUX: AUX transaction
timed out`; no READY, FindHome or GOTO was issued. The operator confirmed no
movement and powered the mount off, after which the retained faulted controller
was explicitly released and the capture finalized.

The capture contains eight packets and zero kernel drops: an unanswered ARP
request for 1.2.3.4, a DHCP reply and six SkyPortal discovery broadcasts. There
is no TCP payload and no captured AUX command. At 23:37:46.188267 UTC, immediately
after the ARP request, the kernel records wlan1 deauthentication from SkyPortal
with reason 7 (`CLASS3_FRAME_FROM_NONASSOC_STA`). Reassociation and DHCP completed
around 23:37:47.36 UTC, but this connection attempt did not recover. This is a
WiFi/connection-establishment failure before motor-controller protocol exchange,
not evidence of a rejected GOTO or bad AUX checksum. The reason-7 mechanism
remains unresolved. The later beacon loss follows the operator's power-off.

Hardware GOTO qualification remains blocked. The mount is powered off; there
is no retained controller session and no automatic reconnect or motion retry.

Offline follow-up: `iw dev wlan1 get power_save` reports off; the saved profile
uses the default power-save setting, so this read does not prove its state at
every earlier failure. Kernel history also records reason-7 drops at 23:12:17,
23:12:50, 23:17:31 and 23:18:22 UTC. At the failed pilot, capture startup enabled
promiscuous mode at 23:37:46.116239, 72 ms before deauthentication. That correlation
does not establish a cause, and it cannot explain all earlier drops. The launcher
now uses `tcpdump -p` to avoid this unnecessary adapter mode change while still
capturing the Pi's own mount traffic. Shell syntax validation passed; the change
is not yet hardware-validated. Next step is an explicitly approved network-only
check after power-on, with a fresh WiFi association and no driver/motor commands.

The uncommitted driver now queues SlewToCoordinatesAsync/SlewToTargetAsync on
the same I/O worker. Admission requires healthy measured references and Tracking
on, chooses the destination branch once, and refuses both a branch-changing target
and a one-degree approach that would cross that branch. Approach direction comes
from each controller's captured MC_GET_APPROACH byte. Synchronous/AltAz slews,
forced pier side, meridian routes and Park remain unsupported.

The worker turns tracking off, sends FAST approach positions, then performs the
mandatory SLOW approach and up to three total SLOW/arrival-correction rounds.
Later rounds correct only axes outside three arcseconds, using wrapped,
unprojected RA error and independent Dec error; errors above one degree fail.
Per-axis no-progress detection is ten seconds while that axis reports busy, and
one 180-second deadline covers dispatch, approach, correction, restoration and
SlewSettleTime. Arrival is checked again after restoration/settling. These
thresholds and the 250 ms polling cadence are candidates, not WiFi hardware
qualification. Abort/disconnect cancel the owner; faults remain visible through
Slewing and use bounded stop attempts without automatic reconnect.

The fake has scripted immediate GOTO completion, stuck-axis mode, per-axis lost
ACKs and SLOW offsets; it models no motor dynamics. New tests cover refusal
before dispatch, selective RA correction near the pole (no cos-Dec projection),
settling, abort before slow approach, exhausted corrections, stalls and drift
introduced by tracking restoration. A new lifecycle stress case starts with
both actual fake GOTO commands already dispatched before racing teardown.
The initial focused native run passed as recorded above; updated and broader
gates remain open.

The faulted tracking-only controller in `logs/stationary-idle-7rYbx035/` has
been released after the confirmed power cycle; there is no replacement session.
No hardware GOTO has been commanded.

### Approved tracking and tracking-enabled Sync checks

After software gates passed, the retained controller extension passed fake-backed
ten-second tracking/automatic-off and tracking-enabled Sync tests. The operator
confirmed bare/clear supervision and explicitly approved another stationary
handoff, FindHome and bounded tracking checks. Three stationary checks preceded
the old controller's verified disconnect. The new retained session is
`logs/stationary-idle-7rYbx035/`, PID 1162437, TCP
`1.2.3.5:52514 -> 1.2.3.4:2000`. Controller binary SHA-256:
`d4085aaf742a85c85a2e5b1598fe7958d6bb7be434514ae050de942cd4d4a9d6`.

FindHome completed in about four seconds. The first ten-second tracking run
reported Tracking=true, Slewing=false and Dec=90 throughout, then verified
Tracking=false/stationary. After its initial sample, the reported RA spread was
about 0.16 arcsec. A second bounded run suspended tracking for Sync, requested
RA 13.867379489346 h / Dec 89.99 degrees, and completed in 1629 ms with target
properties matched and readback RA 13.867544479854 / Dec 89.990000724792.
Tracking resumed, then the controller automatically switched it off and verified
stopping. The operator confirmed normal behavior and a stationary mount. The
Sync readback RA differs from the requested label by about 8.9 arcsec at return;
the off/settle interval advances sky coordinates, and these checks are not
precision optical or ConformU Sync qualification.

`tracking-sync-checkpoint.pcap` reassembles without gaps: all 668 requests have
matching echoes/replies; six tracking-register writes and one label write per
axis. Every complete decoded frame is checksum-valid. One stray incoming `04`
byte remains; this is not corruption-free traffic. The capture checkpoint reports
zero kernel drops (capture continues). Independent raw-encoder slopes, using
reply capture times and excluding the first second after each enable, were:
1534 counts/7.882144 s = 15.033707 arcsec/s; 216/1.120009 = 14.897636; and
653/3.361500 = 15.006009 after restoration. These short intervals include wire
timing/encoder quantization and are bounded direction/rate evidence, not a tuning
rule or long-run tracking specification.

Current state: connection retained, Tracking=false, Slewing=false, AtHome=false
after tracking/Sync; capture and journal recording continue. Local snapshot:
`/tmp/opencode/actual-driver-tracking-sync.pcap`. Site remains placeholder 45/-75.
GOTO, guiding/manual motion, Park, wider tracking/Sync failure qualification,
intermittent link reliability and ConformU remain open.

### Tracking candidate — hardware-free verification

The next uncommitted slice adds sidereal Tracking on/off through the existing
I/O worker and extends the value-only request/result used by Sync. Tracking on
requires the measured healthy session reference; off waits for stable stopping
and retains it. Sync suspends tracking, verifies both labels and restores tracking
before success. Abort stops without restoring and invalidates the reference.
FindHome and site-frame changes refuse while tracking; changing the only supported
TrackingRate (sidereal/0) is a no-op while idle. The conflict table is recorded in
the implementation plan before this slice's code.

The northern RA command is `0x06 / ff ff`; Dec is explicitly zeroed. Reported
latitude changes do not select the southern opcode. Ambiguous writes invalidate
reference/state, attempt bounded send-only stops on both axes and never reconnect
or retry. Idle monitoring permits commanded RA motion but still refuses unexpected
Dec displacement, busy/GOTO and guide states. It does not yet independently verify
the RA tracking rate against encoder elapsed time. Coordinate reads during tracking
use the RA sample's UTC rather than advancing LST over an unchanged cached encoder.
The fake models constant sidereal encoder motion, not acceleration or hardware
tracking accuracy. Tests cover on/off, frame preservation, Sync restoration,
partial/lost ACKs on each axis, cancellation and unexpected Dec motion.

The initial focused Debug run passed 33/34 cases but exposed two assertions in
the existing controller-report case: unconditional fault cleanup added stop
packets to the stationary idle path (29 requests instead of 27). The catch now
sends those stops only for an active operation or tracking, preserving the
stationary report policy rather than changing the test. Expanded Debug, TSan,
UBSan, full CTest and five tracking mutation checks are running under
`logs/tracking-final-*` / `logs/tracking-mutations/`.

Native arm64 verification now passes: focused **Debug, TSan and UBSan each
36 cases / 1,075 assertions**, with no sanitizer warnings; full Celestron-enabled
Debug CTest **553/553** in 162.16 seconds. All five tracking mutations were killed
(rate, restoration, stop cleanup, ownership and unexpected Dec displacement).
These are hardware-free results, not all-vendor or hardware qualification.
These software results preceded the approved hardware checks recorded above.

### Approved actual-driver abort, recovery and two Sync checks

The operator returned, confirmed a bare/clear mount and supervision, and explicitly
approved one stationary controller handoff. Three fresh home/stationary checks
preceded the old controller's verified disconnect. The new retained session is
`logs/stationary-idle-16kwPyzO/`, PID 1148862, TCP
`1.2.3.5:33848 -> 1.2.3.4:2000`. Binary `skyportal-driver-control-sync` SHA-256:
`f1f6564621368c7ed3034c746ed5258e4ca1c27d81db22ffa0e4785fa6de04e7`.
It links the tested Debug driver and passed fake-backed controller checks for
home/Sync and timed-home/abort before this handoff.

- A 500 ms home/abort completed, but the operator saw no movement: cancellation
  evidence only, not active-motion stopping.
- A second home attempt was aborted after two seconds. The driver reported
  stopping complete about 2.5 seconds after the halt command; the operator
  confirmed visible motion followed by a normal stop.
- FindHome then completed in about ten seconds on the same connection, restoring
  the measured reference. No intervening disconnect or reconnect occurred.
- Stationary Sync requested RA 13.303167539826 h / Dec 89.99 degrees. Completion
  took 1231 ms; readback was RA 13.303311790743 / Dec 89.990000724792, with matching
  target properties. The operator confirmed no physical motion.
- At the operator's request, another small Sync requested RA 13.331157291486 h /
  Dec 89.980000724792 degrees. Completion took 1343 ms; readback was RA
  13.331295368634 / Dec 89.980001449585, again with matching targets and operator
  confirmation of no motion. RA changes with apparent sidereal time while
  tracking is off; these checks are label/readback evidence, not optical pointing.

The retained controller continues reporting stationary, with AtHome false after
the label changes. The site remains the harness placeholder 45/-75. Capture
continues; the first checkpoint reported zero kernel drops. The repeat checkpoint
`home-abort-sync-repeat-checkpoint.pcap` reassembles without TCP gaps: 1558 requests,
matching echoes and replies, exactly two position-label writes per axis, and all
complete decoded AUX frames checksum-valid. It also contains non-AUX bytes
`2a 31 11 09 04 00 ac` in the incoming stream; do not call this corruption-free
traffic or infer that the earlier intermittent framing problem is resolved.
Local capture copy: `/tmp/opencode/actual-driver-home-abort-sync-repeat.pcap`.

These bounded checks close the initial supervised active-motion home-abort,
same-session rehome and small stationary Sync checks for this northern CGX.
They do not qualify full-rate GOTO abort, tracking restoration, general Sync
timings, WiFi reliability, or a complete driver/ConformU pass.

### Coordinate and Sync candidate — software only

The working tree now reports RA/Dec, horizontal coordinates and pointing state
from the healthy measured home reference. Reads remain snapshot-backed, refuse
unknown/faulted references, use apparent sidereal time and retain the northern
mechanical frame when reported site latitude changes. The coordinate-only
focused Debug run passed **24 cases / 907 assertions** before Sync was added.

SyncToCoordinates and SyncToTarget now queue a synchronous request on the existing
joined I/O slot; the caller waits without holding the driver mutex. Both axes
must settle before either label write, then retain the requested labels within
two counts for three consecutive samples. Each phase has at most 30 samples,
50 ms waits and a shared five-second operation deadline. These are candidate
WiFi budgets, not hardware-qualified timings. Tracking remains unsupported/off,
so this slice has no tracking restoration path. Targets publish only after full
verification on the original transport session. The geometric home origin stays
fixed while the idle-position anchor follows the verified labels.

Pre-write settling refusal preserves a healthy reference. An attempted write
followed by failure invalidates it. Abort and connection changes cancel waiting
Sync callers; a failure in an older in-flight status poll must also resolve a
Sync queued behind it, not leave its caller waiting forever. A connection setup
failure likewise resolves any waiter admitted after Connected was published.
The new cases cover both Sync forms, transient and persistent readback offsets,
lost ACKs on either motor, pre-write refusal, cancellation and that older-poll
race. The first Sync build passed **28 cases / 980 assertions**; final expanded
tests and sanitizer/stress runs are in progress. All five mutation checks killed
their intended defects: wrong target publication, skipped pre-write settling,
eight-count tolerance, removed ownership checks, and the unresolved queued-Sync
failure. Logs are under `logs/sync-mutations/` in the validation tree.

The expanded Debug run passed **31 cases / 997 assertions**, but the subsequent
TSan run reported two warnings in the older-failing-poll case despite passing
the assertions. The waiter consumed the shared exception while the poll catch
still read `error.what()` for health and logging after fulfilling its promise.
Completion now follows those reads, establishing their ordering before the
waiter proceeds; no suppression or weakened assertion was added. Sibling sweep:
`run_sync` already completes after reading the failure, connection-setup cleanup
does not inspect the exception after publication, and Abort/disconnect/link-loss
construct their own cancellation exceptions. This does not by itself establish
a runtime exception-reference-counting defect. The previous batch stopped at
TSan, so its UBSan and full-suite steps never ran. The corrected source is being
checked in a new batch (`logs/sync-handoff-*`, with source hashes recorded).
That batch still failed TSan: **31 cases / 999 assertions** passed, but one
warning concerned cancellation exception destruction in the worker versus the
caller's error-code read. UBSan and full CTest were again not reached. Ordering
the log read alone did not close the exception-lifetime gate. The candidate now
transfers an owned message/error-code result through the promise and constructs
the exception in the caller, rather than sharing an exception object across
threads. The existing queued-poll case also checks that its original failure
message survives. This revision passed focused native arm64 **TSan, UBSan and
Debug: 31 cases / 1,000 assertions each**, including lifecycle stress, with no
sanitizer warnings. Full Celestron-enabled Debug CTest passed **548/548** in
143.49 seconds. Evidence: `logs/sync-value-*-tests.log`,
`logs/sync-value-ctest.log` and `logs/sync-value-source.sha256` in the validation
tree. These are hardware-free fake-backed results, not all-vendor or hardware
qualification.

The operator has left for an errand and requires a prompt before hardware tests.
Continue hardware-free work only; obtain renewed confirmation before deployment
or hardware testing. Hardware Sync and fresh active-motion AbortSlew remain open.

The external implementation plan's task checklist has been reconciled on
2026-10-19: completed software and bounded hardware checks are ticked separately,
with the sanitizer, active-motion abort, hardware Sync and later motion gates
still open. Historical checkpoints there are explicitly superseded.

This code has **not** been loaded into the retained mount controller. That
controller still runs the previously tested home-only implementation at
`logs/stationary-idle-Eu7dsnBj/`; no hardware label write or coordinate slew has
occurred. Full driver qualification, active-motion AbortSlew, Sync on hardware,
tracking, GOTO and guiding remain unfinished. Park remains disabled.

### Actual-driver FindHome and no-op check — 20:19–20:20 UTC

Following operator approval, three fresh stationary driver samples preceded the
handoff. The old stationary-only harness had no stopped-release command, so it
was terminated only after those checks; its connection closed. The replacement
connected, verified takeover and cold AtHome=false, then received one explicit
FindHome. The actual driver reported **AtHome=true / Slewing=false about four
seconds later**. A second FindHome completed without initiating motion: the
capture contains exactly **one `0x0b` request per axis** across both API calls.

Current retained controller: PID **1125982**, TCP `1.2.3.5:57242` to
`1.2.3.4:2000`, evidence/FIFO directory `logs/stationary-idle-Eu7dsnBj/` under
the Pi validation directory. It monitors state continuously and accepts `status`,
`home`, `halt`, `close-stopped` and `power-off-confirmed`; it does not reconnect
or queue motion automatically. Previous retained-session descriptions below are
historical. No coordinate, tracking-on, guide or Park operation has been enabled.

`home-checkpoint.pcap` (local copy `/tmp/opencode/actual-driver-home-checkpoint.pcap`)
contains 163 requests, 162 matching echoes/replies and eight motor/switch frames,
all complete AUX frames checksum-valid with no sequence gap. The snapshot ends
after the next RA-position request, before its reply; this is a live-capture
checkpoint, not a completed final capture. tcpdump reported zero kernel drops
at that checkpoint. Capture buffer was increased to 8 MiB; this does not prove
the earlier process stall or packet loss is fixed. The timed home-reference idle
check passed on the same session: **61 samples**, all AtHome=true / Slewing=false,
maximum interval **1005 ms**, with no reported link fault. Evidence:
`home-idle-check.log`. This is driver home/no-op/idle evidence, not
fresh active-motion AbortSlew qualification or optical pointing validation.

### Actual-driver validation resumed — 19:43 UTC

The operator powered the mount back on and directed work back to driver completion.
The unmodified production driver source (SHA-256
`33085fa22be7a5099c3f0e1dcd69426f75170044c6fd228b83b8e94be34f1fdb`)
passed the full native Debug suite: **543/543**. Focused `[nexstar-aux]`
TSan and UBSan each passed **24 cases / 835 assertions**; this tag does not
include the two separately named lifecycle stress cases. Extended idle-fault
coverage after completed home subsequently passed in **Debug, TSan and UBSan:
24 cases / 881 assertions each**. It verifies refused stale home/motion state,
eventual link loss and an explicit reconnect without adopted home or implicit
home/encoder-label writes. Logs: `idle-reference-*-tests.log`. The full 543-case
run preceded this test-only extension; production source was unchanged.

The two explicit SkyPortal lifecycle stress cases also passed under **both TSan
and UBSan (2 cases / 6 assertions each)**, including concurrent home/abort/state
calls and destruction during connect. Logs: `resumed-tsan-stress.log` and
`resumed-ubsan-stress.log`. No sanitizer finding was reported.

A retained stationary harness now exercises that actual driver rather than raw
link calls. Its loopback check passed connect, stationary status, idle polling,
AbortSlew and motion-command refusal. It exposes no home/slew command. The driver
keeps its normal polling and 30-second fault-to-lost policy; the harness does not
disconnect on a test result or automatically reconnect after a fault.

The existing USB profile associated successfully after the CLI's 25-second wait
expired during IP configuration; no driver was started until association and
routing were confirmed. The driver then connected and reported stationary,
tracking/guide inactive, **AtHome false** as required after a power cycle.
Current evidence/FIFO: `logs/stationary-idle-oNvyAEZp/` under the Pi validation
directory. Capture and timestamped journals run throughout the retained session;
the launcher passed mocked lifecycle, existing-session refusal and capture-start
failure checks. The initial **60-second stationary driver check passed**: all
samples remained connected, unreferenced, not slewing/tracking/guiding. PID
**1119866** retains TCP `1.2.3.5:59624` to `1.2.3.4:2000`. A second one-minute
check passed. During the third, harness output paused for **37.6 seconds** and
the external runner expired before completion. The process was observed in
kernel `D` state; by the time its stacks were sampled it had resumed normal
sleep/wait states. The third interval subsequently completed with healthy state,
and queued AbortSlew/status calls returned normally on the same connection.
No AUX fault was logged. This is not a clean five-minute timing pass.

The capture checkpoint fails strict TCP reassembly with a sequence gap. A live
tcpdump statistics request reported **485 kernel-dropped packets** (3605 captured,
4090 received by filter), so this capture cannot support a claim of lossless wire
traffic or locate an AUX defect. The cause of the process stall is not proven.
The two remaining one-minute intervals subsequently completed without a reported
driver fault, ending at 19:49:57 UTC. All five intervals eventually completed on
the same retained connection; the interrupted third interval and capture gaps
remain limitations, not erased by the later passes.
The driver continues its own one-second device polling between harness commands;
this tests the production polling cadence, not an AUX-silent idle interval.

A later explicit status at **20:00:37 UTC** was still healthy on that same
connection. A replacement retained **actual-driver** controller has been built
as `skyportal-driver-control` in the remote `build-geometry-check` directory;
its source is `/tmp/opencode/skyportal-driver-control.cpp`. Loopback checks passed
completed home, repeated-home no-op, halt during active home, explicit stopped
release and EOF halt/session retention. Unlike the stationary harness, it accepts
home/halt commands while monitoring pending operations, uses the production
60-second home deadline with a 70-second harness watchdog, and never queues home
at startup. The current stationary-only process remains connected; the new
controller has not been connected to hardware. Handoff requires the operator's
exception to the no-disconnect-during-tests instruction.

Offline WiFi follow-up found three reason-7 deauthentications at 19:07:45,
19:13:38 and 19:24:16 UTC, rather than a demonstrated fixed idle interval.
After power-off, `iw` reported power saving off; USB runtime power control was
`on`, status `active`. These post-event settings do not prove their state at
failure or identify a root cause. The journals/settings are archived under
`logs/stationary-idle-preparation/`.

### Latest checkpoint — successful home, idle WiFi loss, powered off

The approved 20-second diagnostic retry completed home in **17.1592 seconds**
on both axes, with three stable comparisons, slew done and guide inactive.
Final counts were RA **1503**, Dec **338**; a subsequent status read matched.
Dec traversed the switch region, reversed and made a slower final approach.
This demonstrates why the eight-second diagnostic window was insufficient for
this attempt; it does not explain every earlier failed seek. Production deadlines
were not changed and no production driver reference was installed by this probe.

Evidence directory: `logs/persistent-home20-session-bsmTCZ3g/` under the existing
Pi validation directory. Its capture contains **392 requests, 392 matching echoes
and 392 replies**, all AUX checksums valid, plus ten valid motor/switch frames.
All 1175 captured packets were reported without kernel drops. Capture spans
**19:09:55.570366–19:11:23.991267 UTC** and does **not** cover the later failure.
The regular files are preserved locally in `/tmp/opencode/home20-evidence.tar.gz`
and its extracted `home20-evidence/` directory.

At **19:24:17.368 UTC**, the first RA-position read of a later idle check timed
out after 1000.322 ms. No new motion command had been sent. The poisoned link
refused acknowledged stop exchanges; the diagnostic attempted its bounded
send-only stops but could not verify stopping. The operator confirmed mount
power off; only then was `power-off-confirmed` sent to release PID 1106799 and
TCP `1.2.3.5:40816`. There is **no retained live session** now. Earlier paragraphs
describing retained sessions are historical checkpoints, not current state.

Kernel and supplicant journals record **deauthentication reason 7
(`CLASS3_FRAME_FROM_NONASSOC_STA`) at 19:24:16 UTC**, coincident with the start
of that one-second request. Reassociation completed at 19:24:17; NetworkManager
reported a renewed 1.2.3.5 lease at 19:24:25. Another reason-7 deauthentication
occurred at 19:13:38, after the successful capture ended. This establishes WiFi
association loss during the failure interval, not its underlying cause or the
fate of that request's TCP/AUX bytes. Do not attribute it to malformed AUX,
DHCP alone, an idle timer, power saving or the USB driver without more evidence.
The later beacon loss at 19:25:31 is distinct from the pre-power-off timeout.

Offline preparation is complete for a stationary-only controller: compiled with
`SKYPORTAL_STATIONARY_ONLY`, its home/jog bodies are excluded, and its fake checks
reject any nonzero motion write. Native checks passed healthy status/halt,
refusal of all three motion commands, idle timeout with latched refusal of
subsequent commands, and EOF session retention. It keeps the one-second exchange
deadline and existing acknowledged-stop/verification helper. No hardware run of
this binary has occurred.

The prepared launcher `/tmp/opencode/start-skyportal-stationary.sh` on the Pi
captures ARP/IP traffic and timestamped system journals for the **entire retained
controller session**, instead of expiring capture after 90 seconds. It requires
explicit invocation, refuses an existing TCP session, sends no scheduled motion,
and leaves commands on its private FIFO. It has been syntax-checked; its live
capture lifecycle remains untested. The next stationary experiment should compare
fresh status reads after 1, 5 and 15 minutes of silence, retaining the session and
capture on failure rather than reconnecting automatically. Source, helper,
launcher, binary hash and network/kernel/supplicant journals are archived remotely
under `logs/stationary-idle-preparation/`. The mount remains powered off; nothing
is armed to run when power returns.

### Stationary TCP captures — 2026-10-19 16:07–16:16 UTC

After [CFM identified the adapter](celestron-skyportal-identity.md#cfm-adapter-identification--2026-10-19-1604-utc),
resumed with read-only motor identity, position, home/slew completion and guide-status
queries. No home, stop, label, tracking or coordinate-motion command was requested.
The longer probe follows the home poll's read order and 250 ms inter-cycle wait;
its fake self-check passed before hardware use. Captures used tcpdump on the Pi's
`wlan0`, full packets, with zero kernel capture drops reported in each run.

| Capture directory under the existing remote `logs/` | Result |
| --- | --- |
| `readonly-wire-DrRN9xI2` | Eight cycles completed: 68 queries, 68 echoes and 68 replies, all complete frames checksum-valid. |
| `idle-wire-7XSgI044` | One-second probe timeout on Dec position during cycle 44 (zero-based); received stream ends in incomplete `3b 06 11 20 01 40`. |
| `idle-wire-3s-YP3Bb4VT` | Three-second diagnostic timeout on RA slew status (`0x13`) during cycle 45; received stream ends with a lone `3b`. |
| `idle-wire-pinned-lCFA7LNO` | Same three-second probe with temporary BSSID pin; external 55-second limit terminated it during cycle 31. 257 requests, 256 echoes and 256 replies captured, all complete frames checksum-valid. This is not a driver transaction-timeout result. |

All reported position pairs stayed at RA **9,212,692**, Dec **4,194,345**, stopped
and guide-inactive. Both unpinned long runs captured TCP retransmissions and
incomplete received AUX data; they do not reproduce the earlier checksum failure.
In the pinned run, the last received server payload ends at TCP relative sequence
3499, but the server FIN is at 3512: 13 server bytes are absent from this capture.
Thus even a zero-drop host capture does not prove the adapter never sent a reply.
It cannot locate loss between the bridge and the Pi or rule out radio/firmware
behavior. Pinning did not yield a completed long run; it does not establish a
background-scan cause or cure. Power saving was reported off in the pinned run.

The adapter also emitted ASCII `[88:A2:9E:55:6C:E4 associated]\r\n> ` among AUX
frames. The production parser skipped it successfully in these captures. The
offline analyzer reassembles TCP sequence ranges, checks overlaps and AUX
checksums, and reports non-AUX bytes separately; it intentionally rejects the
two truncated tails. No malformed complete frame has been demonstrated here.

Evidence includes `traffic.pcap`, `probe.log`, `tcpdump.log`, and the longer
probe's source/hash records. Local copies and the self-checked analyzer are under
`/tmp/opencode/` (`analyze-aux-pcap.py`). These probes use the existing native
Debug link library, not a deployed service. Only diagnostic probe timeouts changed;
production deadlines and checksum rejection remain intact. Normal Pi WiFi was
restored and temporary profiles removed after every run. tcpdump was installed
for capture. The original checksum failure and failed physical home remain
unexplained; further motion remains paused.

The operator reports the Pi and SkyPortal are approximately two feet apart with
no obstruction. Offline IPv4 pseudo-header/TCP checksum verification found all
received server packets valid: 111, 511, 559 and 412 respectively in the table's
four captures. This checks received TCP packet integrity, not missing packets or
the AUX bus. In the three-second run, the adapter acknowledged the final six-byte
request within about 10 ms, then delivered only its echo and the next frame's
leading `3b` before timeout. Its FIN sequence is six bytes beyond the received
stream. These observations place the missing reply bytes outside the AUX parser's
input; they do not distinguish bridge TCP behavior from WiFi delivery loss.

### Slower stationary polling — 2026-10-19 17:01 UTC

With operator approval, repeated the read-only probe with only its inter-cycle
pause changed from 250 ms to one second. Query order, three-second reply timeout,
80-cycle target and unpinned temporary WiFi profile stayed the same as the earlier
three-second run. The fake self-check passed; no production code changed.

The probe completed cycles 0–11 and timed out on Dec home status (`0x12`) in
cycle 12. All reported positions remained RA 9,212,692 / Dec 4,194,345, stopped
and guide-inactive. The final request was captured at 13:01:43.290056 local;
its complete echo and reply arrived at 13:01:47.289483, about **3.999 seconds**
later, after the probe's timeout at 13:01:46.292. The Pi reset the closed socket
on that late arrival. Offline reassembly counted 108 queries, 108 echoes and
108 replies, all AUX checksums valid, plus the association notice. Capture:
341 packets, zero kernel drops. Association-time signal was reported as -26 dBm;
this is not a continuous RF-quality measurement.

This run demonstrates a late, complete response rather than a malformed frame.
The slower cycle did not eliminate delays, but queries within a cycle remain
back-to-back, so it does not rule out sensitivity to that burst pattern. It does
not explain the earlier bad checksum or physical home failure and is not grounds
to widen production deadlines. Evidence: `logs/idle-wire-slow-iJSP4tsK/` under
the existing remote validation directory (`probe.cpp`, `probe.sha256`, `probe.log`,
`traffic.pcap`, `tcpdump.log`, `wifi-link.txt`), with a local copy under
`/tmp/opencode/`. Normal Pi WiFi was restored and the temporary profile removed.

### Individually spaced queries — 2026-10-19 18:03 UTC

The next approved read-only probe added 250 ms after every identity/status query,
retained the one-second cycle pause and three-second reply timeout, and targeted
40 cycles with a 175-second external bound. Its fake self-check passed first.
It completed two cycles, then timed out on the third RA position query. The final
six-byte TCP request was retransmitted without a captured acknowledgement; neither
its AUX echo nor its response was captured. All complete captured AUX frames were
checksum-valid (21 outgoing requests; 20 echoes, 20 replies to source `0x20`, plus
six frames between motor addresses and `0x30`/`0x31`). Those additional bus frames
were not sent by the probe. Their presence alone does not identify interference.

Both complete cycles reported stationary RA **1**, Dec **16,777,215**, home/slew
done and guide-inactive: a changed encoder baseline from the earlier runs, whose
cause is not established. Do not treat this as an otherwise identical rig-state
comparison. Association-time signal was -34 dBm; capture recorded 112 packets
with zero kernel drops. Spacing individual queries did not produce a reliable
run. No motion or label write was requested. Normal WiFi was restored and the
temporary profile removed. Evidence: `logs/idle-wire-spaced-rmcOXOll/` under the
existing remote validation directory, also copied to `/tmp/opencode/` locally.

### USB WiFi comparison — 2026-10-19 18:22–18:27 UTC

The operator added a Realtek USB WiFi dongle (`0bda:c820`, driver
`rtw88_8821cu`, interface `wlan1` on separate `phy2`). Initial general scans and
association did not find SkyPortal. After the operator confirmed availability,
a temporary profile with directed SSID probing (`802-11-wireless.hidden=yes`)
connected to the same BSSID on channel 1. This setting requests directed probing;
it is not evidence that SkyPortal actually hides its SSID.

Both captures verified `ip route get 1.2.3.4` selected `wlan1`. Ethernet and
the onboard `wlan0` connection to the normal LAN remained connected. Neither
test changed the probe binary or the three-second reply timeout:

| Test | Result | Remote evidence under existing `logs/` |
| --- | --- | --- |
| Individually spaced queries, one-second cycle pause | All 40 cycles passed; 324 queries, 324 echoes, 324 replies; every AUX checksum valid | `idle-wire-usb-spaced-CSkmrETa/` |
| Original back-to-back queries, 250 ms cycle pause | All 80 cycles passed; 644 queries, 644 echoes, 644 matching replies; every AUX checksum valid | `idle-wire-usb-fast-aYopP3Ww/` |

The fast capture additionally contains a valid `0x30`/RA `0x09` exchange and
the dongle's ASCII association notice. Both captures report zero kernel packet
drops. Every reported position remained RA 1 / Dec 16,777,215, stopped and
guide-inactive. The spaced run's association-time signal was -30 dBm.

These two completed runs show substantially better observed stationary delivery
through the USB adapter than the earlier onboard-radio attempts. They do not
isolate antenna versus radio/driver behavior, prove a cause for the original
bad checksum, or qualify homing. No motion was requested. Temporary USB profiles
were removed after each run; the installed service/configuration was unchanged.
Sources, binary hashes, route checks, probe logs and packet captures are retained
in those evidence directories and copied under `/tmp/opencode/` locally.

### Supervised USB-WiFi FindHome — 2026-10-19 18:30 UTC

After the operator confirmed readiness for the bare/clear-mount diagnostic and
manual power-off fallback, ran the existing diagnostic harness through `wlan1`.
Its fake diagnostic passed again first. The harness retained its **one-second**
transaction timeout and eight-second home caller limit (not the read-only
probe's three-second timeout). A baseline capture required all 16 axis samples
to report slew done and guide inactive before the driver ran. Cold Connect
reported AtHome false; FindHome returned in 0.002648 ms.

Home completed naturally about **3.253 seconds after dispatch** (4.1603 s from
harness start), before the abort deadline. Both home flags transitioned false
to true, motion ceased, and three adjacent stable comparisons published the
measured reference: RA **1849**, Dec **1111**. Final driver state was AtHome true,
Slewing/Tracking/Guiding false. Disconnect completed at 4.94708 s. A separate
read-only connection then returned eight unchanged, stopped/guide-inactive pairs
at those counts. Physical home orientation still requires operator confirmation;
this successful run did not exercise deadline AbortSlew.

Capture contains 92 driver requests, including exactly one home-start per axis,
with all corresponding echoes/replies checksum-valid; baseline and postcheck
each contain 68 matching queries/echoes/replies. It also contains eight valid
motor/switch-address frames, the ASCII association notice and five unframed bytes
`8a 09 04 00 ac`. The parser skipped those bytes; their origin is unknown, so do
not describe the entire stream as clean or claim the older checksum fault solved.
tcpdump reported 633 packets and zero kernel drops. Driver and postcheck exited 0.

Evidence: `logs/driver-usb-home-as00FB36/` under the existing remote validation
directory, copied under `/tmp/opencode/`, including harness source/binary hashes,
route, baseline, home, postcheck and packet capture. Temporary USB profile removed;
installed service and configuration unchanged. No further motion was queued.

### USB abort/recovery and recurring fault — 2026-10-19 18:33–18:37 UTC

The operator confirmed physical home after the preceding USB-WiFi run, then
authorized continued supervised checks. The bounded harness uses eight-second
limits for both the initial and recovery home, retaining one-second transactions.
All modes were fake-checked before hardware use; production code was unchanged.

- `driver-usb-recovery-0FqEVKy4`: Abort requested two seconds after dispatch,
  returned in 0.005166 ms; verified stopped/unreferenced about two seconds later.
  The last pre-abort sample already reported no motion, so this is cancellation
  and recovery evidence, not active-motion stopping. Rehome succeeded; postcheck
  stable at RA 1630 / Dec 16,777,150. Harness and postcheck exited 0.
- `driver-usb-early-recovery-NEQks1IP`: repeated with Abort at one second. The
  preceding sample reported both home flags false and motion active. Abort
  returned in 0.012 ms, verified stopped/unreferenced about two seconds later.
  Recovery home completed about 7.50 s after its start, within the unchanged
  eight-second harness bound; separate postcheck stable at RA 1569 / Dec
  16,777,204. Both exited 0. All 216 driver requests had matching checksum-valid
  replies, but additional unframed bytes were captured, so the stream was not
  entirely clean. Neither recovery received separate physical-home confirmation.
- `driver-usb-lifecycle-7lmIPvJ2`: intended disconnect/reconnect-during-home
  followed by idle hold. **Failed before the planned disconnect**, on Dec
  home-status `0x12`, with a 1001.46 ms transaction timeout. The driver attempted
  send-only stops, retained the fault and reported stop verification unavailable.
  The next independent read-only connection initially saw Dec still moving
  (16,574,973 → 16,528,494 → 16,509,192), then stopped at about 1.42 s and settled
  to 16,508,919; RA settled to 1568. Its exit 0 means reads succeeded, not that
  all samples were stationary. The script checked the states and skipped idle.

The failed driver's TCP stream contains no valid Dec home-status reply. It
contains unframed bytes `82 6f 22 11 20 12 00 b9` (alongside other unframed
chunks); the suffix resembles a reply but cannot be accepted as a valid AUX
frame. All complete parsed frames have valid checksums. This is now captured
evidence of unexpected bytes reaching the Pi through the USB radio, not proof
of a production parser defect or of the bus/bridge mechanism. The improved
stationary USB results did not eliminate motion-time communication failures.

All three directories are under the existing remote `logs/`, copied locally
under `/tmp/opencode/`; source/binary hashes, baselines, postchecks and raw packet
captures are retained. Temporary profiles removed, normal LAN and Ethernet
retained. Further motion paused; the operator subsequently confirmed the mount
physically stopped. This confirms stopping, not successful home or a valid reference.
Disconnect/reconnect and idle qualification remain incomplete.

### Offline replay and quiet-home stop failure — 2026-10-19 18:42–18:44 UTC

Offline analysis of the preceding failed connection found a contiguous 474-byte
server stream; all 59 server TCP packets had valid checksums. The damaged chunk
`82 6f 22 11 20 12 00 b9` arrived about 20.5 ms after the Dec home-status request,
not after its deadline. A loopback replay through the production native link
reproduced the timeout. Replacing only that chunk with the checksum-valid busy
reply `3b 04 11 20 12 00 b9` made the same link return busy. This control is an
offline experiment, never a proposed on-wire repair. Evidence: `aux-stream-replay.log`,
`aux-stream-replay-client.cpp`, `aux-stream-replay.py` and
`aux-failed-server-stream.bin` in the remote `logs/` directory.

With the operator's continued supervision and explicit control authorization,
ran a standalone probe that sent each home command once, then made no client
queries for two seconds. It retained an eight-second post-dispatch deadline,
checked identity and four stationary baseline pairs first, and passed a fake
self-check. This was a diagnostic probe, not the production lifecycle driver.
Starting counts were RA 1568 / Dec 16,508,919, away from the earlier home reference.
RA homed, but Dec kept seeking in the decreasing-count direction. After the
eight-second limit the probe called send-only stops for both axes and immediately
closed TCP. **That was not an effective physical motion bound.** The independent
postcheck continued to see Dec moving through every sample, ending at 14,321,972.
The operator subsequently confirmed that it eventually stopped.
Do not label that outcome successful home or verified
software stopping; no further movement tests are authorized by this result.

Packet evidence is specific: both zero-rate stop frames appear on the outgoing
TCP stream, the second shares a packet with FIN, and SkyPortal TCP acknowledged
both frames and FIN about 3 ms later. **Neither stop was echoed or acknowledged
at AUX level** before SkyPortal closed. This distinguishes TCP receipt from
motor acceptance. Immediate close is a plausible contributor, not a proven cause.
The standalone probe's send-only cleanup must not be used as a bounded-motion
guarantee; a healthy stream needs acknowledged stopping and subsequent stopped
verification, and ambiguous I/O still requires an independent physical fallback.

The capture has 144 outgoing probe frames, 142 echoes and 142 matching replies;
the two missing echo/reply pairs are the stops. All complete frames are
checksum-valid. Four additional motor/RA-switch frames and an ASCII association
notice were present; no binary unframed noise was found in this run. This single
quiet interval does not establish polling as the cause of earlier corruption.
Capture reported 613 packets, zero kernel drops. Evidence:
`logs/quiet-home-usb-xSBWYNEf/`, copied locally under `/tmp/opencode/`, including
source/binary hashes, home log, postcheck and pcap. Temporary profile removed.
Mount left untouched after the operator confirmed stopping; physical orientation
and a usable home reference are not established.

### Corrected diagnostic halt — offline only, 2026-10-19 18:52 UTC

The operator explicitly requested keeping the connection open during tests and
halting on faults rather than disconnecting; the mount is stopped **away from
home**. No subsequent hardware command was sent for this correction.

The standalone quiet-home probe's corrected diagnostic helper now:

- attempts acknowledged zero-rate stops for both axes on a healthy link;
- on acknowledgement failure, attempts both bounded send-only fallback stops
  but reports stopping unverified, never success;
- verifies both axes' slew-done, guide-inactive and three stable encoder-pair
  comparisons (two-count wrapped tolerance) under a separate three-second budget;
- never closes or reopens the link itself. The probe retains the session after
  success or a fault while the underlying transport remains open, including on
  stdin EOF. Explicit `close-stopped` is accepted only after verification;
  `power-off-confirmed` is the operator release for an unverified stop. A peer
  closure cannot be prevented and remains a failed verification.

Native arm64 loopback checks passed: healthy halt, delayed ACK (no early success),
missing RA ACK, missing Dec ACK, ignored stops, already-poisoned stream and peer
closure. All locally retained-session cases assert the connection remains open
and both axes receive stop commands; failed cases assert no verified result.
Operator-input checks reject `close-stopped` when unverified. The integrated probe
passed its fake home test; a fake-only subprocess stayed alive on EOF after
verification and exited on explicit stopped release. These tests do not validate
physical deceleration, bus delivery, or the long home seek.

The change is diagnostic-only, outside production sources. Archived source,
checks and hashes: `logs/diagnostic-halt-corrected/` under the existing remote
validation directory; corrected binary is
`source/AlpacaCore/build-geometry-check/skyportal-quiet-home-probe-corrected`.
The earlier probe that sent stops then immediately closed must not be used again
for motion. Further recovery needs a new supervised procedure from the current
unreferenced position, with an independent power-off fallback; no automatic
rehome or disconnect test is queued.

### Retained-session stationary halt — 2026-10-19 18:54 UTC

After the operator authorized the halt-only check, a dedicated diagnostic
executable verified the exact motor identities, issued acknowledged zero-rate
stops for both axes and passed the corrected stopped/guide-inactive/stable-count
verification. It sent no home, GOTO, label or tracking-on command. Its loopback
self-check first verified the allowed command set, both stops and retained link.
Hardware output: `STOP_VERIFIED; session retained.` This verifies a stationary
halt, not stopping active motion, and establishes no home reference.

The completed capture contains 36 requests, 36 echoes and 36 matching replies,
including both stop acknowledgements, plus four valid Dec/switch frames. Every
AUX checksum is valid; tcpdump reports 114 packets and zero kernel drops.

**Session intentionally remains open** per the operator's instruction. At this
checkpoint PID 1104628 is the halt-only executable, TCP `1.2.3.5:43548` to
`1.2.3.4:2000` is ESTABLISHED with empty send/receive queues, and temporary USB
profile `opencode-halt-hold-20261019T185431Z` remains active. Ethernet and normal
`wlan0` LAN remain connected. No command is queued: the process waits on
`control.fifo` in `logs/halt-only-usb-5e0xfCQC/` under the existing remote
validation directory. That directory holds its PID, profile, source/hash records,
halt log, route check and packet capture. The process accepts only explicit
`close-stopped` or `power-off-confirmed` release, not home/movement commands.
Do not kill it or remove its network profile as routine cleanup during tests;
release the verified stopped session only with the operator's agreement.

### Operator-approved stationary handoff — 2026-10-19 18:58 UTC

The operator approved one closure of the verified-stopped halt-only session to
replace it with a persistent diagnostic controller. The old session was released
through its `close-stopped` command; the USB profile remained active. The new
controller verified identity, acknowledged both stops and verified stationary
state before becoming ready. A subsequent explicit status read reported RA 1531,
Dec **16,026,287**, both stopped and guide-inactive. This is current raw position,
not a home reference, and does not establish why the preceding seek eventually
stopped there.

Current retained process: PID **1105066**, executable `skyportal-session`, TCP
`1.2.3.5:47450` to `1.2.3.4:2000`. Current command FIFO, logs, source/hash records
and capture are in `logs/persistent-usb-session-2tRMGSBE/` under the existing remote
validation directory. This supersedes PID 1104628 and its FIFO above. The existing
`opencode-halt-hold-20261019T185431Z` USB profile remains active. No motion command
has been sent by the new controller.

Its explicit commands are `status`, `halt`, `jog-ra`, `jog-dec`, `close-stopped`
and `power-off-confirmed`; there is no home or coordinate command. A jog uses
fixed rate 1 for 250 ms after acknowledgement, followed by acknowledged halt and
stop verification on the same connection. Any communication failure latches out
further jogs and attempts halt. EOF also halts and retains the session. A software
duration is not a physical stop guarantee; manual power-off remains the fallback.
The native fake self-check exercised both jog command sequences, status, halt,
unknown-command refusal and stopped release. That fake does not model nonzero
manual rates, so this is command/control-flow evidence only; real-motion arrest
has not been qualified with this controller.

### Lowest-rate jog/halt checks — 2026-10-19 18:59 UTC

With the operator's authorization, the persistent controller performed one
250 ms fixed-rate-1 positive jog per axis, RA first and then Dec, each preceded
and followed by acknowledged halt and stable stopped verification. RA changed
1531 → 1557 while Dec stayed 16,026,287; Dec then changed 16,026,287 → 16,026,317
while RA stayed 1557. Each command reported complete with both axes stopped and
guide-inactive. A later status request returned the same final counts.

The same TCP session (`1.2.3.5:47450`, PID 1105066) remains established; no
disconnect, home, encoder-label or coordinate command occurred. This is small
lowest-rate motion/stop evidence only, not a fast home/slew stop qualification,
physical pointing calibration or confirmation that home is reached.

Evidence: `jog-check-ApFs61He/` inside the current persistent-session directory,
also copied locally under `/tmp/opencode/`. Both jog/stop sequences are captured
with valid AUX checksums and no binary noise. The capture was explicitly stopped
after the later status log completed; tcpdump reported 410 captured / 421 filter
packets and zero kernel drops. Its tail omits the last status responses, so do
not describe it as a complete capture of that final read. It contains 133
requests and 132 matching echoes/replies; the uncaptured tail starts at RA guide
status. All ten `0x24` requests (one nonzero and four zero-rate per axis) have
captured matching replies. The controller log, not that truncated capture tail,
is the evidence for the final unchanged position pair.

### Recovery planning checkpoint

Another read on the retained session confirmed unchanged RA 1557 / Dec
16,026,317, stopped and guide-inactive. Relative to the last explicitly
physically confirmed home (RA 1849 / Dec 1111), wrapped encoder differences are
approximately -0.0063 degrees RA and -16.1364 degrees Dec. These are arithmetic
differences, not a restored reference: wrapped counts do not establish the
intervening trajectory or current mechanical orientation. The operator confirmed
that the mount visually appeared about 16 degrees from home, consistent with the
encoder difference. This does not validate a home reference or establish the
complete intervening trajectory.
Keep the connection open and the mount stationary; no further home seek is queued.

### Retained-session home attempt — 2026-10-19 19:05 UTC

The operator approved a stationary handoff to a home-capable persistent
controller. Native loopback tests passed for completed home, an eight-second
home deadline followed by verified halt, and lost home acknowledgement followed
by unverified halt; both failures latched out further motion commands.

The real attempt began at RA 1557 / Dec 16,026,317. Each axis received exactly
one home-start command. RA completed and settled at 1551. Dec initially moved
toward the earlier home counts, crossed encoder zero, reached 109,339 and then
reversed. At the eight-second diagnostic deadline it still reported home busy.
The controller halted both axes with acknowledgements and verified stable stopped
readings: RA **1551**, Dec **16,662,900**. The operator clarified that it is
not quite home, visually about one or two degrees away. This is consistent with
the remaining wrapped Dec difference of about 2.48 degrees from the earlier
physically confirmed home count (1111). The seek did not report completion
before the explicit halt, so no successful home reference has been established.

Unlike the earlier communication failures, this capture is complete and clean:
248 queries, 248 echoes and 248 matching replies, all checksum-valid, plus eight
valid motor/switch frames; no unframed bytes. tcpdump reported 718 packets and
zero kernel drops. It proves this attempt ended on the diagnostic time budget,
not a missing or malformed reply. Production's 60-second home deadline was not
changed; the diagnostic limit was eight seconds.

Current retained session supersedes the prior controller: PID **1106120**, TCP
`1.2.3.5:54740` to `1.2.3.4:2000`, directory
`logs/persistent-home-session-WleQGI3t/` under the existing remote validation
directory. Source/hashes, FIFO, logs and capture are there and regular files were
copied locally under `/tmp/opencode/`. The USB profile remains active and the
connection open. The operation-timeout latch currently refuses further home/jog
commands; halt and explicit operator release remain available.

### Earlier home implementation and results (historical details below)

**Home-test result — 2026-10-19, 14:55 UTC:** after the operator returned,
reconfirmed a bare/clear rig and authorized continued supervised testing, the
actual driver passed the home/abort/recovery checks below, but a subsequent home
timed out. Further movement is paused pending investigation and the operator's
observation. This slice is not hardware-qualified.

The standalone driver harness links the native Debug libraries; the installed
service/binary/config were not changed. Driver source SHA-256:
`96f2903da989084b2a05559ddcacec587a81157360d139cc6d7e1e108bcbe8d0`.
The latest Debug focused run also passed 26 cases/836 assertions. The harness
uses placeholder northern site coordinates (45/-75), not measured site/pointing
data; no coordinate command is enabled or tested. Each harness mode was first
checked over the loopback fake.

| Hardware check | Observation |
| --- | --- |
| FindHome at 13:12 UTC | Initiator 0.005445 ms, completion about 3.50 s after dispatch; AtHome true, Slewing/Tracking/Guiding false; operator confirmed normal physical home. |
| Abort at 14:52 UTC | Requested two seconds after FindHome; initiator 0.011556 ms, Slewing remained true for about two seconds of stop verification; final AtHome false, stopped/off; operator reported all good. |
| Same-session recovery at 14:54 UTC | Abort completed unreferenced, a second FindHome established AtHome; entire connect/abort/rehome/disconnect sequence about 9 s. |
| Disconnect during pending home | Async disconnect completed, reconnect reported AtHome false, explicit rehome established AtHome; entire sequence about 9.93 s. |
| Intended idle-hold check at 14:55 UTC | Initial FindHome never completed within 60 s. Driver then stopped and retained its timeout error; harness caught it at about 63 s. Disconnect returned without a stop-verification error. Idle hold/no-op checks were never reached. |

The successful abort checks establish the driver's pending-operation cleanup,
not proof that the motor was still physically moving at the exact abort instant
or full-speed slew braking performance. No raw wire capture was retained during
these driver runs, so the timeout log does not distinguish a busy home flag from
unsettled encoder readings. Do not infer a cause or widen the deadline/tolerance.

A separate read-only capture at 14:57 UTC then found eight identical pairs:
RA **11,743,957**, Dec **16,148,871**, both home_done/slew_done true and guide_active
false. This establishes stopped/steady status after cleanup, not successful home
or the pre-cleanup failure mechanism. Normal WiFi was restored after every run.
The operator subsequently confirmed the mount was stopped **very far from home**.
Thus the post-stop home_done flags are not evidence of physical home. Only the
first driver home received explicit physical-home confirmation; later recovery
rows describe API outcomes, not independently verified switch positions.
Logs are `driver-{home,abort,recovery,disconnect,idle}-hardware.log` and
`driver-home-timeout-readonly.log` under the existing remote validation `logs/`
directory. Harness hashes are `driver-{home,recovery,idle}-harness.sha256`;
the final harness source is retained in `source/AlpacaCore/build-geometry-check/`.

### Timeout diagnostics follow-up (software only)

The WiFi backend sends the same `MC_LEVEL_START` (`0x0b`) and reads the same
`MC_LEVEL_DONE` (`0x12`, strict `0x00`/`0xff`) as the USB backend. The reviewed
code does not substitute SE leveling geometry for CGX home commands. This does
not explain the failed seek. The USB home loop uses a 180-second deadline;
the new WiFi loop uses 60 seconds and additionally polls position/slew/guide
status while homing. Neither difference has been established as the cause.

Added DEBUG home samples containing each axis's encoder count/home-done flag,
aggregate moving/guiding status and adjacent-count stability. A home timeout
now logs and retains its last pre-cleanup sample in the completion error, so
post-stop flags cannot erase that evidence. Extended the existing timeout case
to verify the retained diagnostic fields after stop cleanup. Native focused
Debug passed **26 cases / 841 assertions** (`home-diagnostics-tests.log`).
No deadline, tolerance, command sequence or reference-acceptance rule was changed.
No mount commands were sent for this diagnostic change. The hardware-tested
source was archived before editing as `logs/hardware-tested-source-before-diagnostics.tar.gz`.
Further motion remains paused until a recovery/diagnostic procedure is agreed.

### Short diagnostic retry — 2026-10-19 15:50 UTC

The operator returned the mount to home and removed the handset. After earlier
NetworkManager scans failed to show the AP, a direct `iw` scan found the same
SkyPortal BSSID on channel 1 (previously 6). A read-only baseline showed eight
steady samples: RA 9,214,784 / Dec 4,194,388, home/slew done and no guiding.

The diagnostic harness (fake-checked first) used the new DEBUG diagnostics and
an eight-second caller deadline before abort/disconnect cleanup. FindHome returned
in 0.006741 ms. Its first two complete samples were:

| RA counts | RA home done | Dec counts | Dec home done | Moving | Guiding | Stable |
| --- | --- | --- | --- | --- | --- | --- |
| 9,208,218 | false | 4,187,254 | false | true | false | false |
| 9,206,646 | false | 4,189,784 | false | true | false | false |

The following RA position query (`source=32 destination=16 command=1`) failed with
`Celestron AUX frame checksum is invalid`, about 1.3 s after home dispatch. The
driver attempted both send-only stops and faulted the stream; the harness reported
stop verification unavailable on that poisoned connection. A subsequent separate
read-only connection found eight identical samples at RA **9,212,692** / Dec
**4,194,345**, both home/slew done and no guiding. The operator reported no visible
departure from home. This independently confirms stationary post-cleanup state,
not successful home completion. The failure occurred with the handset removed.

No raw packet capture was taken, so the offending bytes and whether corruption
originated on the bus, bridge or frame extraction are unknown. Do not conflate
this checksum failure with the earlier 60-second home timeout; neither cause is
established. Normal WiFi was restored; further movement paused. Evidence:
`driver-diagnostic-{baseline,home-hardware}.log`, `driver-diagnostic.sha256`,
`driver-checksum-readonly.log`; the diagnostic harness source/binary are retained
under `source/AlpacaCore/build-geometry-check/` in the existing validation directory.

The subsequent code slice implements asynchronous FindHome and advertises
`CanFindHome`. The existing joined polling slot owns home dispatch, completion
polls and abort cleanup, so public initiators queue state under the driver mutex
without waiting on TCP. It captures measured reference counts only after both
axes report home complete, slew complete and no guiding, followed by three
stable comparisons within two counts on the original healthy transport session.
The operation has a 60-second deadline; stopping has a separate four-second bound.
An ambiguous acknowledgement is never retried: both send-only stops are attempted,
the reference is discarded and the communication fault remains visible.

AbortSlew queues verified stopping; Slewing stays true until the stop settles.
A home timeout remains visible through Slewing/AtHome after cleanup. Disconnect
invalidates the reference, joins the worker, verifies stopping and closes TCP;
every reconnect starts unreferenced. Site changes are refused during home/stop.
Sync, sky coordinates and coordinate motion remain unsupported. This reference
currently establishes AtHome only; it is not yet a pointing calibration.

Native focused Debug checks passed **24 cases / 830 assertions** on the Pi.
Full native regression passed 542/543: the sole failure was the catalog test's
old lifecycle-only description expectation. Updating that assertion to the new
home-reference description and rebuilding passed the failed case (1/1). Thus all
543 cases have passed across the full run and targeted retest, not a single
543/543 run. Logs: `home-integrated-ctest.log` and `home-catalog-retest.log`.
The first focused TSan run passed 25/26 cases without a race report; the existing
muted-telemetry case exceeded its five-second wait for a worker using a three-second
wire timeout under sanitizer/build load. That case now sets the connection's
response timeout to 100 ms, retaining its fault-age and no-reconnect assertions
and existing test deadlines. The sibling muted teardown case already uses 100 ms.
The focused TSan rerun passed **26 cases / 836 assertions**, including the two
SkyPortal lifecycle stress cases, with `TSAN_OPTIONS=halt_on_error=1` and no race
report (`logs/home-tsan-retest.log`). Focused UBSan also passed **26 cases / 836
assertions**, with recovery disabled and no undefined-behavior report
(`logs/home-ubsan-retest.log`). Its initial run passed 25/26: copying the description
rename during the build produced an old driver object and a new metadata assertion.
Rebuilding after source updates stopped resolved that mismatch; no runtime fix was
needed. Future validation builds should use frozen source snapshots rather than
updating a source directory while another configuration is compiling it.
The later text-only description
change removes `CGX` from the generic SkyPortal mount description and updates
both assertions; it does not broaden the accepted motor identity. The four new
cases cover unequal-axis completion at measured labels,
reference loss on reconnect, abort/deadline cleanup, lost home acknowledgements
on either motor without retry, and disconnect during home. The existing stress
callback now exercises FindHome, AbortSlew and AtHome. The fake has scripted home
completion and injected labels, not physical motor dynamics or stop deceleration.

All four new cases were mutation-verified on the native Pi: each mutation below
failed its intended case with assertion-failure exit 42. Mutated driver objects
were compiled from source streams and linked into separate executables under
`source/AlpacaCore/build-home-mutations/`; production source and the clean test
binary were unchanged. Logs are `logs/home-mutation-*.log` in the validation
directory above.

- "Celestron SkyPortal AUX Driver - Home captures measured labels and reconnect discards them": omit the successful home reference assignment in `poll_status()`.
- "Celestron SkyPortal AUX Driver - Abort and deadline stop home without adopting a reference": omit `stop_axes()` in the StopRequested branch of `poll_status()`.
- "Celestron SkyPortal AUX Driver - Ambiguous home acknowledgement stops both axes without retry": omit the send-only stops in `poll_status()`'s exception handler.
- "Celestron SkyPortal AUX Driver - Disconnect joins home before stopping and closing": omit the teardown `takeover()` call in `set_connected(false)`.

No service deployment was performed. The original standalone home probe below
predates this driver change; the later actual-driver results above supersede the
earlier hardware-test-pending status. Resolve the timeout and complete hardware
and ConformU qualification before release.

The user confirmed presence, a bare/clear northern CGX and no competing motion
or tracking, then confirmed the hand controller had established the current home
position and authorized rehoming if needed. This session follows merge `0b9aa128`
of upstream `46711579` into the WiFi branch; the older native results below do not
validate that merged source.

### Read-only baseline and simultaneous WiFi experiment

The original bounded identity probe ran again at 03:30:47 UTC. Both motors still
returned firmware `07 12 08 a6` (7.18.2214), model `17 88` and approach `00`.
The adapter-address GET_VER again echoed without replying; the probe exited 1 at
that final timeout and sent nothing further. This is six successful motor queries,
not a successful adapter identification. Raw JSONL SHA-256:
`e6f34097b4534675cb21434d6ccbba876b9a331752ee76515e8146bdb2217d6c`.
Local evidence is `/tmp/opencode/skyportal-resume-20261019/`, with remote original
`/tmp/opencode/skyportal-resume-e0NUNwvc/` on the Pi.

At the user's request, tested two simultaneous managed WiFi interfaces on the
Pi's single radio. `iw phy phy1 info` advertises two managed interfaces and up to
two channels. At 03:34 UTC `wlan0` remained associated with `NCC-1701` on channel
44 and a temporary `auxwifi` interface associated with `Celestron-13` on channel
6, receiving `1.2.3.6/28`. The temporary profile had no default route or DNS role.
Association and DHCP were not proof of working concurrent connectivity:

- Ten LAN-gateway pings explicitly bound to `wlan0` received no replies (100% loss).
- The simultaneous identity probe timed out opening SkyPortal TCP, before sending
  any AUX query.
- After deleting the temporary profile/interface, the unchanged `wlan0` connection
  immediately passed five of five gateway pings (about 5–12 ms).

Ethernet SSH remained connected throughout. Normal WiFi was restored and the
installed service was not changed. Evidence is in local
`/tmp/opencode/skyportal-dualwifi-20261019/` and remote
`/tmp/opencode/skyportal-dualwifi-CfU5Cw9t/`. This failed short cross-band trial
does not establish the cause or rule out other firmware/channel configurations;
use Ethernet plus a single SkyPortal WiFi association for subsequent rig checks.

### Geometry/reference slice

Added `AlpacaCore/tests/test_celestron_aux_geometry.cpp` under the Celestron build
guard. The candidate reuses the existing CGX transform with explicitly supplied
home counts. Twelve independent fixtures specify both encoder inputs and sky outputs
for both hemispheres/branches, negative declination, RA wrap, and shifted reference
labels crossing encoder zero. The inverse is checked against the literal fixture,
not against the forward function's output. A second case pins signed one-count
RA/Dec changes and the two sides of the pole. Neither test establishes physical
WiFi geometry or southern support.

The reference workflow still needs implementation and qualification. A reported
HC home position is useful for comparing raw counts, but is not permission for
Connect to adopt arbitrary labels as calibration. The next candidate is an explicit
standard FindHome operation with observed completion and settled counts, captured
under one transport generation. Cold connect and TCP replacement must remain
uncalibrated; no USB retained-fd continuity is implied. Sync follows only after
that workflow and its cancellation/stop ownership are implemented and tested.

### HC-established home capture

A separate bounded native probe using `0b9aa128`'s shared AUX link read both
motor identities, then eight pairs of position/home-done/slew-done/guide-active
values through SkyPortal at approximately 03:41 UTC. Both axes answered:

| Raw position | Home done | Slew done | Guide active |
| --- | --- | --- | --- |
| `0x000001` (1) | true | true | false |

Every sample was identical; the first-to-last observation spanned approximately
3.18 s. This is the position the operator said the HC had homed, not a home
operation initiated or witnessed by this probe. No label write, stop, tracking,
home or coordinate motion was sent. The shared link can acknowledge controller
reports; none were returned to the probe. Normal WiFi was restored afterward.

Source SHA-256: `6c766c6c01da12efd5707aa668c9ae38f5ed65e0316112ecf8168271b3dc1207`;
binary SHA-256: `0f3de6ae28e1d02f3c818195d4745c222de083bd6ac86e4aaf105a3446102108`.
Remote evidence: `/home/pauleyj/alpaca-validation/aux-wifi-0b9aa128-20261019/logs/`
(`reference-readonly.log`, `reference-probe.sha256`, `reference-capture-start.txt`).

These home labels differ from the CGX geometry helper's default quarter-turn RA
and half-turn Dec labels. Two added fixtures supply home counts `(1,1)` explicitly;
their displaced target coordinates are analytical, not measured motion. This
supports retaining measured labels with explicit reference provenance, not treating
default labels, a bare `home_done` bit, or a successful Connect as calibration.

### Supervised explicit home through SkyPortal

After the clean merged-source native suite passed **537/537**, used the user's
explicit rehoming approval for a standalone home probe at approximately 03:52 UTC.
It verified both identities and four stopped/home-complete/guide-inactive samples,
captured the transport generation, then sent `MC_LEVEL_START` once to each motor.
It read completion, guiding and position at a 250 ms inter-cycle cadence, with
one 20 s whole-probe deadline and <=1 s transaction budgets. Failure would attempt
both send-only stops and report their completion as unverified; no failure path
was exercised in this run. The production lifecycle driver was not deployed.

- Home-start ACKs arrived at 1.854 s (RA) and 1.883 s (Dec) from probe start.
- Both motors subsequently reported home/slew busy and changing positions.
- RA first reported home/slew complete at 5.192 s, Dec at 16.392 s.
- Three consecutive completed/guide-inactive sample pairs with adjacent encoder
  differences <=2 counts finished at **18.132 s** from probe start (about 16.25 s
  after the second home ACK). Dec's initial three-count change reset that count;
  the tolerance was not widened.
- Final counts: RA **16,763,171**, Dec **16,148,955**. No controller report or
  transport-generation change occurred. Exit 0, `HOME_OK`.
- The operator then confirmed normal switch/home orientation, both axes stopped,
  and no clearance or cable problem.

The probe sent no encoder-label write, GOTO, tracking-on, guide or Park command.
Homing itself moved both axes and left the original HC labels changed by their
physical displacement: it did **not** establish a fixed label convention. Capture
the settled counts from an explicitly owned home operation rather than resetting
or assuming them. This one northern CGX run establishes a candidate reference
sequence and observed timing, not optical calibration, abort/failure cleanup,
repeatability, TCP reconnect continuity or a complete FindHome implementation.

Probe source SHA-256: `4aa12f82a9e2d9f242aff933512776b452d08aeef2f47bd0ed87a019e2be9fe2`;
binary SHA-256: `25526750e361354b98534c8b368c4ff2ca5d5a6e3c8f869d72d639afc55885fc`.
Evidence in the same remote `logs/` directory: `home-probe.log`,
`home-probe.sha256`, `home-capture-start.txt`. Normal WiFi was restored, the
temporary connection removed, and the installed daemon/binary/config left intact.

### Native verification of this slice

The source archive is the Celestron-build subset of clean merge `0b9aa128`.
Native Debug CTest passed **537/537** in 245.77 s. Adding only the new geometry
test source and its CMake registration then passed **539/539** in 265.41 s;
the focused new cases passed **2 cases / 90 assertions**.

Both final cases were falsified in separate native executables compiled from
mutated source streams; the production source on disk was never modified. Each
mutation failed its intended case with four assertions and exit 42, while the
other case passed. The clean executable passed all 90 assertions.

- "Celestron AUX geometry - independent branch and hemisphere fixtures": `AlpacaCore/src/vendors/celestron/celestron_cgx_geometry.cpp:154` replace the supplied RA home reference with constant `0x400000` in the inverse result.
- "Celestron AUX geometry - signed displacement and one-count pole boundaries": `AlpacaCore/src/vendors/celestron/celestron_cgx_geometry.cpp:127` change the branch predicate from `< 0` to `<= 0`.

Logs and source hashes are in the same remote validation directory, under
`logs/geometry-*`; the auxiliary geometry executables and both probes are in
`source/AlpacaCore/build-geometry-check/`. These are native Celestron-enabled
Debug results, not new sanitizer, HTTP, all-vendor, packaging or ConformU results.
No production driver behavior or capability flag was changed by this slice.

## End-of-day checkpoint — 2026-10-15 local / 2026-10-16 UTC

Stopped after the successful supervised **idle** lifecycle check; no further
hardware commands are queued. RPi Ethernet and original `NCC-1701` WiFi are
active, temporary SkyPortal profiles are removed, and no harness/build/test
process remains running. The installed service/binary/config were not changed.

At the pre-commit checkpoint, work was uncommitted on
`driver/celestron-nexstar-aux-wifi`, HEAD `aa109c5d`. Today's production delta is only the USB diagnostic compatibility
fix; tests add the two report-publication cases. Documentation, scoped guidance,
changelog and the external plan contain the new identity/native/lifecycle evidence.
The operator subsequently requested committing and pushing this checkpoint to
the fork; use the branch history for its publication commit.

Resume with independent geometry fixtures and an explicit reference/calibration
workflow, then two-phase Sync. Connect must not invent calibrated coordinates;
tracking, GOTO, guide/manual motion and Park remain unavailable. Before another
physical test, reconfirm operator presence and bare/clear-mount conditions; today's
permission is not unattended authorization for a later session.

Final native Celestron-only Debug/UBSan **520/520**, focused TSan **48/1,411**,
stress **7/17**; read-only motor identity and one idle takeover check passed.
ASan on a compatible arm64 host, all-vendor/full integration, active-motion stop,
reference/Sync and all coordinate-motion qualification remain outstanding.

## Branch and scope

Started 2026-10-13 on `driver/celestron-nexstar-aux-wifi`, stacked on
`driver/celestron-cgx-usb` at `37de2b25`. The implementation plan is
`/home/pauleyj/src/open-astro/nexstar-aux-wifi-port-plan.md`.

Current published checkpoint: `aa109c5d`, two WiFi commits above PR #1200 USB
head `6e38ede3cb40fdc17e1aacff5939a8d5804179dc`. Local/fork main are at
`c4f8937f106c9a7e389e3bef5051544f995b6998`. The latest user-approved rebase
and force-with-lease push supersede the older uncommitted/no-push status below.
Deployment and physical-test restrictions remain in force.

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

### Latest USB rebase and fork publication — 2026-10-15

After the user approved committing/pushing the review slice (`61d338ac`), main
advanced to `c4f8937f` and local `driver/celestron-cgx-usb` advanced to PR #1200
head `6e38ede3`. The user requested rebasing WiFi onto that USB branch: the two
WiFi commits became `c9d57963` and `aa109c5d`. Backup ref
`backup/aux-wifi-pre-cgx-rebase-20261015T233852Z` preserves `61d338ac`.
The fork WiFi branch was subsequently updated to `aa109c5d` with an explicit
force-with-lease at the user's request. No merge/rebase is pending.

The USB telescope implementation, shared telescope header, CGX
protocol/telescope/telemetry tests and published USB report match `6e38ede3`
exactly. Conflict resolution retained newer USB transaction-failure diagnostics
in the extracted shared link's `exchange_impl()`. The imported USB report now
records motors 7.18.2214 / model `0x1788`, binary `f11acde9`, bare northern CGX,
with all 98 timing rows passing. This is USB evidence, not WiFi qualification.

Immediate post-rebase C++20 syntax-only and whitespace checks passed. Earlier
505-case Debug/UBSan and TSan passes below retain their `709f1976` attribution;
they do not certify `aa109c5d`. Native reruns are the next offline gate. No
deployment, real mount query, takeover or motion occurred during the rebase/push.

## Verification (historical source snapshots)

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

### Offline report-publication regressions — 2026-10-15

Added two lifecycle-driver cases after the `aa109c5d` checkpoint, using the
existing TCP fake and virtual clock. Both motor addresses are exercised: low
battery warnings remain diagnostic-only, while motion-limit warnings and controller
errors are acknowledged, refuse the warm Tracking/Slewing snapshot, retain their
fault without autonomous polling/reconnect, and become lost at 30 virtual seconds.
No production behavior, capabilities, fake dynamics or USB sources changed.

C++20 syntax-only, changed-range formatting and whitespace checks passed locally.
Native arm64 focused Debug, UBSan and TSan each passed **20 cases / 726
assertions**, including these additions. Both mutations below were observed
failing with exit 42, then original production code was restored and the clean
focused cases passed. The low-battery mutation caused 14 failed assertions;
the motion-report mutation caused 4. Literal `if(false)` needed an otherwise
inert `static_cast<void>(report)` to avoid an unused-variable build failure.
These are fake-only results, not physical report qualification.

- "Celestron SkyPortal AUX Driver - Low battery reports remain diagnostic-only": AlpacaCore/src/vendors/celestron/celestron_nexstar_aux_telescope_driver.cpp:523 replace the diagnostic-warning condition with false.
- "Celestron SkyPortal AUX Driver - Controller motion reports latch until virtual-time link loss": AlpacaCore/src/vendors/celestron/celestron_nexstar_aux_telescope_driver.cpp:526 replace the controller-report throw with continue.

### Rebased native validation — one USB regression blocks motion

**Resolved by the USB-prefix fix below.** The failures in this section are the
pre-fix results; do not treat them as the final validation outcome.

Evidence directory on `pauleyj@192.168.4.44`:
`/home/pauleyj/alpaca-validation/aux-wifi-aa109c5d-20261015-offline/`.
Full commands/results: `logs/RESULTS.txt`.

| Source | Native Debug CTest | Native UBSan CTest |
| --- | --- | --- |
| Frozen `aa109c5d` | 517/518 | 517/518 |
| `aa109c5d` plus the new report-test file | 519/520 | 519/520 |

All four full-suite runs fail only **Celestron CGX AUX link - logs the original
transaction failure once without changing exceptions**, at
`test_celestron_cgx_protocol.cpp:715,720`. Its qualified USB error-prefix
expectations differ from the extracted shared AUX implementation. Do not loosen
those assertions; preserve USB compatibility in the transport before declaring
this rebase validated or running the physical-driver harness.

Updated-source TSan passed **20 focused cases / 726 assertions** and **7 stress
cases / 17 assertions**, with no sanitizer reports. ASan remains blocked by the
known Pi kernel layout. HTTP/package and ConformU were not run. Source verification
confirmed the only change from the frozen archive was the authorized report-test
file (SHA-256 `fece37456628fe04e7c8341df0a47225ebba6d658047a66dfbe897cde19206b5`).
The archive hash is `cc37ccdb18c058cd066c1118a64faace37750a34f08cdda2b834c71e2ef5b577`;
the validated test-only dirty diff hash is
`11630e0edc331cf128b17d57b9ae06a7f11662eb665732e6c4e528ff6ae9d321`.

A native standalone lifecycle harness was built under
`source/AlpacaCore/build-debug/aux_lifecycle_harness` in that isolated directory,
but **never executed**. Its initial binary hash is
`4466b3a81e12a4e869d14907ba562900fe74348b33b33862d9beb7f4616a8028`;
it must be rebuilt and rehashed after the USB diagnostic fix. It uses explicitly
logged dummy northern site 45/0 for this status-only test, connects, observes idle
telemetry for about three seconds, then disconnects. It makes no sky-motion calls,
but its lifecycle calls send takeover stops and require supervision.

### USB-prefix compatibility fix and final native reruns

Changed only `celestron_aux_link.cpp`: serial errors keep their historical USB
prefix/detail text, while TCP errors retain AUX text. Read/write/poll helpers
capture transport kind before disconnect cleanup resets it. The existing USB
regression assertions are unchanged; no timing, state, I/O or error-code behavior
was changed. The sibling sweep covered serial open/configure/exclusivity/flush,
TCP connection and queued I/O, exchange parsing/desync, report ACKs and both stops.

Native final evidence is under the preceding isolated directory's
`logs/usb-prefix-fix-20261016/` (`RESULTS.txt`, `final-source-verification.txt`,
`final-sha256.txt`, `final-*` logs). Earlier evidence was preserved unchanged.

- Debug CTest: **520/520 passed**, 140.83 s.
- UBSan CTest: **520/520 passed**, 155.70 s, no sanitizer finding.
- Original unchanged USB diagnostic case: **60 assertions passed** in each build.
- Focused TSan: **48 cases / 1,411 assertions passed**.
- Separate TSan stress: **7 cases / 17 assertions passed**; no race reported.
- Final production-file SHA-256: `cdd2b1e94bd835b6f10eb734948cc5db35d9bb08e393c7492b7cc01476780ec8`.
- New report-test file remains `fece37456628fe04e7c8341df0a47225ebba6d658047a66dfbe897cde19206b5`.

The source is `aa109c5d` plus the report tests and USB-prefix fix, **not a clean
commit**. ASan, full TSan, all-vendor/HTTP/package checks and ConformU remain
outstanding. An intermediate overlapping-build collision was excluded; final
recorded runs were rerun without duplicate jobs.

### Supervised SkyPortal lifecycle check — 2026-10-16 01:23:58 UTC

After the final native passes and code review, ran the bounded standalone
harness under the operator's explicit approval/presence and bare/clear-mount
confirmation. Rebuilt harness SHA-256:
`956409ac9c441c93c348978bfc484b508b2f109d1fca4ecb1588ddde59f9f7cf`.
It used the same validated production source above, host `1.2.3.4:2000`, and
explicitly logged dummy site 45/0 without writing site settings to hardware.

Results: synchronous Connect **910.469 ms**; firmware `RA 7.18.2214, Dec
7.18.2214`; Connected true, no link fault, Tracking/Slewing/Guiding false.
Those states remained unchanged during approximately three seconds of idle
observation. Explicit Disconnect returned Connected false, no firmware cache and
no retained fault. Harness result **OK**, exit 0; no timeout occurred. No separate
Disconnect timing was measured.

This **did send takeover stops** through the real driver on Connect/Disconnect.
It did not enable tracking, home, Sync, slew, guide, manual motion or Park. The
installed service/binary/config were untouched. Original WiFi was restored and
the new temporary profile removed; Ethernet SSH stayed active.

Evidence: remote `/tmp/opencode/skyportal-takeover-20261016-N5Gzyg/`, local
`/tmp/opencode/skyportal-takeover-evidence-20261016/`; `harness.log` SHA-256
`b94be006e215ae6cccb2f72f3f8defcb583acac51cf494d0166612492898921f`.
This is a single **idle lifecycle/takeover check**, not moving-axis deceleration,
native-guide cancellation, reference/calibration, optical or ConformU qualification.

### Remaining implementation and qualification

After the read-only capture, the operator explicitly authorized motion tests
when ready, confirmed presence, bare mount/no OTA, clear obstacles/cables and
no competing controller/client motion or tracking. The bounded idle lifecycle
check above subsequently passed after native validation. Coordinate-motion
capabilities remain disabled; moving-axis and guiding takeover still need their
own supervised evidence.

The [read-only SkyPortal capture](celestron-skyportal-identity.md) on
2026-10-16 00:11:44 UTC verified both motors' `07 12 08 a6` firmware
(7.18.2214), model `17 88`, and approach `00` through `1.2.3.4:2000`, source
`0x20`. The adapter GET_VER at `0xb5` returned only an echo and timed out;
adapter generation/firmware remain unknown. Original WiFi was restored and
the temporary profile removed. No deployment, driver Connect/takeover or motion
occurred. Earlier unprivileged cached scan listings were incorrectly described
as fresh; a privileged directed scan found the AP. They do not prove its absence.

The Telescope V4 lifecycle-only backend, initial snapshot, fault/lost-state policy,
narrow firmware cache and catalog selection are implemented. The candidate identity
profile now validates matching four-byte firmware (including the build high byte)
and exact two-byte CGX model `0x1788` on both motors. Legacy/mixed replies and
CGX-L are refused before takeover. The candidate was originally USB-sourced and
fake-tested; the capture above now verifies this northern CGX's identity reply
shape through SkyPortal. Idle takeover passed the bounded lifecycle check above;
moving-axis stopping, guide cancellation and coordinate operation remain unqualified.
Next come reference/Sync and stop/motion implementation.
Connect must not manufacture a calibrated reference or guessed tracking state.

## Historical uncommitted review work — 2026-10-14

At this historical checkpoint, new work commits, WiFi pushes and deployment
were awaiting user review. The USB merge was staged with explicit approval,
then replaced by the requested upstream-main rebase. Later explicit approval
allowed committing and publishing the WiFi work as recorded above; it did not
authorize deployment or motion tests.
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
