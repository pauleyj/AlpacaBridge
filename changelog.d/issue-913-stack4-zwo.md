### Breaking changes
- **ZWO camera clients must retrieve or abort each exposure before changing ROI geometry** (AlpacaCore, issue #913): BinX, BinY, NumX, NumY, StartX and StartY changes now return InvalidOperation while an exposure image remains undownloaded; models supporting both Y8 and RGB24 without a higher-priority format now default to Y8. **After upgrading:** retrieve each completed image or abort it before setting a new ROI, and explicitly select RGB24 if that is preferred.
- **ZWO cameras report invalid frames and failed lazy downloads** (AlpacaCore, issue #913): unsupported SDK formats and failed or inconsistent frame downloads now surface a DriverException through ImageReady and ImageArray instead of leaving the exposure ready with stale or fabricated pixels. **After upgrading:** inspect the reported camera acquisition error and verify the selected ROI or SDK state.

### Fixed
- **ZWO camera frame publication** (AlpacaCore, issue #913): validate exposure-time ROI/format metadata and output shape, preserve requested geometry with zero-padding only for sensor-edge pixels omitted by SDK alignment, and latch lazy-transfer failures until a new exposure succeeds.

### Added (tests)
- **ZWO fake-SDK frame regressions and stress** (issue #913): cover lazy readiness, padded Raw16/RGB24 conversion, odd binned full-frame sensor edges, metadata mismatch, transfer failure/recovery, duplicate reads, abort/disconnect during download, and connected lifecycle/exposure churn.
