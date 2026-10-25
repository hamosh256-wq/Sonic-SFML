import numpy as np
from PIL import Image
from scipy import ndimage
from collections import Counter
import json

def bg_by_mode(arr, stride=3):
    sample = arr[::stride, ::stride].reshape(-1, arr.shape[2])
    return np.array(Counter(map(tuple, sample)).most_common(1)[0][0])

img = Image.open('mecha_itself_crop.png').convert("RGBA")
arr = np.array(img)
bg = bg_by_mode(arr)
print("bg:", bg)
diff = np.abs(arr.astype(int)-bg.astype(int)).sum(axis=2)
mask = (diff > 14) & (arr[:,:,3] > 10)
labeled, n = ndimage.label(mask, structure=np.ones((3,3)))
objs = ndimage.find_objects(labeled)
boxes=[]
for sl in objs:
    if sl is None: continue
    y0,y1=sl[0].start,sl[0].stop; x0,x1=sl[1].start,sl[1].stop
    w,h=x1-x0,y1-y0; area=int(mask[sl].sum())
    if h<12 or w<8 or area<60: continue
    if w>350 or h>250: continue  # drop giant merged section-header/table blobs
    boxes.append({"x":int(x0),"y":int(y0),"w":int(w),"h":int(h),"area":area})
print("raw", n, "filtered", len(boxes))

def cluster_rows(boxes, y_tol=10):
    boxes = sorted(boxes, key=lambda b: b["y"]+b["h"]/2)
    rows=[]
    for b in boxes:
        cy=b["y"]+b["h"]/2
        for row in rows:
            rcy=sum(r["y"]+r["h"]/2 for r in row)/len(row)
            if abs(cy-rcy)<y_tol: row.append(b); break
        else: rows.append([b])
    for row in rows: row.sort(key=lambda b:b["x"])
    rows.sort(key=lambda row: sum(r["y"] for r in row)/len(row))
    return rows

rows = cluster_rows(boxes)
for i,row in enumerate(rows):
    xs=[b['x'] for b in row]
    ws=[b['w'] for b in row]
    print(f"row{i}: n={len(row):2d} avgW={sum(ws)/len(row):5.1f} avgH={sum(b['h'] for b in row)/len(row):5.1f} y~{sum(b['y']+b['h']/2 for b in row)/len(row):6.1f} x0={min(xs)}")

with open("mecha_boxes.json","w") as f:
    json.dump({"crop_offset_y":640, "rows":[{"boxes":row} for row in rows]}, f, indent=1)

# draw overlay for visual verification
from PIL import ImageDraw
overlay = img.convert("RGB").copy()
d = ImageDraw.Draw(overlay)
for row in rows:
    for b in row:
        d.rectangle([b['x'],b['y'],b['x']+b['w'],b['y']+b['h']], outline=(255,0,255), width=1)
overlay.save('mecha_overlay.png')
