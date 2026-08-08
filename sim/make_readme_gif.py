#!/usr/bin/env python3
"""Render an LED-panel-look animated GIF from simulator PPM frames.

This is what produces the animation in the top-level README. It exists so that
image can be regenerated after a layout change instead of going stale - the
photo it replaced was inherited from upstream and no longer matched the panel.

    make readme-gif                    # 128x64, the README image
    make readme-gif VARIANT=64x64      # the square clock

Unlike render.py's --led mode (which draws one ellipse per pixel in Python and
is far too slow for a 50+ frame animation), this composites a precomputed dot
mask over an upscaled frame with numpy, then adds a blurred bloom pass.
"""

import argparse
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

# One LED per source pixel. SCALE is the on-screen size of each LED in px.
SCALE = 5
# Diameter of the lit dot within its cell, as a fraction of SCALE. Leaving a gap
# is what reads as "discrete LEDs" rather than a blurry upscale.
DOT_RATIO = 0.72
# How much of the blurred copy to add back as glow.
BLOOM_STRENGTH = 0.55
BLOOM_RADIUS = 2.2


def dot_mask(scale: int, ratio: float) -> np.ndarray:
    """A single LED cell: 1.0 inside the dot, feathered at the rim, 0.0 outside."""
    yy, xx = np.mgrid[0:scale, 0:scale]
    cx = cy = (scale - 1) / 2.0
    dist = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2)
    radius = (scale * ratio) / 2.0
    # 1px feather so the dots don't alias into squares at small scales.
    return np.clip(radius + 0.5 - dist, 0.0, 1.0)


def led_frame(src: Image.Image, mask_tile: np.ndarray, scale: int) -> Image.Image:
    arr = np.asarray(src.convert("RGB"), dtype=np.float32)
    h, w, _ = arr.shape

    # Nearest-neighbour upscale, then knock out the inter-LED gaps.
    big = np.repeat(np.repeat(arr, scale, axis=0), scale, axis=1)
    mask = np.tile(mask_tile, (h, w))[:, :, None]
    dots = big * mask

    # Bloom from the un-masked upscale so glow bleeds into the gaps between LEDs.
    bloom = Image.fromarray(big.astype(np.uint8)).filter(
        ImageFilter.GaussianBlur(radius=BLOOM_RADIUS)
    )
    out = dots + np.asarray(bloom, dtype=np.float32) * BLOOM_STRENGTH
    return Image.fromarray(np.clip(out, 0, 255).astype(np.uint8))


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("frame_dir", type=Path, help="directory of numbered .ppm frames")
    ap.add_argument("output", type=Path, help="output .gif path")
    ap.add_argument("--scale", type=int, default=SCALE)
    ap.add_argument("--duration", type=int, default=70, help="ms per frame")
    ap.add_argument("--colors", type=int, default=128, help="GIF palette size")
    ap.add_argument("--hold", type=int, default=12,
                    help="extra copies of the final frame, so the loop pauses on the settled clock")
    args = ap.parse_args()

    ppms = sorted(args.frame_dir.glob("*.ppm"))
    if not ppms:
        raise SystemExit(f"no PPM frames found in {args.frame_dir}")

    tile = dot_mask(args.scale, DOT_RATIO)
    frames = []
    for p in ppms:
        with Image.open(p) as im:
            frames.append(led_frame(im, tile, args.scale))

    frames.extend([frames[-1]] * args.hold)

    # Quantize every frame against one shared palette, so the GIF doesn't carry a
    # local colour table per frame (which is most of the file size otherwise).
    palette_src = frames[0].quantize(colors=args.colors, method=Image.MEDIANCUT)
    quantized = [f.quantize(palette=palette_src, dither=Image.Dither.NONE) for f in frames]

    args.output.parent.mkdir(parents=True, exist_ok=True)
    quantized[0].save(
        args.output,
        save_all=True,
        append_images=quantized[1:],
        duration=args.duration,
        loop=0,
        optimize=True,
        # disposal=1 (leave the previous frame in place), NOT 2 ("restore to
        # background"). Every frame here is full-size and opaque, so 2 buys
        # nothing - and it paints the GIF background colour into any pixel the
        # encoder skips, which showed up as a yellow bar down the right edge and
        # made the file about 8x bigger.
        disposal=1,
    )
    size = args.output.stat().st_size
    print(f"wrote {args.output} - {len(quantized)} frames, "
          f"{quantized[0].width}x{quantized[0].height}, {size/1024:.0f} KB")


if __name__ == "__main__":
    main()
