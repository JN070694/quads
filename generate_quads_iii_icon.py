#!/usr/bin/env python3
"""
Quads-II icon generator (Ultra-Luxe / art-deco pearl-on-black style)
Requires: Pillow (pip install pillow)
Requires: DejaVu Serif Bold font at the path below (adjust if needed)
Output: 512x512 PNG, rendered at 2x and downsampled for crisp anti-aliasing
"""

from PIL import Image, ImageDraw, ImageFont

SCALE = 2
size = 512 * SCALE
img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
d = ImageDraw.Draw(img)

BLACK = (8, 8, 10, 255)
PEARL = (231, 226, 212, 255)
RED   = (176, 30, 40, 255)

# ---- Black rounded square with double pearl border ----
radius = 70 * SCALE
d.rounded_rectangle([0, 0, size - 1, size - 1], radius=radius, fill=BLACK)

outer_pad = 22 * SCALE
d.rounded_rectangle(
    [outer_pad, outer_pad, size - 1 - outer_pad, size - 1 - outer_pad],
    radius=radius - outer_pad // 2, outline=PEARL, width=3 * SCALE
)
inner_pad = 34 * SCALE
d.rounded_rectangle(
    [inner_pad, inner_pad, size - 1 - inner_pad, size - 1 - inner_pad],
    radius=radius - inner_pad // 2, outline=PEARL, width=2 * SCALE
)

# ---- Title: letter-spaced small-caps "QUADS-II" ----
font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf"
text = "QUADS-III"
fsize = 46 * SCALE
font = ImageFont.truetype(font_path, fsize)
letter_spacing = 8 * SCALE

glyph_info = []
for ch in text:
    bbox = d.textbbox((0, 0), ch, font=font)
    glyph_info.append((ch, bbox[0], bbox[2] - bbox[0]))
total_w = sum(w for _, _, w in glyph_info) + letter_spacing * (len(text) - 1)

start_x = (size - total_w) / 2
y = 95 * SCALE

x = start_x
tops = []
baselines = []  # bottoms of all glyphs EXCEPT Q's descender swash
for ch, bearing, w in glyph_info:
    draw_x = x - bearing
    d.text((draw_x, y), ch, font=font, fill=PEARL)
    gb = d.textbbox((draw_x, y), ch, font=font)
    tops.append(gb[1])
    if ch != 'Q':
        baselines.append(gb[3])
    x += w + letter_spacing

visual_top = min(tops)
visual_baseline = max(baselines)  # true baseline, ignoring Q's tail

# flourish lines: identical gap above visual_top and below visual_baseline
gap = 24 * SCALE
line_y_top = visual_top - gap
line_y_bot = visual_baseline + gap
line_half = 62 * SCALE
lgap = 14 * SCALE
cx = size / 2
lw = 2 * SCALE
for ly in (line_y_top, line_y_bot):
    d.line([(cx - line_half, ly), (cx - lgap, ly)], fill=PEARL, width=lw)
    d.line([(cx + lgap, ly), (cx + line_half, ly)], fill=PEARL, width=lw)
    dsz = 5 * SCALE
    d.polygon([(cx, ly - dsz), (cx + dsz, ly), (cx, ly + dsz), (cx - dsz, ly)], fill=PEARL)


# ---- Two fanned Ace of Diamonds cards ----
def make_card(w, h, angle, corner_r):
    card = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    cd = ImageDraw.Draw(card)
    cd.rounded_rectangle([0, 0, w - 1, h - 1], radius=corner_r,
                          fill=PEARL, outline=(25, 25, 25, 255), width=3 * SCALE)

    def diamond(dcx, dcy, s, aspect=0.6):
        cd.polygon([(dcx, dcy - s), (dcx + s * aspect, dcy),
                    (dcx, dcy + s), (dcx - s * aspect, dcy)], fill=RED)

    af = ImageFont.truetype(font_path, int(h * 0.155))
    a_x, a_y = w * 0.12, h * 0.05
    cd.text((a_x, a_y), "A", font=af, fill=RED)
    a_bbox = cd.textbbox((a_x, a_y), "A", font=af)
    a_center_x = (a_bbox[0] + a_bbox[2]) / 2  # true glyph center, for the pip below it

    pip_cy = h * 0.05 + h * 0.155 * 1.05 + h * 0.045
    diamond(a_center_x, pip_cy, s=h * 0.030)

    diamond(w * 0.5, h * 0.5, s=h * 0.165)  # big diamond, true card center

    return card.rotate(angle, expand=True, resample=Image.BICUBIC)


card_w, card_h = 167 * SCALE, 235 * SCALE
corner_r = 18 * SCALE

card_back = make_card(card_w, card_h, angle=10, corner_r=corner_r)
card_front = make_card(card_w, card_h, angle=-8, corner_r=corner_r)

cy_center = 340 * SCALE
cx_center = size // 2

bx = cx_center - card_back.width // 2 + 20 * SCALE
by = cy_center - card_back.height // 2 - 16 * SCALE  # tucked up, avoids edge-crossing smudge
img.paste(card_back, (bx, by), card_back)

fx = cx_center - card_front.width // 2 - 20 * SCALE
fy = cy_center - card_front.height // 2 + 6 * SCALE
img.paste(card_front, (fx, fy), card_front)

img = img.resize((512, 512), Image.LANCZOS)
img.save("Quads-III-icon.png")
print("Saved Quads-III-icon.png")
