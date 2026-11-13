#!/usr/bin/env python3
# Spooky Stable v2's soak test: writes a host-simulator script of N minutes of random toddler play (every toy, keys
# tapped, held and mashed, every knob, moods, presets, PLAY, REC, cleanups, quiet mode), with checks along the way.
#   python3 tests/make_soak.py 30 > app/build/soak.txt && app/build/host/spooky_host app/build/soak.txt app/build/host
# v3: --school adds School mode's gestures (the Bell, a toy's button held 1.5 s, the Sticker Book, the Challenge key
# and right and wrong answers); without it the script is v2's, play for play.
import random, sys
school = "--school" in sys.argv
args = [a for a in sys.argv[1:] if a != "--school"]
minutes = float(args[0]) if args else 30
random.seed(7)
BTN = ["HOME", "EDIT", "LFO", "FX", "ENV", "GLO", "ARP", "SEQ"]
ENC = ["K1", "K2", "K3", "K4", "SELECT", "ALGO", "PRESET"]
out = ["# soak: %g simulated minutes of random play" % minutes, "wait 700", "master 4096", "peakreset"]
t, total, n = 0, minutes * 60000, 0
while t < total:
    r = random.random()
    if r < 0.45:
        out += ["tapkey %d %d" % (random.randrange(27), random.randint(20, 127)), "wait %d" % random.randint(10, 300)]
        t += 100
    elif r < 0.55:
        k = random.randrange(27)
        h = random.randint(100, 2500)
        out += ["key %d down 100 40" % k, "wait %d" % h, "key %d up" % k]
        t += h
    elif r < 0.63:
        a, b = random.randrange(16), random.randrange(16)
        out += ["strum %d %d %d" % (a, b, random.randint(8, 40))]
        t += 300
    elif r < 0.70:
        out += ["tap %s" % random.choice(BTN), "wait 300"]
        t += 300
    elif r < 0.80:
        out += ["turn %s %d 20" % (random.choice(ENC), random.choice([-3, -2, -1, 1, 2, 3]))]
        t += 60
    elif r < 0.85:
        out += ["tap %s" % random.choice(["OCT-", "OCT+", "PLAY", "REC", "SEL", "SAVE"]), "wait 200"]
        t += 200
    elif r < 0.88:
        b = random.choice(BTN + ["REC", "OCT+", "SEL"])
        out += ["press %s" % b, "wait 900", "release %s" % b]
        t += 900
    elif school and r < 0.895:
        q = random.random()
        if q < 0.25:
            out += ["press OCT-", "wait 800", "release OCT-", "wait 300"]
        elif q < 0.45:
            b = random.choice(["HOME", "EDIT", "LFO", "SEQ"])
            out += ["press %s" % b, "wait 1600", "release %s" % b, "wait 300"]
        elif q < 0.55:
            out += ["press OCT-", "wait 2100", "release OCT-", "wait 400", "tapkey 3", "wait 200", "tap HOME", "wait 300"]
        elif q < 0.75:
            out += ["tapkey 26", "wait %d" % random.randint(1500, 4500)]
        else:
            out += ["answer%s" % random.choice(["", " wrong"]), "wait %d" % random.randint(1000, 5000)]
        t += 2500
    elif r < 0.90:
        w = random.randint(3000, 15000)
        out += ["wait %d" % w]
        t += w
    else:
        out += ["wait %d" % random.randint(200, 1500)]
        t += 800
    n += 1
    if n % 200 == 0:
        out += ["expect voices <13", "expect particles <201", "expect flash_gap >399"]
        if school:
            out += ["expect sched <25"]
out += ["wait 3000", "tap PLAY", "wait 10000", "expect peak_db_max <-3.0", "expect flash_gap >399"]
print("\n".join(out))
