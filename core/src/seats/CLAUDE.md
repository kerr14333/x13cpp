# SEATS — working notes

The measured records for this subsystem are in **`docs/M5_PORT_NOTES.md`**.
Read the relevant entry BEFORE changing anything here; each one is a bug
already paid for once.

| entry | what it covers |
|---|---|
| **0** | the decomposition core — the AR-polynomial sign fixes (`phis = +mo.phi`, `bphis = +mo.bphi`), CALCFX's Pstar>0 stationary-AR filter, the `imean` centering (wm seeds, `za` in FCAST, `wmf`/`wmb` in ESTBUR), and the mean+TD combined-factor rule |
| **22** | the `seats{}` option surface — `imean`, the HP family (bridge closed, filter walled), `finite`, `noadmiss`, and CB-14/15/16 |
| **33, 34** | `slidingspans{}` / `history{}` under `seats{}` — the shared X-11 pre-stage, and why `Tsrs` must be rebuilt per span |
| **45** | QS / NP diagnostics under `seats{}` — and `editor.f:517-518`'s Muladd force |
| **49** | `composite{}` under `seats{}` (`agr3s.f`) |
| **53** | the FORECAST decomposition — **still open**, 39 of 52 specs wrong in two families; see also `tools/seats_forecast_scouting.md` |

## The two traps most likely to bite here

- **The decomposition is unique only up to a constant log shift between the
  seasonal and the trend, and the bias block absorbs exactly such a shift.**
  So the historical tables cannot pin the normalization: `mean(s11/s12) == 1`
  holds identically for any `bias2c`. Only the forecast trend can distinguish
  them. This is why the forecast trend takes `bias1c` where `sigsub.f:1594`
  literally reads `bias3c` — see entry 53.

- **A span replay holds the model fixed, so `Tsrs` is never refilled.** Its
  only two writers are the `resid` calls inside `rgarma`, both behind
  `Nestpm > 0`. The oracle does not care because it hands SEATS `Orixs`
  instead; this port pre-linearizes into `tsrs`, so a span must rebuild it or
  every span decomposes the full series (measured 6.2e-3 against a 5e-15
  target). Rebuild in the TRANSFORMED scale from `orix` minus the regeff
  arrays, and do it BEFORE the `adjreg` call — `adjreg` inverse-transforms
  `orix` in place.
