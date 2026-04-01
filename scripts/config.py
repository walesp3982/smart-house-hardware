from enum import Enum
import json
from pydantic_settings import BaseSettings, SettingsConfigDict
from pydantic import BaseModel

class WifiSettings(BaseSettings):
    SSID: str = ""
    PASSWORD: str = ""
    model_config = SettingsConfigDict(
        env_file=".env",
        extra="ignore",
        env_prefix="WIFI_",
    )

wifi_settings = WifiSettings()

class MQTTConfig(BaseSettings):
    HOST: str = "localhost"
    PORT: int = 1883
    USER: str = ""
    PASSWORD: str = ""
    model_config = SettingsConfigDict(
        env_file=".env",
        extra="ignore",
        env_prefix="MQTT_",
    )

mqtt_settings = MQTTConfig() # pyright: ignore[reportCallIssue]
class DeviceValue(BaseModel):
    cpp_value: int
    web_value: str

class TypeDevice(Enum):
    light = "light"
    camera = "camera"
    door = "door"
    thermostat = "thermostat"
    movement = "movement"

    def get_values_program(self) -> DeviceValue:
        match self:
            case self.light:
                return DeviceValue(cpp_value=0, web_value="light")
            case self.camera:
                return DeviceValue(cpp_value=1, web_value="camera")
            case self.door:
                return DeviceValue(cpp_value=2, web_value="door")
            case self.thermostat:
                return DeviceValue(cpp_value=4, web_value="thermostat")
            case self.movement:
                return DeviceValue(cpp_value=5, web_value="movement")
            


class DeviceMetadata(BaseModel):
    name: str
    uuid: str
    type: TypeDevice

class Device(DeviceMetadata):
    verification_code: str
    

class DevicesSettings(BaseModel):
    devices: list[Device] = []

def load_json_device() -> DevicesSettings:
    with open("devices.json") as file:
        data = json.load(file)

    return DevicesSettings(**data)