"""Generate the indexed C texture from the shipping artwork (requires Pillow)."""
from pathlib import Path
from PIL import Image
root = Path(__file__).resolve().parents[1]
img = Image.open(root/'art/cover.png').convert('P')
palette = img.getpalette()
with (root/'src/cover.inc').open('w') as f:
    f.write('static const unsigned int inkset[] = {' + ','.join('0x%02x%02x%02x'%tuple(palette[i:i+3]) for i in range(0,len(palette),3))+'};\n')
    f.write('static const unsigned char artwork[411][408] = {\n')
    data = img.tobytes()
    assert img.size == (408,411)
    for y in range(411):
        f.write('{'+','.join(map(str,data[y*408:(y+1)*408]))+'},\n')
    f.write('};\n')
