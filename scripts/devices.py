from enum import Enum
from pathlib import Path
from pydantic import BaseModel
from typing import Literal
import json

class DeviceValue(BaseModel):
    cpp_value: int
    web_value: str

class TypeDevice(Enum):
    light = "light"
    camera = "camera"
    door = "door"
    thermostat = "thermostat"
    movement = "movement"


VALID_TARGETS = ("esp32cam", "esp32")
TargetsOptions = Literal["esp32cam", "esp32"]
class Device(BaseModel):
    name: str
    uuid: str
    type: TypeDevice
    target: list[TargetsOptions]
    verification_code: str

class ListDevices(BaseModel):
    devices: list[Device]

    def filter(self, target: TargetsOptions) -> list[Device]:
        return list(device for device in self.devices if target in device.target) 

def get_list_device_by_file(filepath: Path) -> ListDevices: 
    with open(filepath) as file:
        data = json.load(file)
        list_devices = ListDevices(**data)
        return list_devices