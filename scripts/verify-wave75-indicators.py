"""Check the shipped archive call sites against the newly linked firmware."""
import argparse
import json
import re
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--objdump", required=True)
parser.add_argument("--firmware", required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
keyboard = root / "keyboards/hm/wave75_rgb"

def disassemble(path):
    return subprocess.check_output([args.objdump, "-dr", str(path)], text=True)

def functions(dump):
    result = {}
    matches = list(re.finditer(r"^([0-9a-f]+) <([^>]+)>:\s*$", dump, re.M))
    for i, match in enumerate(matches):
        end = matches[i + 1].start() if i + 1 < len(matches) else len(dump)
        result[match[2]] = (int(match[1], 16), dump[match.end():end])
    return result

source = (keyboard / "indicator_lighting.c").read_text(encoding="utf-8")
calls = re.findall(r"\{(\w+), (0x[0-9a-f]+), (160|255)\}", source)
assert len(calls) == 16
archive = functions(disassemble(root / "lib/rdr_lib/librdrcommon.a"))
firmware = functions(disassemble(args.firmware))
for name, offset, maximum in calls:
    instruction = int(offset, 16) - 4
    assert re.search(rf"^\s*{instruction:x}: R_ARM_THM_CALL\s+rgb_matrix_set_color$", archive[name][1], re.M), (name, offset)
    instruction += firmware[name][0]
    assert re.search(rf"^\s*{instruction:x}:.*\bbl\s+[^\n]*<__wrap_rgb_matrix_set_color>", firmware[name][1], re.M), (name, offset)
assert re.search(r"\bbl\s+[^\n]*<rgb_matrix_set_color>", firmware["__wrap_rgb_matrix_set_color"][1])
assert "<rgb_matrix_driver_set_color_all>" in firmware["User_Test_Colour_Show"][1]
normal = {"Logo_Wave_Rgb_mode_Show", "Logo_Wave_Ds_mode_Show", "Logo_Spectrum_mode_Show", "Logo_Breath_mode_Show", "Logo_Light_mode_Show"}
assert not normal.intersection(name for name, _, _ in calls)
for name in normal:
    assert "<__wrap_rgb_matrix_set_color>" in firmware[name][1]

# Check symbol interposition selected our endpoint-correct PWM routines.
for name in ("Logo_Pwm_Rgb_Updata", "Logo_Pwm_Ds_Updata"):
    address = firmware[name][0]
    symbol_name = re.escape(name)
    symbol_address = re.escape(f"0x{address:08x}")
    link_map = Path(args.firmware).with_suffix(".map").read_text(encoding="utf-8")
    assert re.search(rf"\.text\.{symbol_name}\s+{symbol_address}\s+0x[0-9a-f]+\s+[^\n]*[/\\]wave75_rgb\.o", link_map), name
for name, helper in (
    ("Logo_Wave_Rgb_mode_Show", "Logo_Pwm_Rgb_Updata"),
    ("Logo_Spectrum_mode_Show", "Logo_Pwm_Rgb_Updata"),
    ("Logo_Wave_Ds_mode_Show", "Logo_Pwm_Ds_Updata"),
    ("Logo_Breath_mode_Show", "Logo_Pwm_Ds_Updata"),
    ("Logo_Light_mode_Show", "Logo_Pwm_Ds_Updata"),
):
    assert f"<{helper}>" in firmware[name][1], name

definition = json.loads((keyboard / "WAVE 75 RGB.JSON").read_text(encoding="utf-8"))
controls = {}
identifiers = set()

def visit(node):
    if isinstance(node, list):
        for child in node:
            visit(child)
    elif isinstance(node, dict):
        content = node.get("content")
        if isinstance(content, list) and len(content) == 3 and isinstance(content[0], str) and isinstance(content[1], int):
            key = tuple(content[1:])
            assert key not in controls, key
            assert content[0] not in identifiers, content[0]
            controls[key] = node
            identifiers.add(content[0])
        for child in node.values():
            visit(child)

visit(definition["menus"])
assert controls[2, 7]["type"] == "toggle"
assert controls[2, 8]["type"] == "toggle"
assert controls[2, 9]["options"] == [0, 100]
assert controls[2, 9]["showIf"] == "{id_indicator_custom} == 1"
assert controls[2, 10]["type"] == "toggle"
assert controls[2, 1]["options"] == [0, 255]
assert controls[3, 1]["options"] == [0, 255]
print("Indicator call sites, PWM overrides, factory exception and 0-255 VIA controls verified")
