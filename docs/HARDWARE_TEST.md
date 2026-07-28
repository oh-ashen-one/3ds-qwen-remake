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
- [x] Confirm Homebrew Launcher can launch Ashen Rift; the SD card passed FAT32 verification before the run.
- [ ] Record the test build commit and SHA-256.

Boot9strap, Luma3DS, and the standard finalization tools were installed using the current official [3DS Hacks Guide](https://3ds.hacks.guide/get-started.html) for this model and firmware. This setup milestone does not satisfy the game-specific physical acceptance gate below.

## Deployment evidence

- [ ] `make run IP=<3DS-IP>` transfers and starts the `.3dsx` through netloader.
- [x] The persistent SD bundle launches with the Mac disconnected.
- [ ] Select returns cleanly to Homebrew Launcher.
- [ ] Sleep/wake resumes without corrupting rendering, input, or audio.

## Current physical blocker

The first CTR-001 run on 2026-07-20 launched from the persistent SD bundle and
rendered both screens, but a large dark slab occluded most of the top-screen
view in the Sunken Vestibule. The slab disappeared after entering the outdoor
zone. Inspection confirmed that the interior camera was above the room's
ceiling and looked through that geometry toward the player. The pending build
keeps that camera inside the room and hides generated columns or trees only
while they cross the camera-to-player sightline. A same-console retest must
rotate the camera through a full orbit beside vestibule columns, vista trees,
and arena columns before any playthrough is counted toward acceptance.

## Playthrough gate

Run three consecutive cold-boot playthroughs using the intended 7:25 beat sheet in [PLAYTHROUGH_ROUTE.md](PLAYTHROUGH_ROUTE.md). For each run record:

| Run | Duration | Boot | Interior/reveal | NPC/fog gate | Boss | Reach/horse | Minimum FPS | Peak linear memory | Audio underruns | Result |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 |  |  |  |  |  |  |  |  |  |  |
| 2 |  |  |  |  |  |  |  |  |  |  |
| 3 |  |  |  |  |  |  |  |  |  |  |

Required: a coherent 5–8 minute sequence, no crash, no audio underrun, no sustained performance below 24 FPS, no visible unmasked loading, and at least 20% measured memory headroom at every zone's peak.

## Edge cases

- [ ] Pause/resume in each zone.
- [ ] Abort and repeat the NPC interaction.
- [ ] Lose lock-on when the boss dies.
- [ ] Die during each boss attack class and return to the correct boss-area grace with full health, stamina, and flasks.
- [ ] Win, cross into the Reach, and confirm the post-Warden resource restoration before a later death or relaunch.
- [ ] Repeat every masked handoff and return path at least five times across sessions.
- [ ] Orbit the camera beside columns/trees/mountains in all six zones with no full-screen obstruction.
- [ ] Mount each of the three horses, gallop, heal while mounted, dismount, recall the selected horse, and reach every field boundary.
- [ ] Ride the river system from its lower bend past the tributary, bridge, and ford to the mountain pass; confirm flowers, cedars, deer, foxes, cranes, clouds, and snowcaps do not cause sustained drops below 24 FPS.
- [ ] Verify the bottom-screen map, objective changes, facing/horse/waystone markers, and ACT/RIDE/DISMOUNT, HEAL, LOCK/CALL, DEBUG touch targets.
- [ ] Confirm Ashen Deep Hall plays in the interior/vista, Ashen Gate plays in all three boss zones, and Valley After Dawn plays in the Reach and lower Cloudbreak climb; every fade must complete without a pop, crash, long gap, or underrun.
- [ ] Complete the extended branch route: hog charge/stomp, defeat persistence, mounted cloud climb, summit dismount, ogre magic telegraph/impact, second defeat persistence, and both return paths.

## Proof recording

- [ ] Uncut shot shows the physical console and both screens.
- [ ] Shot includes Homebrew Launcher, application selection, launch, and gameplay.
- [ ] Do not describe the build as verified real-hardware software until this checklist passes.
