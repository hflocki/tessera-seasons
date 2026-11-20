"""Draws the screensaver's four seasonal night scenes: flat pictures of 1500 x 1000 pixels.

They are dark on purpose: the screensaver stands on the glass all night, so the sky stays deep and only small things
glow (the moon, a window, a lantern). The clock stands in the sky in the upper middle, so that part stays calm.

Every subject sits in the middle 600 pixels of the width, so a screen standing up (which cuts the sides away) still
shows it; what lies outside them is only scenery.

The screen gets them through the Tessera app, which fetches them from this repository's main branch
(raw.githubusercontent.com). Run it again after a change: python3 motifs/draw.py (needs Pillow).
"""
import math
import os
import random

from PIL import Image, ImageDraw, ImageFilter

K = 2                    # drawn at twice the size, then scaled down: smooth edges
W, H = 1500 * K, 1000 * K
OUT = (1500, 1000)
HERE = os.path.dirname(os.path.abspath(__file__))


def k(*v):
    return [int(x * K) for x in v]


def gradient(stops):
    """A sky from top to bottom through (position 0..1, colour) stops."""
    img = Image.new('RGB', (W, H))
    d = ImageDraw.Draw(img)
    for y in range(H):
        t = y / (H - 1)
        for (p0, c0), (p1, c1) in zip(stops, stops[1:]):
            if p0 <= t <= p1:
                f = (t - p0) / (p1 - p0) if p1 > p0 else 0
                d.line([(0, y), (W, y)], fill=tuple(int(a + (b - a) * f) for a, b in zip(c0, c1)))
                break
    return img


def glow(img, x, y, r, color, strength=0.5):
    layer = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    ImageDraw.Draw(layer).ellipse(k(x - r, y - r, x + r, y + r), fill=color + (int(255 * strength),))
    layer = layer.filter(ImageFilter.GaussianBlur(r * K / 2))
    img.paste(layer, (0, 0), layer)


def stars(d, rnd, n, top=0, bottom=550, color=(200, 210, 235)):
    for _ in range(n):
        x, y = rnd.uniform(0, 1500), rnd.uniform(top, bottom)
        r = rnd.choice([1.2, 1.5, 1.8, 2.4])
        a = rnd.uniform(0.35, 1.0)
        c = tuple(int(v * a) for v in color)
        d.ellipse(k(x - r, y - r, x + r, y + r), fill=c)


def moon(img, x, y, r, color=(236, 232, 210), crescent=False):
    glow(img, x, y, r * 2.4, color, 0.18)
    d = ImageDraw.Draw(img)
    if crescent:
        # The lit disc less a disc beside it: a mask, so the sky shows through where the dark side is.
        mask = Image.new('L', (W, H), 0)
        m = ImageDraw.Draw(mask)
        m.ellipse(k(x - r, y - r, x + r, y + r), fill=255)
        m.ellipse(k(x - r + r * 0.55, y - r - r * 0.25, x + r + r * 0.55, y + r - r * 0.25), fill=0)
        img.paste(Image.new('RGB', (W, H), color), (0, 0), mask)
    else:
        d.ellipse(k(x - r, y - r, x + r, y + r), fill=color)
        shade = tuple(int(v * 0.88) for v in color)
        for cx, cy, cr in [(-0.3, -0.2, 0.18), (0.25, 0.3, 0.13), (0.1, -0.45, 0.09)]:
            d.ellipse(k(x + cx * r - cr * r, y + cy * r - cr * r, x + cx * r + cr * r, y + cy * r + cr * r),
                      fill=shade)


def hill(d, y0, amp, phase, color, waves=1.3):
    pts = [(0, H)]
    for i in range(0, 1501, 10):
        pts.append((i * K, (y0 + amp * math.sin(i / 1500 * math.pi * waves + phase)) * K))
    pts.append((W, H))
    d.polygon(pts, fill=color)


def pine(d, x, base, h, color, snow=None):
    w = h * 0.42
    for i in range(3):
        top = base - h + i * h * 0.24
        bot = base - h * 0.18 + i * h * 0.02 - (2 - i) * h * 0.18
        half = w * (0.55 + i * 0.22)
        d.polygon([(x * K, top * K), ((x - half) * K, bot * K), ((x + half) * K, bot * K)], fill=color)
        if snow:
            d.polygon([(x * K, top * K), ((x - half * 0.45) * K, (top + (bot - top) * 0.42) * K),
                       ((x + half * 0.45) * K, (top + (bot - top) * 0.42) * K)], fill=snow)
    d.rectangle(k(x - h * 0.04, base - h * 0.2, x + h * 0.04, base), fill=(40, 30, 28))


