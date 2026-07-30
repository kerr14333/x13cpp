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
| **53** | the FORECAST decomposition — **CLOSED 2026-07-30**, all 52 specs gate; the entry itself is superseded, read `tools/seats_forecast_scouting.md` |

## The two traps most likely to bite here

- **The saved FORECAST tables are not the components.** `tfd/sfd/afd/yfd` come
  from `ansub4.f`'s `ftr/fsa/fs/fcyc`, which refold the deterministic
  preadjustment factors back onto the antilogged components — not from
  `sigex.f:3631-3636`, whose punches are guarded `if (Tramo .le. 0)` and are
  DEAD on an X-13 run. Two separate "unreachable" claims in this front turned
  out to be live code, and they cancelled each other, which is why 46 of 52
  specs passed with both halves wrong. Never assert reachability here without
  running the oracle — `tools/seats_forecast_scouting.md` §3 has the recipe.

- **A span replay holds the model fixed, so `Tsrs` is never refilled.** Its
  only two writers are the `resid` calls inside `rgarma`, both behind
  `Nestpm > 0`. The oracle does not care because it hands SEATS `Orixs`
  instead; this port pre-linearizes into `tsrs`, so a span must rebuild it or
  every span decomposes the full series (measured 6.2e-3 against a 5e-15
  target). Rebuild in the TRANSFORMED scale from `orix` minus the regeff
  arrays, and do it BEFORE the `adjreg` call — `adjreg` inverse-transforms
  `orix` in place.
