### Changed (validation)

- **Celestron CGX direct USB ConformU report refreshed on v6.0.0-beta.2** (issue #2119): ConformU 4.6.0-rc.1 passes with 0 errors, 0 warnings, 0 issues and no timing misses on the bare northern CGX, on the v6.0.0-beta.2 release package (`stable/6.0` `61cade85`) on a Raspberry Pi 5, with the CGX's data interface bound to the generic usb-serial driver because `cdc_acm` stops the Pi 5's USB controller on this mount. `AlpacaCore/conformu/Celestron/CGX/Linux-arm64-usb.txt`.