def save(img, name):
    img = img.resize(OUT, Image.LANCZOS)
    img.save(os.path.join(HERE, name + '.png'), optimize=True)


def winter():
    rnd = random.Random(1)
    sky = [(0.0, (6, 12, 30)), (0.6, (18, 32, 62)), (1.0, (30, 48, 82))]
    img = gradient(sky)
    d = ImageDraw.Draw(img)
    stars(d, rnd, 160)
    moon(img, 1290, 130, 50)
    d = ImageDraw.Draw(img)
    hill(d, 740, 30, 0.6, (92, 110, 142), 1.1)
    for x, h in [(140, 210), (250, 260), (1260, 240), (1380, 200), (360, 170)]:
        pine(d, x, 760, h, (14, 30, 36), (120, 138, 168))
    hill(d, 820, 26, 2.2, (128, 146, 176), 1.6)
    # A cabin with a warm window, left of the middle.
    cx, cy = 560, 800
    d.rectangle(k(cx - 90, cy - 110, cx + 90, cy), fill=(52, 34, 30))
    d.polygon(k(cx - 115, cy - 105, cx, cy - 190, cx + 115, cy - 105), fill=(40, 26, 24))
    d.polygon(k(cx - 120, cy - 100, cx, cy - 196, cx + 120, cy - 100, cx + 100, cy - 112, cx, cy - 176, cx - 100, cy - 112),
              fill=(170, 186, 210))
    d.rectangle(k(cx + 45, cy - 200, cx + 70, cy - 150), fill=(46, 30, 28))
    glow(img, cx - 30, cy - 55, 70, (255, 190, 90), 0.35)
    d = ImageDraw.Draw(img)
    d.rectangle(k(cx - 60, cy - 80, cx - 10, cy - 35), fill=(255, 196, 102))
    d.line(k(cx - 35, cy - 80, cx - 35, cy - 35), fill=(52, 34, 30), width=4 * K)
    d.line(k(cx - 60, cy - 58, cx - 10, cy - 58), fill=(52, 34, 30), width=4 * K)
    d.rectangle(k(cx + 25, cy - 75, cx + 65, cy), fill=(30, 20, 18))
    # A snowman right of the middle.
    sx, sy = 930, 850
    snow_c, line_c = (160, 176, 204), (120, 136, 166)
    for r, y in [(78, sy), (56, sy - 115), (40, sy - 198)]:
        d.ellipse(k(sx - r, y - r, sx + r, y + r), fill=snow_c, outline=line_c, width=2 * K)
    d.rectangle(k(sx - 32, sy - 268, sx + 32, sy - 228), fill=(18, 18, 26))
    d.rectangle(k(sx - 46, sy - 232, sx + 46, sy - 222), fill=(18, 18, 26))
    d.rounded_rectangle(k(sx - 48, sy - 172, sx + 48, sy - 156), 6 * K, fill=(150, 40, 46))
    d.rectangle(k(sx + 18, sy - 166, sx + 34, sy - 120), fill=(150, 40, 46))
    for ex in (-14, 14):
        d.ellipse(k(sx + ex - 5, sy - 210, sx + ex + 5, sy - 200), fill=(20, 20, 26))
    d.polygon(k(sx, sy - 196, sx + 38, sy - 190, sx, sy - 184), fill=(210, 110, 40))
    for by in (-130, -105, -80):
        d.ellipse(k(sx - 5, sy + by - 5, sx + 5, sy + by + 5), fill=(20, 20, 26))
    d.line(k(sx - 52, sy - 120, sx - 120, sy - 170), fill=(60, 42, 34), width=6 * K)
    d.line(k(sx + 52, sy - 120, sx + 120, sy - 165), fill=(60, 42, 34), width=6 * K)
    hill(d, 930, 14, 1.0, (150, 166, 194), 2.0)
    save(img, 'winter')


