#!/bin/sh
# Spooky Stable v3's checks: v2's, unchanged (Play mode must behave exactly as v2), then School mode's.
# v2's in fm1-base's host simulator: every toy from its button and every key region in it, all
# seven knobs, presets, the eight moods, the loop recorder and replay, cleanup, quiet mode, idling, the wardrobe, the
# settings across a power cycle (basic.txt); then every toy mashed at full volume (the 70 % ceiling), the flash-rate
# limit (never under 400 ms apart), MEGA mashes and voice stealing (stress.txt); and a 5-minute random soak.
# Screenshots go to app/build/host/. A longer soak: python3 tests/make_soak.py 30 > app/build/soak.txt, then run it.
set -e
cd "$(dirname "$0")/.."
python3 base/tools/build.py app --host
H=app/build/host/spooky_host
out=$("$H" app/tests/basic.txt app/build/host 2>&1 || true)
echo "$out" | grep -q "EXPECTATIONS FAILED" && { echo "$out" | grep -v "^  ok"; echo "FAIL basic"; exit 1; }
echo "$out" | tail -1
out=$("$H" app/tests/stress.txt app/build/host 2>&1 || true)
echo "$out" | grep -q "EXPECTATIONS FAILED" && { echo "$out" | grep -v "^  ok"; echo "FAIL stress"; exit 1; }
echo "$out" | grep -q "ok peak" || { echo "$out"; echo "FAIL loudness ceiling"; exit 1; }
echo "$out" | grep "peak"
python3 tests/make_soak.py 5 > app/build/soak5.txt
out=$("$H" app/build/soak5.txt app/build/host 2>&1 || true)
echo "$out" | grep -q "EXPECTATIONS FAILED" && { echo "$out" | grep -v "^  ok"; echo "FAIL soak"; exit 1; }
echo "$out" | tail -1
# v3: School mode: the Bell, the four learning toys and their challenges, pacing, adaptive difficulty, stickers, the
# parent's switch, persistence and v2's settings migrated (school.txt); every learning toy mashed at full volume and
# the draw budgets (stress_school.txt); a 5-minute soak with School mode's gestures in it.
out=$("$H" app/tests/school.txt app/build/host 2>&1 || true)
echo "$out" | grep -q "EXPECTATIONS FAILED" && { echo "$out" | grep -v "^  ok"; echo "FAIL school"; exit 1; }
echo "$out" | tail -1
out=$("$H" app/tests/stress_school.txt app/build/host 2>&1 || true)
echo "$out" | grep -q "EXPECTATIONS FAILED" && { echo "$out" | grep -v "^  ok"; echo "FAIL stress_school"; exit 1; }
echo "$out" | grep -q "ok peak" || { echo "$out"; echo "FAIL loudness ceiling (school)"; exit 1; }
echo "$out" | grep "peak"
python3 tests/make_soak.py 5 --school > app/build/soak5s.txt
out=$("$H" app/build/soak5s.txt app/build/host 2>&1 || true)
echo "$out" | grep -q "EXPECTATIONS FAILED" && { echo "$out" | grep -v "^  ok"; echo "FAIL school soak"; exit 1; }
echo "$out" | tail -1
echo "all checks passed"
