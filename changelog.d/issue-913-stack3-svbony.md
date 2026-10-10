### Breaking changes
- **SVBONY cameras reject malformed frames and unsupported connect formats** (AlpacaCore, issue #913): invalid SDK ROI metadata, unsupported formats, incomplete frames, or no supported connect-time output format now refuse connection or surface a DriverException through ImageReady and ImageArray.

### Fixed
- **SVBONY camera frame publication** (AlpacaCore, issue #913): verify SDK ROI/format readbacks and source capacity before caching; expose conversion/acquisition failures and recover on the next valid exposure.

### Added (tests)
- **SVBONY fake-SDK connect, exposure and frame regressions** (issue #913): exercise default format selection/refusal with balanced opens, non-finite/overflow duration rejection, watchdog failure reporting, valid publication, malformed ROI/format rejection, failure recovery, and connected lifecycle/exposure churn.
