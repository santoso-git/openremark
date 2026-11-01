# Draws one synthetic notebook page per case in cases.json, the size of the tablet's capture area.
# Questions use a handwriting-like font (FONT, default Bradley Hand on macOS); answers use Caveat, as on the tablet.
import json, os, sys, textwrap
from PIL import Image, ImageDraw, ImageFont
here = os.path.dirname(os.path.abspath(__file__))
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, 'pages')
os.makedirs(out, exist_ok=True)
pen = ImageFont.truetype(os.environ.get('FONT', '/System/Library/Fonts/Supplemental/Bradley Hand Bold.ttf'), 64)
caveat = ImageFont.truetype(os.path.join(here, '..', '..', 'fonts', 'Caveat.ttf'), 46)
for name, case in json.load(open(os.path.join(here, 'cases.json'))).items():
    image = Image.new('RGB', (1174, 1752), 'white')
    draw = ImageDraw.Draw(image)
    y = 60
    for kind, text in case['page']:
        if kind == 'q':
            draw.text((60, y), text, font=pen, fill='black'); y += 110
        else:
            for line in textwrap.wrap(text, 48):
                draw.text((60, y), line, font=caveat, fill='black'); y += 62
            y += 50
    image.save(os.path.join(out, name + '.png'))
print(out)
