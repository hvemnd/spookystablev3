# Spooky Stable v3 "Recess": what changed from v2

v3 adds School mode (the stealth-learning design): the School Bell, four learning toys, a challenge framework with
no fail states, adaptive difficulty, and a Sticker Book. Play mode is v2's, behaviour for behaviour: v2's
`basic.txt`, `stress.txt` and soak run unchanged and pass, with the same simulated times and frame counts as v2.
The platform calls are v1's and v2's exactly; nothing in `base/` is changed. See README.md for the controls and for
every place v3 departs from the design document.

## Manifest

### New files (22 source files, 3 test files, 6 screenshots)

| File | Design § | What it is |
|---|---|---|
| `app/school.h`, `app/school.c` | 2 | the School Bell (OCT- held), routing to the learning toys, direct entry (a toy's button held 1.5 s), the long-press discovery sparkle, the parent's switch (OCT- + K1 at power-on), the apple, School mode's LED rhythm |
| `app/learn.h`, `app/learn.c` | 9.1, 9.2, 9.4 | the challenge framework: pacing (3 free actions, a calm moment, never on a first visit or twice in a row), the no-fail feedback grammar, idle re-demo and quiet abandon, adaptive difficulty over the last 8 tries, mastery moments, a small note scheduler, dot groups |
| `app/keymap.h`, `app/keymap.c` | 10.3 | physical key -> virtual grid, built from plat.h's white and black key lists (no guessed key ids) |
| `app/seq.h`, `app/seq.c` | 4.4 | Poop Parade's 3 x 16 sequencer: BPM and swing clock, sample-accurate in the render, column events back to the screen (`steps_*`, apart from Drum Jam's `seq_*`) |
| `app/fm2.h`, `app/fm2.c` | 5.4 | Fart Mix-Up's two-operator FM on v2's voices (carrier, modulator, the mix <-> mod knob), the cloud pad, the consonance classifier |
| `app/ribbon.h`, `app/ribbon.c` | 5.2 | the wavy ribbon renderer (pitch -> waves, level -> amplitude, sideband zigzag with the ratio's bumps), the paint-like colour mix |
| `app/ladder.h`, `app/ladder.c` | 7 | the 3/5/7-rung pitch ladder: drawing and hit test |
| `app/stickers.h`, `app/stickers.c` | 9.3 | 24 procedural stickers, the book, the new-sticker pop, feeding the wardrobe |
| `app/toys/poop_parade.c` | 4 | Poop Parade: the step sequencer; Copy Me, Count Them, Left to Right |
| `app/toys/fart_mixup.c` | 5 | Fart Mix-Up: two unicorns, FM; Make the Cloud Happy, Copy the Interval, Count the Bumps |
| `app/toys/pee_piano.c` | 7 | Pee Piano: pitch as height; Copy the Tune, Fill the Bucket |
| `app/toys/booger_wall.c` | 6 | Booger Wall: Match (growing walls), Count, Sort |
| `app/tests/school.txt` | 12 | School mode's definition of done (see README, "Test without an FM-1") |
| `app/tests/stress_school.txt` | 12 | every learning toy mashed at full volume; the draw budgets |
| `app/docs/img/school_*.png` | | the four learning toys, the Bell, the Sticker Book |

### Modified files

| File | Change |
|---|---|
| `app/app.c` | the unity build includes the new modules and toys; TOYREG has 12 toys; buttons route through `school_route`; OCT- long = the Bell (its tap's preset step taken back); very long holds (a toy's button 1.5 s, OCT- 2 s); the book takes keys and buttons while open; `synth_key` around a toy's key handler; a toy may take a panel button (`on_button`: the Parade's PLAY); `sched_tick` each frame; School mode's overlays drawn; `app_wardrobe_gift`; host-only: the `answer`, `draws_reset` and `fake_v2` commands and v3's expectations |
| `app/app.h` | `APP_HOST_CMD` (the host's `answer` command) |
| `app/toys.h` | the learning toys' ids, `NTOYS_V2`, `TF_LEARN`, `on_button` and `play_toy` in `toy_def_t`, seven new knob icons |
| `app/knobs.h`, `app/knobs.c` | settings v3 ("SST3": School mode, the parent's switch, the discovery count, each learning toy's difficulty, the stickers, the Parade's pattern); a v2 record ("SST2") migrated field by field; K1 also tells the parent's switch |
| `app/voice.h`, `app/voice.c` | three note fields (extra FM index, feedback, ratio; 0 for every v2 note, so v2's sounds are bit-identical) and two queue commands (`snd_fm`, `snd_vel`) for a held note's FM and level; the render runs the Parade's clock |
| `app/patch.h`, `app/patch.c` | seven patches: PLOP_STEP, FART_CARRIER, FART_MOD, MOD_CLOUD, BELL_STING, SUCCESS_ARP, RETRY_BOING (after v2's, so v2's ids are unchanged) |
| `app/ui.c` | a toy's own key lights (`g_led_lit`, `g_led_mid`), School mode's LED rhythm, the quiet moon and the replay tape move left of the apple |
| `app/draw.h`, `app/draw.c` | `dot3` (a 3-pixel dot on raw rows); host-only shape counters for the draw budgets |
| `app/util.h` | `ss_zero`, `ss_copy`, `ss_fmod` |
| `app/toys/{fart,poop,pee,burp,sneeze,drumjam,toilet,booger}.c` | one line each: the two new `toy_def_t` fields |
| `tests/run_tests.sh` | after v2's checks: `school.txt`, `stress_school.txt` and a 5-minute soak with School mode's gestures |
| `tests/make_soak.py` | `--school`: School mode's gestures; without it the script is v2's, play for play |
| `app/words.tsv`, `app/fm1.toml`, `firmware.json` | the learning toys' names; the description |
| `README.md` | School mode's controls and challenges, the tests, the code map, how v3 departs from its design, what to check on hardware |
| `CHANGES.md` | this file |

Settings use a new magic ("SST3"). A v2 unit keeps everything it knew; School mode's fields start at zero.

---

# Spooky Stable v2: what changed from v1

v1's single `app/app.c` (2,220 lines) is now an app unity build of modules (about 7,400 lines in all); the
platform calls are v1's exactly, and nothing in `base/` is changed. See README.md for the controls and for every
place v2 departs from the design document.

## New files

| File | Design § | What it is |
|---|---|---|
| `app/util.h` | — | random numbers, maths, RGB565 colour helpers, the universal and content colours |
| `app/draw.h`, `app/draw.c` | 2.6, 2.7 | v1's scanline primitives (moved out of app.c) plus capsules, arcs, stars, hearts, the 16×16 glow sprite, silhouettes |
| `app/fx.h`, `app/fx.c` | 2.1–2.3, 2.9, 11 | easing library, trauma shake, hitstop, impact frames, the flash limiter (v1's), rings, six transitions |
| `app/mood.h`, `app/mood.c` | 2.4, 2.5, 3.1 | the 8 moods, scales, `quantize_to_scale()`, the dithered sky column, parallax props (blast / suck / return) |
| `app/particles.h`, `app/particles.c` | 2.7 | the 200-slot particle pool, 19 types, mood-driven defaults |
| `app/patch.h`, `app/patch.c` | 3.4–3.6 | `patch_t`, all 60 patches (reference patches + variants), the 5 drum kits |
| `app/voice.h`, `app/voice.c` | 3.2, 3.3, 6 | 12-voice FM synth chain, stealing with a 5 ms fade, delay/chorus/bitcrush/duck, limiter, the drum sequencer |
| `app/unicorn.h`, `app/unicorn.c` | 2.6 | the one procedural unicorn rig, its secondary motion, 12 accessories, 6 manes |
| `app/toys.h` | 8.1 | `toy_def_t`, the registry's types |
| `app/knobs.h`, `app/knobs.c` | 4 | the knob framework, overlay icons, settings persistence |
| `app/ui.h`, `app/ui.c` | 2.10, 3.7, 7 | LED choreography, chaos balloon, charge, mash, idle (wave / sleep / dream), overlays, replay, dancers |
| `app/toys/common.c` | — | what every toy shares: the unicorn's pose plumbing, impacts, wardrobe, the mini ghost prop |
| `app/toys/fart.c` | 1, 2.1 | Unicorn Fart (ported from v1; v1's timing and cloud kept) |
| `app/toys/poop.c` | 1 | Unicorn Poop (ported from v1; pile, tower dance and giggling ghost kept) |
| `app/toys/pee.c` | 1 | Unicorn Pee (ported from v1; stream, pool, reflection kept) |
| `app/toys/burp.c` | 2.8 | Unicorn Burp (new) |
| `app/toys/sneeze.c` | 2.8 | Unicorn Sneeze (new) |
| `app/toys/drumjam.c` | 6 | Drum Jam with loop recorder and auto-drummer (new) |
| `app/toys/toilet.c` | 2.8 | Toilet Flush (new) |
| `app/toys/booger.c` | 2.8 | Booger Flick (new) |
| `tests/make_soak.py` | 10 | writes an N-minute random-play soak script |
| `CHANGES.md` | — | this file |

## Modified files

| File | Change |
|---|---|
| `app/app.c` | rewritten: now the registry, input (tap / long press), mood landing, toy switching, the frame loop, test hooks |
| `app/app.h` | `APP_DIM_LEVELS` (breathing key LEDs) and `APP_OVERLOAD` (the synth sheds voices if the render runs late) |
| `app/tests/basic.txt`, `app/tests/stress.txt` | rewritten for v2's definition of done |
| `tests/run_tests.sh` | runs basic, stress and a 5-minute soak |
| `app/words.tsv`, `app/fm1.toml`, `firmware.json` | the eight toys' names and the new description |
| `README.md` | v2's controls, code map, and the differences from the design document |
| `setup.sh` | `git submodule add -f`: v1's failed on a fresh unzip because `base/` is in `.gitignore` |
| `.github/workflows/release.yml` | the comment's example tag (v2.0) |
| `app/docs/img/fart.png`, `poop.png`, `pee.png` | new screenshots |

New screenshots: `app/docs/img/burp.png`, `sneeze.png`, `drum.png`, `toilet.png`, `booger.png`, `moods.png`.

## Removed

- The Ghost and Monster toys, their buttons (FX and ENV now open Burp and Sneeze), GLO's surprise glitch and Mega Spook,
  and the K2 animation-speed setting (K2 is each toy's own now). v1's ghost drawing survives only as the tiny
  giggling poop ghost behind `PROPS`.
- `app/docs/img/ghost_oops.png`, `ghosts.png`, `monster_roar.png`, `mega.png`.

Settings use a new magic ("SST2"): a v1 unit starts v2 with defaults.
