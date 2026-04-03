import httpx
from scripts.settings import AppSettings
from scripts.devices import ListDevices, Device, get_list_device_by_file
from pydantic import BaseModel
from urllib.parse import ParseResult, urlunparse
from pathlib import Path

DEVICE_JSON_PATH = Path(__file__).parent / "devices.json"

class DeviceRequest(BaseModel):
    uuid: str
    type: str
    activation_code: str

    @staticmethod
    def from_entity(entity: Device) -> "DeviceRequest":
        return DeviceRequest(
            uuid=entity.uuid,
            activation_code=entity.verification_code,
            type=entity.type.value
        )
    
class DeviceCreatedResponse(BaseModel):
    message: str

def building_url():
    """
    Creamos la url donde vamos a guardar la url
    """
    app_setting = AppSettings() # pyright: ignore[reportCallIssue]
    host: str = app_setting.host.encoded_string()
    return urlunparse(
        ParseResult(
            scheme="http",
            netloc=host,
            path="devices",
            params="",
            query="",
            fragment=""
        )
    )

class DeviceCannotCreatedException(Exception):
    def __init__(self, status_code: int, uuid: str):
        super().__init__(f"El dispositivo no pudo ser creado uuid:{uuid}, status_code:{status_code}")

def post_device_api(device: DeviceRequest, url: str) -> DeviceCreatedResponse:
    response = httpx.post(
        url=url,
        json=device.model_dump()
    )

    if(response.status_code == 200):
        return DeviceCreatedResponse(**response.json())
    else:
        print(response)
        raise DeviceCannotCreatedException(response.status_code, device.uuid)
    
def main():
    list_devices: ListDevices = get_list_device_by_file(DEVICE_JSON_PATH)
    devices_request: list[DeviceRequest] = [DeviceRequest.from_entity(device) for device in list_devices.devices]
    url = building_url()
    print(f"Creando dispositivos a api: {url}")
    print("*"*30)

    for device in devices_request:
        try:
            response = post_device_api(device, url)
            print(f"Ok: {response.message}")
        except DeviceCannotCreatedException as err:
            print("Error: ", err)
        except Exception as err:
            print("Error desconocido", err.__str__())
    print("*"*30)
        
    
if __name__ == "__main__":
    main()