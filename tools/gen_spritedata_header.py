import json

r = json.load(open("assets/spritedata/sheet_analysis_clean.json"))
mecha = json.load(open("assets/spritedata/mecha_sonic_itself_boxes.json"))

def rows_of(name): return r[name]["rows"]

# ---- Metal Sonic: hand-verified row -> clip name mapping (see chat writeup) ----
metal_clips = [
    (1,  "Idle"),
    (2,  "Walk"),
    (3,  "Run_A"),
    (4,  "JumpSpin"),
    (5,  "HoverIdle"),
    (6,  "Fx_DustDebris"),
    (7,  "Fx_ChargeRings"),
    (8,  "Fx_LockOnRing"),
    (9,  "Run_B"),
    (10, "Damaged_EMP"),
    (11, "Fx_HomingCrescent"),
    (12, "Dash_Boost"),
    (13, "Attack_Combo"),
]

exe_clips = [
    (0,  "Idle"), (2, "Walk"), (3, "Run"), (4, "TransformStart"),
    (5,  "BloodDrip_Fx"), (6, "Corrupted_Idle"), (7, "Corrupted_Run"),
    (8,  "Attack"), (9, "Taunt"), (10, "Hurt"), (11, "Crouch"),
    (12, "SpinDash"), (13, "SpinTrail_Fx"),
]

tails_clips = [
    (1, "Idle"), (3, "Run"), (4, "ActionSet_1"), (5, "Attacking"),
    (6, "SummonBullets_Fx"), (7, "Freeze"), (8, "BeamAttack_Fx"), (9, "ProjectileSet"),
]

silver_clips = [
    (0, "Idle_A"), (2, "Idle_B"), (4, "Idle_C"), (5, "Run_A"), (6, "Run_B"), (7, "SpinRoll"),
]

mecha_clips = [
    (2,  "Idle"), (5, "ChargingDash"), (8, "RollingJump"),
    (12, "FallingTurn"), (15, "ChargingUp"), (19, "DefeatedPortrait"),
    (20, "TurningSuper"), (21, "SuperMechaSonic"),
]

def emit_table(cname, sheet_key, clip_list, rows_src, y_offset=0):
    out = []
    out.append(f"// {cname} -- extracted from {sheet_key} via tools/extract_sprite_rects.py")
    out.append(f"// Coordinates are VERIFIED (connected-component detection + visual overlay check),")
    out.append(f"// clip NAMES are best-effort guesses from row order/shape -- rename freely, the rects are what matter.")
    out.append(f"inline const std::unordered_map<std::string, std::vector<sf::IntRect>>& {cname}Frames() {{")
    out.append(f"    static const std::unordered_map<std::string, std::vector<sf::IntRect>> table = {{")
    for row_idx, clip_name in clip_list:
        boxes = rows_src[row_idx]["boxes"]
        rects = ", ".join(f"{{{{{b['x']}, {b['y']+y_offset}}}, {{{b['w']}, {b['h']}}}}}" for b in boxes)
        out.append(f'        {{"{clip_name}", {{ {rects} }}}},')
    out.append("    };")
    out.append("    return table;")
    out.append("}")
    return "\n".join(out)

header = []
header.append("#pragma once")
header.append("// " + "="*60)
header.append("// SPRITE DATA -- auto-generated from real pixel analysis of the")
header.append("// uploaded sheets (see tools/extract_sprite_rects.py). Do not hand-edit")
header.append("// rect numbers here; re-run the script and regenerate instead if a")
header.append("// sheet changes. Clip *names* are safe to rename freely (see below).")
header.append("// " + "="*60)
header.append("#include <SFML/Graphics.hpp>")
header.append("#include <unordered_map>")
header.append("#include <vector>")
header.append("#include <string>")
header.append("")
header.append("namespace spritedata {")
header.append("")
header.append(emit_table("metalSonic", "1002050419.png (Metal Sonic / Heroes-style sheet)", metal_clips, rows_of("metal_heroes")))
header.append("")
header.append(emit_table("sonicExe", "1002050423.png (Sonic.exe sheet)", exe_clips, rows_of("sonic_exe")))
header.append("")
header.append(emit_table("tailsDoll", "1002050420.png (Tails Doll sheet)", tails_clips, rows_of("tails_doll")))
header.append("")
header.append(emit_table("silverSonic", "1002050422.png (Silver Sonic sheet)", silver_clips, rows_of("silver_sonic")))
header.append("")
header.append(emit_table("mechaSonic", "1002050421.png (Mecha Sonic sheet, cropped to y[640,1850])", mecha_clips, mecha["rows"], y_offset=mecha["crop_offset_y"]))
header.append("")
def rect_str(b):
    return f"{{{{{b['x']}, {b['y']}}}, {{{b['w']}, {b['h']}}}}}"

header.append("// Sonic.exe's oversized corrupted-face icon (188x193) sits alone, off-grid --")
header.append("// use it as a static HUD/jumpscare portrait, not a played animation.")
box = rows_of("sonic_exe")[1]["boxes"][0]
header.append("inline sf::IntRect sonicExeFaceIcon() { return " + rect_str(box) + "; }")
header.append("")
header.append("} // namespace spritedata")

with open("include/SpriteData.hpp", "w") as f:
    f.write("\n".join(header) + "\n")

print("wrote include/SpriteData.hpp")
print(len("\n".join(header).splitlines()) if False else len(header), "lines")
