#!/usr/bin/env python3
"""Draws art/tile.png, the 270x110 picture MPC's Sounds > INSTRUMENTS browser shows for DINXXL (the key rows of the panel)."""
import os

from PIL import Image, ImageDraw, ImageEnhance, ImageFont

ART = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "art")
bg = Image.open(os.path.join(ART, "bg_keys.png")).convert("RGB")
crop = bg.crop((20, 340, 810, 580)).resize((270, 82), Image.LANCZOS)
im = Image.new("RGB", (270, 110), (28, 28, 28))
im.paste(ImageEnhance.Brightness(crop).enhance(0.9), (0, 28))
d = ImageDraw.Draw(im)
lamp = Image.open(os.path.join(ART, "lamp_on.png")).convert("RGBA").resize((9, 9), Image.LANCZOS)
for i in range(10):
    im.paste(lamp, (10 + i * 26, 44), lamp)
d.rectangle([0, 0, 269, 109], outline=(120, 120, 120), width=2)
f = ImageFont.truetype(os.path.join(ART, "fonts", "ChakraPetch-SemiBold.ttf"), 26)
f2 = ImageFont.truetype(os.path.join(ART, "fonts", "ChakraPetch-Medium.ttf"), 14)
d.text((12, 16), "DINXXL", font=f, fill=(233, 233, 230), anchor="lm")
d.text((258, 16), "L'Cronx", font=f2, fill=(26, 180, 99), anchor="rm")
im.save(os.path.join(ART, "tile.png"))
print("tile: art/tile.png")
