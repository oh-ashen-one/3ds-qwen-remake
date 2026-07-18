# Production playthrough route

This is the intended first-completion and proof-recording route. It targets **6:40**, inside the required 5–8-minute window, without changing simulation speed or hiding edits. The physical acceptance run must record the real duration; this plan is not evidence that the timing already passed.

| Time | Beat | Required visible evidence |
| --- | --- | --- |
| 0:00–0:10 | Title and launch settle | Ashen Rift title, both screens, A prompt |
| 0:10–0:55 | Sunken Vestibule | Third-person movement, C-stick camera, quick-item selection, room silhouette |
| 0:55–1:20 | Door/lift reveal | A interaction, rising door, vista data preloaded behind the mask |
| 1:20–2:35 | Sable Expanse reveal | Panorama, low-poly path, camera sweep, culling counter once |
| 2:35–3:05 | Veiled Keeper | Full original subtitle, abort/retry only on an edge-case run |
| 3:05–3:45 | Pale gate | Gate approach, arena preload, fade, prior-zone unload |
| 3:45–6:20 | Ashen Warden | Name card, lock-on, slash and slam telegraphs, dodge, heal, light/heavy attacks, defeat |
| 6:20–6:40 | Victory and restart | Victory lockout, A restart, return to the opening zone |

## Recording rules

- Use one continuous take for the acceptance proof; no cuts or speed changes.
- Start the timer when the title first appears and stop after the restarted vestibule renders.
- Enable diagnostics briefly in each zone, then turn them off for readability.
- Record the measured duration, minimum sustained FPS, peak linear-memory use, and audio-underrun count in `HARDWARE_TEST.md`.
- If the natural route falls outside 5–8 minutes, adjust content or pacing and retest; do not pad the recording by idling.
