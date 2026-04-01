from pathlib import Path
from scripts.defines import (
    TargetsOptions,
    get_list_device_by_file,
    Device,
    filter_devices
)
Import("env")  # type: ignore # noqa: F821

PLATFORMIO_PROYECT = Path.cwd()
PROYECT_NAME = PLATFORMIO_PROYECT.name
INCLUDE_PATH = PLATFORMIO_PROYECT / "include" / "generated_devices.h"

ROOT = Path(env["PROJECT_DIR"]).resolve().parent # type: ignore  # noqa: F821
DEVICE_JSON = ROOT / "devices.json"


def generate_lines_code_defines(devices: list[Device]) -> list[str]:
    lines = []
    lines.append("#pragma once")
    for device in devices:
        define = f'#define UUID_{device.name.upper()} "{device.uuid}" '
        lines.append(define)
    return lines


def generate_include_defines(devices: list[Device], include_path: Path):
    include_path.parent.mkdir(parents=True, exist_ok=True)
    file = include_path
    lines = "\n".join(generate_lines_code_defines(devices))
    file.touch()
    file.write_text(lines)


def main():
    if PROYECT_NAME == "esp32-actuator":
        target: TargetsOptions = "esp32"
    else:
        target: TargetsOptions = "esp32cam"

    print(f"[load.py] Proyecto: {PROYECT_NAME}")
    print(f"[load.py] Target: {target}")
    print(f"[load.py] JSON: {DEVICE_JSON}")
    print(f"[load.py] Output: {INCLUDE_PATH}")
    list_device = get_list_device_by_file(DEVICE_JSON)

    devices = filter_devices(list_device, target)
    generate_include_defines(devices, INCLUDE_PATH)


main()