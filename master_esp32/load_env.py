import os
from config import wifi_settings, load_json_device


def build_flag_bool(key: str, condition: bool):
    if condition:
        return f"-D{key}"
    return ""

def build_flag(key: str, value: str | int | bool):
    if isinstance(value, bool):
        return build_flag_bool(key, value)

    result: str  = ""
    if isinstance(value, int):
        result = f"{value}"
        
    if isinstance(value, str):
        result = f'\\\"{value}\\\"'
    return f'-D{key.strip()}={result}'

with open('.env', 'r') as f:
    # Carga flags del .env
    for key, value in wifi_settings.model_dump().items():
        prefix = wifi_settings.model_config.get("env_prefix")
        key = f"{prefix}{key}"
        print(build_flag(key, value))

    # Cargar flags del devices.json
    config_devices = load_json_device()

    for device in config_devices.devices:
        prefix_name = device.name
        device_dict = device.model_dump(exclude={"name", "verification_code"})
        type_device = device.type
        for key, value in device_dict.items():
            if key == "type":
                value = type_device.get_values_program().cpp_value
            key = f"{prefix_name}_{key}".upper()
            print(build_flag(key, value))