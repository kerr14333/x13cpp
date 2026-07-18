# tests/corpus/extra/data — provenance

This directory is reserved for **new** data files unique to the extra corpus
(e.g. synthesized user-defined regressor series). If a file is added here it
MUST be reproducible from a committed, seeded generator and documented below.

Currently the extra specs reference only the shared corpus series in
`../../data/` (`airline.dat`, `payems.dat`) — no bespoke data files are needed,
so this directory holds only this README. The `pickmdl.mdl` model file lives one
level up (`tests/corpus/extra/pickmdl.mdl`); it is not a data series but the
list of candidate ARIMA models for the `pickmdl` spec — the classic
X-11-ARIMA/88 five-model set (Dagum 1988), with the airline model marked as the
default (`*`).
