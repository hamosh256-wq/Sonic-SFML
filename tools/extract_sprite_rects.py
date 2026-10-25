import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage
import json
from collections import Counter

def bg_by_mode(arr, sample_stride=3):
    """Most frequent color across a downsampled grid of the whole image - robust
    against black letterbox corners that corner-sampling was fooled by."""
    sample = arr[::sample_stride, ::sample_stride].reshape(-1, arr.shape[2])
    counts = Counter(map(tuple, sample))
    return np.array(counts.most_common(1)[0][0])

def analyze(path, min_h=10, min_w=5, min_area=30, tol=10, crop=None):
    img = Image.open(path).convert("RGBA")
    if crop: img = img.crop(crop)
    arr = np.array(img)
    H, W = arr.shape[0], arr.shape[1]
    bg = bg_by_mode(arr)

    diff = np.abs(arr.astype(int) - bg.astype(int)).sum(axis=2)
    mask = diff > tol
    if arr.shape[2] == 4:
        mask &= (arr[:,:,3] > 10)

    labeled, n = ndimage.label(mask, structure=np.ones((3,3)))
    objs = ndimage.find_objects(labeled)
    boxes = []
    for sl in objs:
        if sl is None: continue
        y0,y1 = sl[0].start, sl[0].stop
        x0,x1 = sl[1].start, sl[1].stop
        w,h = x1-x0, y1-y0
        area = int(mask[sl].sum())
        if h < min_h or w < min_w or area < min_area: continue
        boxes.append({"x":int(x0),"y":int(y0),"w":int(w),"h":int(h),"area":area})
    return {"size":[W,H], "bg": bg.tolist(), "n_raw": n, "n_filtered": len(boxes), "boxes": boxes}

def cluster_rows(boxes, y_tol=9):
    boxes = sorted(boxes, key=lambda b: b["y"]+b["h"]/2)
    rows = []
    for b in boxes:
        cy = b["y"]+b["h"]/2
        for row in rows:
            row_cy = sum(r["y"]+r["h"]/2 for r in row)/len(row)
            if abs(cy-row_cy) < y_tol:
                row.append(b); break
        else:
            rows.append([b])
    for row in rows: row.sort(key=lambda b: b["x"])
    rows.sort(key=lambda row: sum(r["y"] for r in row)/len(row))
    return rows

files = {
    "metal_heroes": ("/mnt/user-data/uploads/1002050419.png", None),
    "tails_doll":   ("/mnt/user-data/uploads/1002050420.png", None),
    "silver_sonic": ("/mnt/user-data/uploads/1002050422.png", None),
    "sonic_exe":    ("/mnt/user-data/uploads/1002050423.png", None),
}

results = {}
for name, (path, crop) in files.items():
    r = analyze(path, crop=crop)
    rows = cluster_rows(r["boxes"])
    r["rows"] = [{"count": len(row), "avg_w": round(sum(b["w"] for b in row)/len(row),1),
                  "avg_h": round(sum(b["h"] for b in row)/len(row),1),
                  "y_center": round(sum(b["y"]+b["h"]/2 for b in row)/len(row),1),
                  "boxes": row} for row in rows]
    results[name] = r
    print(f"=== {name} === size={r['size']} bg={r['bg']} raw={r['n_raw']} filtered={r['n_filtered']} rows={len(rows)}")
    for i,row in enumerate(r["rows"]):
        xs = [b['x'] for b in row["boxes"]]
        print(f"  row{i}: n={row['count']:2d} avg={row['avg_w']:5.1f}x{row['avg_h']:5.1f} y~{row['y_center']:6.1f}  x0={min(xs)}")

with open("/home/claude/analysis/results2.json","w") as f: json.dump(results, f, indent=1)
