# Asset provenance

Ashen Rift uses no extracted commercial assets.

| Asset | Source | Provenance |
| --- | --- | --- |
| Low-poly environment and characters | Procedural boxes in `source/renderer.cpp` | Original code and composition |
| Ambient loop | `tools/generate_original_assets.py` | Original deterministic synthesis |
| Combat hit sound | Runtime synthesis in `source/audio_streamer.cpp` | Original deterministic synthesis |
| Veiled Keeper dialogue | `romfs/dialogue/keeper.txt` | Original writing |
| UI | citro2d primitives and 3DS system font | Runtime primitives; no bundled font asset |

Every future asset must be recorded in `assets/manifest.json` with a source, provenance statement, allowed license, expected output, and size budget before entering RomFS.
