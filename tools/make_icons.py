"""Generate the RoarDS placeholder icon set from code.

Draws an original icon (a bold "R" with roar sound-waves on an
orange-to-red rounded tile) and writes every size the build uses:

  res/icon/roards_{16..256}.png   window/taskbar icon, Linux install
  res/roards384.png, roards512.png splash-screen logo
  res/roards.ico                   Windows .exe icon
  res/roards.icns                  macOS bundle icon

Run from the repository root:  python tools/make_icons.py
Replace this with real artwork whenever it exists.
"""
import os
import sys

from PIL import Image, ImageDraw, ImageFont

MASTER = 1024
TOP = (255, 153, 0)      # orange
BOTTOM = (214, 40, 40)   # red
WHITE = (255, 255, 255, 255)


def find_font(size):
    candidates = [
        r"C:\Windows\Fonts\seguibl.ttf",   # Segoe UI Black
        r"C:\Windows\Fonts\arialbd.ttf",
        "/System/Library/Fonts/Supplemental/Arial Bold.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
    ]
    for path in candidates:
        if os.path.exists(path):
            return ImageFont.truetype(path, size)
    sys.exit("no bold TrueType font found; add one to find_font()")


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

    d = ImageDraw.Draw(img)

    # the "R", left of centre
    font = find_font(round(s * 0.62))
    cx, cy = s * 0.40, s * 0.52
    d.text((cx, cy), "R", font=font, fill=WHITE, anchor="mm")

    # three roar waves to the right
    w = round(s * 0.045)
    ox, oy = s * 0.60, s * 0.52
    for i, r in enumerate((0.14, 0.22, 0.30)):
        rr = s * r
        d.arc([ox - rr, oy - rr, ox + rr, oy + rr], start=-40, end=40, fill=WHITE, width=w)

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
