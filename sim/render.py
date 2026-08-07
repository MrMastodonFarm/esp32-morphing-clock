#!/usr/bin/env python3
import argparse
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter


SCALE = 6


def flat_render(ppm_path: Path) -> Path:
    png_path = ppm_path.with_suffix(".png")
    with Image.open(ppm_path) as image:
        enlarged = image.convert("RGB").resize(
            (image.width * SCALE, image.height * SCALE),
            resample=Image.Resampling.NEAREST,
        )
        enlarged.save(png_path)
    print(f"wrote {png_path}")
    return png_path


def led_render(ppm_path: Path) -> Path:
    png_path = ppm_path.with_name(f"{ppm_path.stem}-led.png")
    with Image.open(ppm_path) as source:
        source = source.convert("RGB")
        size = (source.width * SCALE, source.height * SCALE)
        dots = Image.new("RGB", size, (2, 3, 5))
        bloom_seed = Image.new("RGB", size, (0, 0, 0))
        dot_draw = ImageDraw.Draw(dots)
        bloom_draw = ImageDraw.Draw(bloom_seed)

        for y in range(source.height):
            for x in range(source.width):
                red, green, blue = source.getpixel((x, y))
                if red == 0 and green == 0 and blue == 0:
                    continue
                left = x * SCALE + 1
                top = y * SCALE + 1
                right = left + SCALE - 3
                bottom = top + SCALE - 3
                dot_draw.ellipse((left, top, right, bottom), fill=(red, green, blue))
                bloom_draw.ellipse(
                    (left - 1, top - 1, right + 1, bottom + 1),
                    fill=(red // 3, green // 3, blue // 3),
                )

        bloom = bloom_seed.filter(ImageFilter.GaussianBlur(radius=2.0))
        ImageChops.add(dots, bloom).save(png_path)
    print(f"wrote {png_path}")
    return png_path


def ppm_files(path: Path):
    return sorted(path.rglob("*.ppm"))


def make_gif(frame_dir: Path, output_path: Path) -> None:
    frames = []
    for ppm_path in ppm_files(frame_dir):
        with Image.open(ppm_path) as image:
            frames.append(
                image.convert("RGB").resize(
                    (image.width * SCALE, image.height * SCALE),
                    resample=Image.Resampling.NEAREST,
                )
            )
    if not frames:
        raise SystemExit(f"no PPM frames found under {frame_dir}")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    frames[0].save(
        output_path,
        save_all=True,
        append_images=frames[1:],
        duration=60,
        loop=0,
        disposal=2,
    )
    print(f"wrote {output_path} ({len(frames)} frames)")


def parse_args():
    parser = argparse.ArgumentParser(description="Render simulator PPM output")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--gif", nargs=2, metavar=("DIR", "OUTPUT"))
    group.add_argument(
        "--led",
        nargs="*",
        metavar="PPM",
        help="render specified PPM files, or every PPM under out/",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if args.gif:
        make_gif(Path(args.gif[0]), Path(args.gif[1]))
        return

    if args.led is not None:
        paths = [Path(value) for value in args.led] or ppm_files(Path("out"))
        for path in paths:
            led_render(path)
        return

    for ppm_path in ppm_files(Path("out")):
        flat_render(ppm_path)


if __name__ == "__main__":
    main()
