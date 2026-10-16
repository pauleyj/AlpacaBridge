# CGX identity through SkyPortal — read-only capture

## Scope and provenance

Captured **2026-10-16 00:11:44 UTC** from the RPi4 over raw TCP AUX at
`1.2.3.4:2000`, application source `0x20`. Radio: `Celestron-13`, BSSID
`4C:55:CC:14:B4:13`, channel 1, open network. The temporary client received
`1.2.3.5/28`; DHCP identified `1.2.3.4` as server/router. Ethernet SSH remained
active. The original `netplan-wlan0-NCC-1701` connection was restored and the
new temporary profile deleted after capture, including after the final timeout.

Repository checkpoint: `aa109c5d` plus uncommitted documentation and offline
report tests. **No driver binary was deployed or connected.** A standalone
Python probe sent only the seven queries listed below, with a fixed command
set, bounded TCP waits, checksum validation and reversed-address matching.
It sent no report ACKs, stops, takeover, position writes, tracking, home, slew,
guide or Park commands. After the unmatched adapter query it closed the socket
without another query or retry.

Probe SHA-256: `ef5718d471655837326db82ac3ce648fab9a346798cae7498141f224deeecebc`.
Raw JSONL SHA-256: `49ba1f1292fab01eb167cbc0ef023ab98dc3923c619500cc8ba693f9402fb82d`.
Local evidence: `/tmp/opencode/skyportal-readonly-capture-20261016/identity.jsonl`
and `session.log`. Remote originals:
`/tmp/opencode/skyportal-readonly-20261015-aNDMVi/` on `192.168.4.44`.
The complete AUX frames are preserved below; temporary paths are not durable
repository artifacts.

## Raw requests and replies

Each request was echoed byte-for-byte before its reply. All received complete
frames had valid length/checksum. Echoes were not mistaken for replies.

| Query | Request | Matching response | Interpretation |
| --- | --- | --- | --- |
| RA GET_VER | `3b 03 20 10 fe cf` | `3b 07 10 20 fe 07 12 08 a6 04` | 7.18.2214 |
| Dec GET_VER | `3b 03 20 11 fe ce` | `3b 07 11 20 fe 07 12 08 a6 03` | 7.18.2214 |
| RA MC_GET_MODEL | `3b 03 20 10 05 c8` | `3b 05 10 20 05 17 88 27` | `0x1788` |
| Dec MC_GET_MODEL | `3b 03 20 11 05 c7` | `3b 05 11 20 05 17 88 26` | `0x1788` |
| RA MC_GET_APPROACH | `3b 03 20 10 fc d1` | `3b 04 10 20 fc 00 d0` | `00` (positive approach) |
| Dec MC_GET_APPROACH | `3b 03 20 11 fc d0` | `3b 04 11 20 fc 00 cf` | `00` (positive approach) |
| WiFi address GET_VER | `3b 03 20 b5 fe 2a` | No matching response within 2 s; request echo only | Adapter firmware unknown |

Firmware build bytes `08 a6` are big-endian: 2214. These both-motor identity
payloads match the candidate WiFi profile and the latest USB identity evidence.
This verifies their reply shape through this adapter, not the rest of the driver.

## Limits and next gates

The adapter's hardware generation and firmware are **still unknown**. Silence at
`0xb5` does not identify a generation, prove absence, or justify probing arbitrary
commands. Obtain its label/model or independently sourced identification procedure.

The identity capture itself tested no physical stop, native-guide cancellation, calibration/reference continuity,
Sync, tracking, GOTO, guiding, manual motion or Park behavior was tested. No
ConformU run occurred. CGX-L, other firmware/adapters and southern operation are
not qualified by this capture. Obtain fresh operator approval/presence before
deployment or driver Connect: Connect sends takeover stops.

A subsequent separately approved [idle lifecycle check](celestron-nexstar-aux-wifi-progress.md#supervised-skyportal-lifecycle-check--2026-10-16-012358-utc)
passed Connect/takeover, approximately three seconds of idle telemetry and
Disconnect on the same adapter/motors. It does not qualify moving-axis stopping,
guide cancellation or coordinate operation.

## Scan correction

Earlier unprivileged `nmcli ... --rescan yes` listings showed only cached
`NCC-1701` data. An explicit directed rescan reported `not authorized`; repeating
it with `sudo -n` and then reading the completed scan found `Celestron-13`.
Those earlier listings do **not** establish that the AP was absent. The older
failed association is retained as historical evidence, not proof of RF absence.
