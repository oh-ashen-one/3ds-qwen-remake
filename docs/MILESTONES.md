# Development and footage log

Record real builds while they exist. Do not reconstruct fake broken-build footage later.

| Milestone | Build/commit | What changed | Honest visible problems | Metrics | Capture request |
| --- | --- | --- | --- | --- | --- |
| M0 Toolchain probe | local source toolchain | devkitARM detected; official 3DS libraries/tools built into a user-writable prefix because system install needs an administrator password | System-wide packages still pending | libctru/citro3d/citro2d/3dstools/picasso/3dslink built | Terminal/toolchain establishing shot |
| M1 Native foundation | `2b210e1` | Core state machines, procedural renderer, dual-screen UI, audio and tests | Awaiting physical rendering validation | Host tests and native cross-build pass | First boot, malformed poses, camera issues |
| M2 Connected slice | `2b210e1` | Interior reveal, vista NPC and fog transition | Visual handoff timing is unverified on device | Zone draw/memory counters implemented | Door handoff and outdoor reveal |
| M3 Boss | `2b210e1` | Lock-on, stamina combat and Ashen Warden | Hit readability and performance are unverified on device | CI run `29655685968` passed | Boss behaviors and failures |
| M4 Hardware candidate | pending | Optimization and acceptance build | Must be recorded honestly | Three-run report | Uncut Homebrew Launcher-to-game proof |
