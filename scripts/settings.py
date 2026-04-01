from typing import Any

from pydantic_settings import BaseSettings, SettingsConfigDict
from pathlib import Path

# El .env tiene que estar localizado en ../
ROOT = Path(__file__).resolve().parent.parent 

ENV_FILE = ROOT / ".env"
DEVICE_FILE = ROOT / "devices.json"

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
        extra="ignore"
    )

def get_dict_with_prefix(settings: BaseSettings) -> dict[str, Any]:
    """
    Genera un dict con el prefijo de la configuración
    """
    prefix = settings.model_config.get("env_prefix")

    diccionary: dict[str, Any] = {}
    for key, value in settings.model_dump().items():
        new_key = f"{prefix}{key}".upper()
        diccionary[new_key] = value
    
    return diccionary

def value_flag(value: Any) -> str | None:
    """
    Devuelve la representación correcta del valor
    para cargar la flag, la flag en cpp solo acepta
    como values NUMBER y STRING
    """
    if isinstance(value, str): 
        return f'\\\"{value}\\\"'
    if isinstance(value, int):
        return str(value)
    if isinstance(value, bool):
        return None
    if isinstance(value, float):
        return str(value)
    return None

def generate_value_allow_flags(flags: dict[str, Any]) -> dict[str, str | None]:
    """
    Transforma todos los valores de la flags a number o str (solo valores permitidos)
    """
    cpp_flags: dict[str, str | None] = {}

    for key, value in flags.items():
        cpp_key = f"{key}"
        cpp_flags[cpp_key] = value_flag(value)

    return cpp_flags

def building_flag(setting: BaseSettings) -> list[str]:
    """
    Generar flags formato -D[name]=[value]
    """
    cpp_flags = generate_value_allow_flags(get_dict_with_prefix(setting))
    
    build_flags: list[str] = []
    for key, value in cpp_flags.items():
        if value is None:
            build_flags.append(f"-D{key}")
        else:
            build_flags.append(f"-D{key}={value}")

    return build_flags