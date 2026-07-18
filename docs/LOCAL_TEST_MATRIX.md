# Automated and local test matrix

This matrix separates deterministic proof from hardware-only claims.

| Contract flow | Automated/local evidence | Physical evidence still required |
| --- | --- | --- |
| Clean asset pipeline | Manifest/reference/license/format/size/zone-budget checks; generated registry and scene freshness; T3X v1/RGB565 descriptor checks | Visual texture correctness and load-time behavior |
| Boot and metadata | 3DSX header, SMDH fields, RomFS markers, linked subsystem symbols, hashes | Homebrew Launcher boot on the exact console |
| Persistent SD bundle | Deterministic ZIP layout, embedded commit/hash metadata, and archived 3DSX hash verification | Launch from SD with the Mac disconnected |
| Indexed world rendering | Native verifier enforces Citro3D's byte-index enum; corrected artifact renders vestibule geometry in Azahar | Exact textures, winding, depth, and culling on PICA200 hardware |
| Title to play | `GameSession` title confirmation test | Real A-button timing and both-screen presentation |
| Interior to vista | Door preload, masked reveal, enter, prior-zone unload, counters | No visible loading and frame-time behavior |
| Dialogue | Start, abort with B, retry, complete | Subtitle readability and input feel |
| Vista to arena | Fog-gate preload/fade/enter/unload assertions | Masking, audio continuity, and frame pacing |
| Combat | Forward hitbox, rear-target rejection, stamina cost, dodge invulnerability, healing | Readability, latency, C-stick, L/R/ZR ergonomics |
| Boss lifecycle | Lock-on, out-of-range target loss, death lock release, victory | Both attack telegraphs and all death edge cases |
| Restart stability | Death restart plus five consecutive arena victory/reset cycles | Three cold-boot full playthroughs and repeated transitions |
| Pause/suspend | Pause freezes simulation; suspend/resume restores prior mode | lid sleep/wake rendering, input, and NDSP continuity |
| Performance counters | Frame ms, visible/culled draws, loaded-zone budget, peak/free linear memory, underruns are rendered | 30 FPS target, 24 FPS floor, zero underruns, 20% memory headroom |

Run the local gate with:

```sh
make clean
make audit-repo
make validate-assets
make test-host
make verify-build
make package-sd
```

Only [HARDWARE_TEST.md](HARDWARE_TEST.md) can close the final column.