def spring():
    rnd = random.Random(2)
    img = gradient([(0.0, (8, 14, 30)), (0.55, (14, 26, 46)), (1.0, (26, 52, 58))])
    d = ImageDraw.Draw(img)
    stars(d, rnd, 120)
    moon(img, 1290, 140, 48, crescent=True)
    d = ImageDraw.Draw(img)
    hill(d, 760, 34, 1.4, (22, 52, 40), 1.2)
    hill(d, 830, 22, 0.2, (30, 70, 46), 1.8)
    # A blossoming tree left of the middle.
    tx, ty = 600, 840
    d.polygon(k(tx - 22, ty, tx + 22, ty, tx + 12, ty - 190, tx - 12, ty - 190), fill=(46, 32, 30))
    for a, l in [(-60, 150), (-20, 170), (25, 160), (60, 140)]:
        ex = tx + l * math.sin(math.radians(a))
        ey = ty - 180 - l * math.cos(math.radians(a)) * 0.55
        d.line(k(tx, ty - 180, ex, ey), fill=(46, 32, 30), width=10 * K)
    for _ in range(170):
        ang = rnd.uniform(0, 2 * math.pi)
        rr = math.sqrt(rnd.random())
        x = tx + math.cos(ang) * 200 * rr
        y = ty - 260 + math.sin(ang) * 100 * rr
        r = rnd.uniform(14, 26)
        c = rnd.choice([(214, 120, 158), (190, 96, 136), (230, 150, 180), (168, 82, 124)])
        d.ellipse(k(x - r, y - r, x + r, y + r), fill=c)
    # Tulips in the meadow, right of the middle.
    for i, (x, c) in enumerate([(830, (200, 60, 80)), (890, (220, 170, 60)), (950, (170, 90, 190)),
                                (1010, (230, 110, 60)), (1070, (210, 80, 130))]):
        top = 790 - (i % 2) * 30
        d.line(k(x, 900, x, top), fill=(40, 96, 56), width=6 * K)
        d.ellipse(k(x - 26, 880 - 50, x - 4, 880), fill=(40, 96, 56))
        d.pieslice(k(x - 22, top - 26, x + 22, top + 18), 0, 180, fill=c)
        d.polygon(k(x - 22, top - 4, x - 14, top - 34, x - 4, top - 10, x, top - 36, x + 4, top - 10, x + 14, top - 34,
                    x + 22, top - 4), fill=c)
    hill(d, 920, 14, 2.4, (24, 60, 40), 2.2)
    for _ in range(60):
        x, y = rnd.uniform(0, 1500), rnd.uniform(900, 1000)
        r = rnd.uniform(3, 6)
        d.ellipse(k(x - r, y - r, x + r, y + r), fill=rnd.choice([(230, 230, 240), (240, 210, 90), (220, 140, 180)]))
    save(img, 'spring')


def summer():
    rnd = random.Random(3)
    img = gradient([(0.0, (10, 10, 34)), (0.42, (36, 26, 72)), (0.62, (120, 64, 92)), (0.68, (170, 92, 80)),
                    (0.69, (24, 30, 58)), (1.0, (14, 18, 40))])
    d = ImageDraw.Draw(img)
    stars(d, rnd, 110, bottom=450)
    moon(img, 1280, 150, 44)
    d = ImageDraw.Draw(img)
    # The moon's path on the sea.
    for i in range(18):
        y = 700 + i * 14
        w = 30 + i * 6 + rnd.uniform(-10, 10)
        a = 1 - i / 20
        d.rounded_rectangle(k(1280 - w, y, 1280 + w, y + 4), 2 * K,
                            fill=tuple(int(v * a) for v in (200, 196, 170)))
    for i in range(40):
        x, y = rnd.uniform(0, 1500), rnd.uniform(705, 860)
        d.line(k(x, y, x + rnd.uniform(20, 50), y), fill=(44, 52, 88), width=2 * K)
    # A dune with grass, and a lighthouse left of the middle.
    pts = [(0, 1000)] + [(i, 860 - 70 * math.exp(-((i - 520) / 380) ** 2)) for i in range(0, 1501, 10)] + [(1500, 1000)]
    d.polygon([(x * K, y * K) for x, y in pts], fill=(58, 46, 52))
    for _ in range(70):
        x = rnd.uniform(200, 900)
        base = 860 - 70 * math.exp(-((x - 520) / 380) ** 2) + 4
        d.line(k(x, base, x + rnd.uniform(-14, 14), base - rnd.uniform(20, 46)), fill=(52, 70, 50), width=3 * K)
    lx, lb = 640, 800
    d.polygon(k(lx - 34, lb, lx + 34, lb, lx + 20, lb - 280, lx - 20, lb - 280), fill=(200, 196, 200))
    for i, band in enumerate(range(4)):
        y0 = lb - 30 - i * 70
        w0 = 34 - (30 + i * 70) / 280 * 14
        w1 = 34 - (70 + i * 70) / 280 * 14
        d.polygon(k(lx - w0, y0, lx + w0, y0, lx + w1, y0 - 34, lx - w1, y0 - 34), fill=(160, 44, 50))
    glow(img, lx, lb - 305, 80, (255, 220, 140), 0.45)
    d = ImageDraw.Draw(img)
    d.rectangle(k(lx - 22, lb - 320, lx + 22, lb - 280), fill=(255, 226, 150))
    d.polygon(k(lx - 30, lb - 320, lx, lb - 352, lx + 30, lb - 320), fill=(40, 34, 40))
    d.rectangle(k(lx - 34, lb - 284, lx + 34, lb - 276), fill=(40, 34, 40))
    # A faint beam to the right.
    beam = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    ImageDraw.Draw(beam).polygon(k(lx + 22, lb - 310, 1500, lb - 420, 1500, lb - 270), fill=(255, 226, 150, 34))
    beam = beam.filter(ImageFilter.GaussianBlur(8 * K))
    img.paste(beam, (0, 0), beam)
    save(img, 'summer')


