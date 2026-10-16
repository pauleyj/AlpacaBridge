### Added
- **Celestron SkyPortal AUX transport and lifecycle foundation** (AlpacaCore): share the existing USB AUX framing and bounded exchange implementation with numeric-IP TCP connections, process-local HC/AUX endpoint exclusion, controller-report acknowledgements, and stop-only sends after ambiguous replies. Include queued TCP I/O in transaction deadlines, close incomplete frame writes before cleanup can append command bytes, and release HC endpoint ownership on confirmed TCP loss. Add selectable lifecycle-only configuration and a USB-sourced candidate CGX identity profile with full firmware-build decoding and both-motor validation. WiFi coordinate motion and real-hardware qualification remain in progress.

### Added (tests)
- **SkyPortal controller-report publication** (AlpacaCore): cover diagnostic-only low-battery warnings and retained motion-warning/error faults through warm telemetry refusal and virtual-time link loss on both motor addresses.

### Changed (docs)
- **SkyPortal identity evidence**: record read-only both-motor firmware 7.18.2214, model `0x1788` and approach `00` replies through the observed adapter; retain unknown adapter firmware and all operational qualification gates.
- **SkyPortal idle lifecycle evidence**: record the supervised standalone Connect/takeover, idle observation and Disconnect check without claiming moving-axis stop, calibration or coordinate-motion qualification.

### Fixed
- **Shared AUX USB diagnostics** (AlpacaCore): preserve historical CGX USB exception text after transport extraction, while retaining TCP AUX diagnostics; keep transport identity across disconnect cleanup without changing I/O, timing or error codes.
