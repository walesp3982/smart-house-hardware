from __future__ import annotations
import json
import re
from pathlib import Path
from typing import Literal

ROOT = Path(__file__).resolve().parent
ENV_FILE = ROOT / ".env"
DEVICE_FILE = ROOT / "devices.json"
ESP32_INCLUDE_DIR = ROOT / "esp32-actuator" / "include"
GENERATED_HEADER = ESP32_INCLUDE_DIR / "generated_devices.h"

try:
    from pydantic import BaseModel
    from pydantic_settings import BaseSettings, SettingsConfigDict
    HAS_PYDANTIC = True
except ImportError:
    BaseModel = object
    BaseSettings = object
    SettingsConfigDict = dict
    HAS_PYDANTIC = False


def sanitize_macro_name(name: str) -> str:
    return re.sub(r"[^A-Z0-9_]+", "_", name.upper())


def parse_env_file() -> dict[str, str]:
    if not ENV_FILE.exists():
        raise FileNotFoundError(f"{ENV_FILE} not found")
    result: dict[str, str] = {}
    for raw_line in ENV_FILE.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        result[key.strip()] = value.strip()
    return result


if HAS_PYDANTIC:
    class WifiSettings(BaseSettings):
        SSID: str
        PASSWORD: str

        model_config = SettingsConfigDict(
            env_file=ENV_FILE,
            env_prefix="WIFI_",
            extra="ignore",
        )

    class MQTTSettings(BaseSettings):
        HOST: str = "localhost"
        PORT: int = 1883
        USER: str = ""
        PASSWORD: str = ""

        model_config = SettingsConfigDict(
            env_file=ENV_FILE,
            env_prefix="MQTT_",
            extra="ignore",
        )

    class Device(BaseModel):
        name: str
        uuid: str
        type: Literal["light", "camera", "door", "thermostat", "movement"]
        verification_code: str

    class DevicesSettings(BaseModel):
        devices: list[Device]

    def load_devices() -> list[Device]:
        if not DEVICE_FILE.exists():
            raise FileNotFoundError(f"devices.json not found at {DEVICE_FILE}")
        return DevicesSettings.model_validate_json(DEVICE_FILE.read_text(encoding="utf-8")).devices

    def load_settings() -> tuple[WifiSettings, MQTTSettings]:
        return WifiSettings(), MQTTSettings()

else:
    class WifiSettings:
        def __init__(self, data: dict[str, str]) -> None:
            self.SSID = data.get("WIFI_SSID", "")
            self.PASSWORD = data.get("WIFI_PASSWORD", "")

        def model_dump(self) -> dict[str, str]:
            return {"SSID": self.SSID, "PASSWORD": self.PASSWORD}

    class MQTTSettings:
        def __init__(self, data: dict[str, str]) -> None:
            self.HOST = data.get("MQTT_HOST", "localhost")
            self.PORT = int(data.get("MQTT_PORT", "1883"))
            self.USER = data.get("MQTT_USER", "")
            self.PASSWORD = data.get("MQTT_PASSWORD", "")

        def model_dump(self) -> dict[str, str | int]:
            return {
                "HOST": self.HOST,
                "PORT": self.PORT,
                "USER": self.USER,
                "PASSWORD": self.PASSWORD,
            }

    def load_devices() -> list[dict[str, str]]:
        if not DEVICE_FILE.exists():
            raise FileNotFoundError(f"devices.json not found at {DEVICE_FILE}")
        data = json.loads(DEVICE_FILE.read_text(encoding="utf-8"))
        return data.get("devices", [])

    def load_settings() -> tuple[WifiSettings, MQTTSettings]:
        data = parse_env_file()
        return WifiSettings(data), MQTTSettings(data)


def build_env_defines(wifi: WifiSettings, mqtt: MQTTSettings) -> list[tuple[str, str | int | bool]]:
    return [
        ("WIFI_SSID", wifi.SSID),
        ("WIFI_PASSWORD", wifi.PASSWORD),
        ("MQTT_HOST", mqtt.HOST),
        ("MQTT_PORT", mqtt.PORT),
        ("MQTT_USER", mqtt.USER),
        ("MQTT_PASSWORD", mqtt.PASSWORD),
    ]


def get_device_field(device: object, field: str) -> str:
    if isinstance(device, dict):
        return device[field]
    return getattr(device, field)


def generate_header(devices: list[object]) -> None:
    ESP32_INCLUDE_DIR.mkdir(parents=True, exist_ok=True)
    lines = [
        "#pragma once\n",
        "#include <Arduino.h>\n",
        "\n",
        "struct DeviceMetadata {\n",
        "    const char* name;\n",
        "    const char* uuid;\n",
        "    const char* type;\n",
        "};\n",
        "\n",
    ]

    for device in devices:
        macro_name = sanitize_macro_name(get_device_field(device, "name"))
        lines.append(f"#define UUID_{macro_name} \"{get_device_field(device, 'uuid')}\"\n")

    lines.append("\nstatic constexpr DeviceMetadata DEVICE_METADATA[] = {\n")
    for device in devices:
        name = get_device_field(device, "name")
        macro_name = sanitize_macro_name(name)
        device_type = get_device_field(device, "type")
        lines.append(
            f"    {{ \"{name}\", UUID_{macro_name}, \"{device_type}\" }},\n"
        )
    lines.append("};\n\n")
    lines.append("static constexpr size_t DEVICE_METADATA_COUNT = sizeof(DEVICE_METADATA) / sizeof(DeviceMetadata);\n")

    GENERATED_HEADER.write_text("".join(lines), encoding="utf-8")


def write_build_defines(env_defines: list[tuple[str, str | int | bool]]) -> None:
    lines: list[str] = []
    for key, value in env_defines:
        if isinstance(value, bool):
            lines.append(f"#define {key} {1 if value else 0}\n")
        elif isinstance(value, int):
            lines.append(f"#define {key} {value}\n")
        else:
            safe_value = str(value).replace('"', '\\"')
            lines.append(f"#define {key} \"{safe_value}\"\n")

    target = ESP32_INCLUDE_DIR / "generated_env.h"
    ESP32_INCLUDE_DIR.mkdir(parents=True, exist_ok=True)
    target.write_text("".join(lines), encoding="utf-8")


def main() -> int:
    wifi_settings, mqtt_settings = load_settings()
    devices = load_devices()

    generate_header(devices)
    write_build_defines(build_env_defines(wifi_settings, mqtt_settings))

    try:
        from SCons.Script import Import
        Import("env")
        env = globals().get("env")
        if env is not None:
            env.Append(CPPDEFINES=[
                ("WIFI_SSID", wifi_settings.SSID),
                ("WIFI_PASSWORD", wifi_settings.PASSWORD),
                ("MQTT_HOST", mqtt_settings.HOST),
                ("MQTT_PORT", mqtt_settings.PORT),
                ("MQTT_USER", mqtt_settings.USER),
                ("MQTT_PASSWORD", mqtt_settings.PASSWORD),
            ])
    except ImportError:
        pass

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
