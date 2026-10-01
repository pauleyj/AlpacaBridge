### Fixed
- **OnStep, Celestron and SynScan: a failed mount stop no longer reports success
  or clears the motion state** (AlpacaCore, issue #742): OnStep's
  `MoveAxis(axis, 0)` and `AbortSlew` attempt every required stop and report the
  first failure as `DriverException`, retaining the previous motion flags.
  Celestron and SynScan independently attempt GOTO cancellation and both axis
  stops during failed park cleanup and `Unpark`; a failed cleanup logs at ERROR,
  and a failed `Unpark` joins the cancelled park task before returning
  `DriverException`. Their manual-axis motion flag is also cleared only after a
  successful stop. Hardware-free regressions cover first/later stop failures,
  silent links, recovery and motion-state preservation; ConformU not re-run.
