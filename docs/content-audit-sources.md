# Content audit source evidence

The catalogue contains 180 original mod titles: 177 implemented adaptations,
the original intentionally empty **The Void**, and the unavailable source
placeholders **Havoc** and **Onslaught**. The latter two are not missing playable
upgrades. They are absent from the supplied PC game's acquisition pools.

This check used the supplied `NovaDrift.exe` (version 1.2.38.4), SHA-256
`6555f9f49c9b44f2a38dafc0846dd8af0e7e1f1e100cba976db60967b7173e2d`,
and its English localization CSV. The addresses below are executable virtual
addresses. The compact machine-readable observations are in
[`evidence/source-placeholders.json`](evidence/source-placeholders.json).
Only scalar observations and names are recorded; the executable is not included.

## Original descriptions and objects

| Source item | Observed source content | Consequence |
| --- | --- | --- |
| `ModFullHavoc` | Exactly `[$=*]...` | The source supplies no gameplay description. |
| `ModFullOnslaught` | Exactly `[$=*]...` | The source supplies no gameplay description. |
| `gml_Object_mHavoc_Create_0` | `0x1407cb3e0..0x1407cb468`; sets alarm 0 to 1 | Initialization stub. |
| `gml_Object_mHavoc_Alarm_0` | `0x1407cb470..0x1407cb6f2`; registers itself through `gainMod`, then cleans up | No mechanical effect is defined. |
| `gml_Object_mOnslaught_Create_0` | `0x1407d9b60..0x1407d9be8`; sets alarm 0 to 1 | Initialization stub; no named Onslaught alarm routine exists in the native function table. |

The full executable's typed object constants and RIP-relative references were
also checked. Havoc's object ID is 117: its only typed reference is the
self-registration at `0x1407cb540`, inside its own alarm routine. Onslaught's
object ID is 148 and has no typed object reference. Neither has an external
typed reference that could add it to an offer pool.

## Original upgrade pools

`gml_Object_oPowers_Create_0` initializes three wild-mod pools. Havoc and
Onslaught are absent from all three:

| Pool | Initialization address | Original members |
| --- | --- | --- |
| `wildMods` | `0x140c791e2` | Energized Shields, Masochism, Scorching Wake, Specialist, Hypermetabolism, Galvanic Outburst, Slipstream, Heavy Caliber, Double Tap, Polar Inversion, Propulsive Munitions, Solar Heart, Rampage, Defiance |
| `wildModsRare` | `0x140c796b3` | Evolutionary Niche, Bravado, Chaotic Ambition, Obsession, Outrage Module, Phantom Strike, Transmogrification, Death Blossom, Winnow, Revelation, Discord, Spontaneous Generation |
| `wildModsVeryRare` | `0x140c79ae2` | Farsight, Grandeur, Twin Strike, Explosive Growth, Quantum Decoherence, Maelstrom, Ricochet, Challenger |

The native `gml_Script_updateConditionalMods`, `gml_Script_updateModPools` and
`gml_Script_rollWild` routines were checked as well. None adds either placeholder
through a conditional, updated or random wild-mod path. This agrees with the
whole-executable reference check and the placeholder descriptions.

The port therefore retains both original catalogue titles, but excludes them
from purchasable offers. Enabling either as a functional upgrade would require
inventing mechanics not supplied by this version of the original game.

## Scope of the general audit

`tests/content_audit.py` verifies the complete title inventory and reports the
three categories above. Its `adapted` label means that a native mechanic exists;
it does not certify every coefficient or interaction. The implementation uses
extracted constants and recovered native control flow, with bounded adaptations
for calculator memory, timing and geometry. This is not a claim of frame-by-frame
equivalence with the GameMaker runtime.

Native routine inspection can be repeated with `tools/recover_native.py` against
the same executable, selecting the named functions with `--match`. Source text
comparison can be repeated with `tests/content_audit.py --source` pointing to the
original English CSV. Neither check requires distributing the original game.
