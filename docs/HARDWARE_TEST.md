# Physical New Nintendo 3DS acceptance checklist

Compilation and emulator behavior do not satisfy this checklist.

When physical testing begins, copy `HARDWARE_REPORT_TEMPLATE.json` to
`HARDWARE_REPORT.json` and replace every placeholder with measured evidence.
After the tested `.3dsx` is still present at the repository root, run:

```sh
make validate-hardware-report
```

The validator checks the artifact hash, all three cold-boot runs, the 5–8-minute
duration, 30 FPS target, sustained-drop rule, per-zone 20% memory headroom,
zero crashes/audio underruns/unmasked loads, edge cases, both deployment paths,
and the uncut physical-console proof. A missing report intentionally fails.

## Console readiness

- [ ] Record exact model: New 3DS, New 3DS XL, or New 2DS XL.
- [ ] Record system version, Luma3DS version, and Homebrew Launcher version.
- [ ] Back up important SD-card contents before changing the card.
- [ ] Confirm SD card is healthy and Homebrew Launcher can run a known homebrew app.
- [ ] Record the test build commit and SHA-256.

## Deployment evidence

- [ ] `make run IP=<3DS-IP>` transfers and starts the `.3dsx` through netloader.
- [ ] The persistent SD bundle launches with the Mac disconnected.
- [ ] Select returns cleanly to Homebrew Launcher.
- [ ] Sleep/wake resumes without corrupting rendering, input, or audio.

## Playthrough gate

Run three consecutive cold-boot playthroughs using the intended 6:40 beat sheet in [PLAYTHROUGH_ROUTE.md](PLAYTHROUGH_ROUTE.md). For each run record:

| Run | Duration | Boot | Interior/reveal | NPC/fog gate | Boss | Restart | Minimum FPS | Peak linear memory | Audio underruns | Result |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 |  |  |  |  |  |  |  |  |  |  |
| 2 |  |  |  |  |  |  |  |  |  |  |
| 3 |  |  |  |  |  |  |  |  |  |  |

Required: a coherent 5–8 minute sequence, no crash, no audio underrun, no sustained performance below 24 FPS, no visible unmasked loading, and at least 20% measured memory headroom at every zone's peak.

## Edge cases

- [ ] Pause/resume in each zone.
- [ ] Abort and repeat the NPC interaction.
- [ ] Lose lock-on when the boss dies.
- [ ] Die during each boss attack class and restart.
- [ ] Win and restart.
- [ ] Repeat both zone transitions at least five times across sessions.

## Proof recording

- [ ] Uncut shot shows the physical console and both screens.
- [ ] Shot includes Homebrew Launcher, application selection, launch, and gameplay.
- [ ] Do not describe the build as verified real-hardware software until this checklist passes.
