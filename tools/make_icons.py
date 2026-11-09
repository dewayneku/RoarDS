"""Generate the RoarDS placeholder icon set from code.

Draws an original icon (a roaring dinosaur head with sound waves on an
orange-to-red rounded tile) and writes every size the build uses:

  res/icon/roards_{16..256}.png   window/taskbar icon, Linux install
  res/roards384.png, roards512.png splash-screen logo
  res/roards.ico                   Windows .exe icon
  res/roards.icns                  macOS bundle icon

Run from the repository root:  python tools/make_icons.py
Replace this with real artwork whenever it exists.
"""
import os

from PIL import Image, ImageChops, ImageDraw

MASTER = 1024
TOP = (255, 153, 0)      # orange
BOTTOM = (214, 40, 40)   # red
WHITE = (255, 255, 255, 255)
DINO = (34, 20, 16, 255)      # near-black brown silhouette
MOUTH = (110, 14, 20, 255)    # dark red inside the mouth
TOOTH = (255, 248, 235, 255)
EYE = (255, 214, 64, 255)


def draw_master():
    s = MASTER
    # vertical gradient
    grad = Image.new("RGBA", (s, s))
    gd = ImageDraw.Draw(grad)
    for y in range(s):
        t = y / (s - 1)
        c = tuple(round(TOP[i] + (BOTTOM[i] - TOP[i]) * t) for i in range(3))
        gd.line([(0, y), (s, y)], fill=c + (255,))

    # rounded-square mask with a small margin
    mask = Image.new("L", (s, s), 0)
    m = round(s * 0.04)
    ImageDraw.Draw(mask).rounded_rectangle([m, m, s - m, s - m], radius=round(s * 0.22), fill=255)
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    img.paste(grad, (0, 0), mask)

    # everything is drawn on a full-size layer and clipped to the tile, so
    # the neck can run off the bottom edge
    art = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(art)

    # head outline in a 1024-unit design space, facing right, jaws open
    SCALE, OX, OY = 0.88, -45, 0
    def P(pts):
        return [(x * SCALE * s / 1024 + OX * s / 1024, y * SCALE * s / 1024 + OY * s / 1024) for x, y in pts]

    head = P([
        (60, 1300), (150, 760), (240, 560), (330, 440),        # back of the neck and head
        (430, 360), (560, 330), (650, 345),                     # skull
        (700, 380), (760, 395), (860, 415), (905, 445),         # brow, snout, nose
        (910, 490), (870, 508),                                 # front of the upper jaw
        (700, 528), (575, 568),                                 # upper jaw underside to mouth corner
        (815, 775), (852, 808), (850, 850),                     # lower jaw, chin
        (765, 872), (600, 840), (470, 825), (430, 1300),        # throat and neck front
    ])
    d.polygon(head, fill=DINO)

    # open mouth
    upper_a, upper_b = (600, 556), (868, 510)
    lower_a, lower_b = (612, 592), (820, 782)
    d.polygon(P([(575, 568), upper_a, upper_b, (885, 505), (836, 795), lower_b, lower_a]), fill=MOUTH)

    # teeth along each jaw, pointing into the mouth
    def teeth(a, b, ts, length, width):
        ax, ay = a
        bx, by = b
        ex, ey = bx - ax, by - ay
        n = (ex * ex + ey * ey) ** 0.5
        ux, uy = ex / n, ey / n
        nx, ny = -uy, ux                       # normal, toward the mouth
        if (nx * (700 - ax) + ny * (650 - ay)) < 0:
            nx, ny = -nx, -ny
        for t, k in ts:
            cx, cy = ax + ex * t, ay + ey * t
            w, l = width * k, length * k
            d.polygon(P([(cx - ux * w, cy - uy * w), (cx + ux * w, cy + uy * w),
                         (cx + nx * l + ux * w * 0.3, cy + ny * l + uy * w * 0.3)]), fill=TOOTH)

    teeth(upper_a, upper_b, [(0.12, 0.8), (0.34, 1.0), (0.56, 1.1), (0.78, 1.2), (0.95, 0.9)], 70, 20)
    teeth(lower_a, lower_b, [(0.18, 0.8), (0.42, 1.0), (0.66, 1.1), (0.88, 1.0)], 62, 18)

    # fierce eye: glowing iris, slit pupil, heavy brow
    ex_, ey_ = P([(640, 425)])[0]
    er = 30 * SCALE * s / 1024
    d.ellipse([ex_ - er, ey_ - er, ex_ + er, ey_ + er], fill=EYE)
    d.ellipse([ex_ - er * 0.28, ey_ - er * 0.85, ex_ + er * 0.28, ey_ + er * 0.85], fill=DINO)
    d.polygon(P([(585, 380), (700, 400), (690, 430), (590, 408)]), fill=DINO)

    # nostril
    nx_, ny_ = P([(870, 440)])[0]
    nr = 11 * SCALE * s / 1024
    d.ellipse([nx_ - nr * 1.4, ny_ - nr, nx_ + nr * 1.4, ny_ + nr], fill=MOUTH)

    # roar waves coming out of the mouth
    ox, oy = P([(880, 650)])[0]
    w = round(s * 0.036)
    for r in (0.09, 0.15, 0.21):
        rr = s * r
        d.arc([ox - rr, oy - rr, ox + rr, oy + rr], start=-38, end=38, fill=WHITE, width=w)

    art.putalpha(ImageChops.multiply(art.getchannel("A"), mask))
    img.alpha_composite(art)

    return img


def main():
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "res")
    root = os.path.normpath(root)
    os.makedirs(os.path.join(root, "icon"), exist_ok=True)

    master = draw_master()

    def sized(n):
        return master.resize((n, n), Image.LANCZOS)

    for n in (16, 32, 48, 64, 128, 192, 256):
        sized(n).save(os.path.join(root, "icon", f"roards_{n}x{n}.png"))
    sized(384).save(os.path.join(root, "roards384.png"))
    sized(512).save(os.path.join(root, "roards512.png"))

    master.save(os.path.join(root, "roards.ico"),
                sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)])
    master.save(os.path.join(root, "roards.icns"))

    print("icons written to", root)


if __name__ == "__main__":
    main()
