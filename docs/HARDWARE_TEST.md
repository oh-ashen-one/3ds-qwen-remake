# Physical New Nintendo 3DS acceptance checklist

Compilation and emulator behavior do not satisfy this checklist.

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

Run three consecutive cold-boot playthroughs. For each run record:

| Run | Boot | Interior/reveal | NPC/fog gate | Boss | Restart | Minimum FPS | Peak linear memory | Audio underruns | Result |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 |  |  |  |  |  |  |  |  |  |
| 2 |  |  |  |  |  |  |  |  |  |
| 3 |  |  |  |  |  |  |  |  |  |

Required: no crash, no audio underrun, no sustained performance below 24 FPS, no visible unmasked loading, and at least 20% measured memory headroom at every zone's peak.

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
