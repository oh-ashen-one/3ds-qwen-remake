# Physical original Nintendo 3DS (CTR-001) acceptance checklist

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

- [x] Photo confirms the aqua original Nintendo 3DS family (CTR-001), Japanese region.
- [x] Confirm the original CTR-001 model and record System Settings version `11.17.0-50J`.
- [ ] Record Homebrew Launcher version; Luma3DS `v13.4` is installed.
- [x] Back up important SD-card contents and the post-install SysNAND recovery image outside the repository.
- [ ] Confirm Homebrew Launcher can run a known homebrew app; the SD card has passed FAT32 verification.
- [ ] Record the test build commit and SHA-256.

Boot9strap, Luma3DS, and the standard finalization tools were installed using the current official [3DS Hacks Guide](https://3ds.hacks.guide/get-started.html) for this model and firmware. This setup milestone does not satisfy the game-specific physical acceptance gate below.

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
