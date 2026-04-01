from dataclasses import dataclass
from typing import Literal
from pathlib import Path
import json

VALID_TARGETS = ("esp32cam", "esp32")
TargetsOptions = Literal["esp32cam", "esp32"]

"""
NOTA DEL CÓDIGO:
SOLO UTILIZAR LIBRERÍAS DISPONIBLES EN 
LA LIBRERÍA ESTÁNDAR DE PYTHON

"""

@dataclass
class Device:
    uuid: str
    target: list[TargetsOptions]
    name: str


def get_list_device_by_file(filepath: Path) -> list[Device]:
    with open(filepath) as file:
        data = json.load(file)
        devices: list[Device] = []
        for device in data["devices"]:
            devices.append(
                Device(
                    uuid=device["uuid"],
                    target=device["target"],
                    name=device["name"],
                )
            )
        return devices


def filter_devices(devices: list[Device], option: TargetsOptions):
    return list(device for device in devices if option in device.target)