def pumpkin(img, x, y, r, lit=True):
    d = ImageDraw.Draw(img)
    if lit:
        glow(img, x, y, r * 1.9, (255, 140, 40), 0.32)
        d = ImageDraw.Draw(img)
    body, rib = (196, 90, 22), (150, 64, 16)
    for dx in (-0.55, 0.55, -0.28, 0.28, 0):
        d.ellipse(k(x + dx * r - r * 0.55, y - r * 0.8, x + dx * r + r * 0.55, y + r * 0.8), fill=body, outline=rib,
                  width=2 * K)
    d.rounded_rectangle(k(x - r * 0.08, y - r * 1.05, x + r * 0.08, y - r * 0.72), 4 * K, fill=(70, 90, 40))
    if lit:
        face = (255, 214, 110)
        for ex in (-0.35, 0.35):
            d.polygon(k(x + ex * r - r * 0.16, y - r * 0.12, x + ex * r, y - r * 0.38, x + ex * r + r * 0.16,
                        y - r * 0.12), fill=face)
        d.polygon(k(x - r * 0.06, y + r * 0.02, x, y - r * 0.12, x + r * 0.06, y + r * 0.02), fill=face)
        pts = [(-0.55, 0.22), (-0.38, 0.32), (-0.3, 0.22), (-0.15, 0.36), (0, 0.24), (0.15, 0.36), (0.3, 0.22),
               (0.38, 0.32), (0.55, 0.22), (0.35, 0.52), (0, 0.6), (-0.35, 0.52)]
        d.polygon([((x + px * r) * K, (y + py * r) * K) for px, py in pts], fill=face)


def autumn():
    rnd = random.Random(4)
    img = gradient([(0.0, (14, 8, 24)), (0.55, (36, 18, 40)), (1.0, (62, 30, 34))])
    d = ImageDraw.Draw(img)
    stars(d, rnd, 90, bottom=420)
    moon(img, 1260, 190, 66, color=(232, 170, 90))
    d = ImageDraw.Draw(img)
    # A bare tree at the left edge of the middle.
    tx, ty = 470, 860

    def branch(x, y, ang, length, width, depth):
        if depth == 0 or length < 12:
            return
        ex = x + length * math.sin(math.radians(ang))
        ey = y - length * math.cos(math.radians(ang))
        d.line(k(x, y, ex, ey), fill=(22, 14, 18), width=max(2, int(width)) * K)
        for da in (-rnd.uniform(18, 34), rnd.uniform(18, 34)):
            branch(ex, ey, ang + da, length * rnd.uniform(0.64, 0.78), width * 0.66, depth - 1)

    branch(tx, ty, -4, 190, 30, 7)
    hill(d, 830, 24, 1.0, (44, 24, 22), 1.3)
    hill(d, 880, 16, 2.6, (58, 32, 26), 2.0)
    # Leaves on the ground.
    for _ in range(140):
        x, y = rnd.uniform(0, 1500), rnd.uniform(880, 1000)
        r = rnd.uniform(7, 13)
        c = rnd.choice([(150, 62, 24), (170, 96, 30), (120, 40, 30), (160, 120, 40)])
        d.ellipse(k(x - r, y - r * 0.55, x + r, y + r * 0.55), fill=c)
    # Three pumpkins, the lit ones glowing.
    pumpkin(img, 760, 860, 78)
    pumpkin(img, 900, 890, 54)
    pumpkin(img, 1010, 905, 40, lit=False)
    save(img, 'autumn')


if __name__ == '__main__':
    for draw in (winter, spring, summer, autumn):
        draw()
    print('motifs drawn')
