import json
import uuid
import secrets
import string

def generate_verification_code(length=6):
    alphabet = string.ascii_uppercase + string.digits
    return ''.join(secrets.choice(alphabet) for _ in range(length))

def generate_secrets():
    with open("devices.json") as file:
        data = json.load(file)

    # Procesar dispositivos
    for device in data.get("devices", []):
        # Solo generar si no existe (para no sobrescribir)
        if not device["uuid"]:
            device["uuid"] = str(uuid.uuid4())

        if not device["verification_code"]:
            device["verification_code"] = generate_verification_code()

    # Guardar cambios
    with open("devices.json", "w") as f:
        json.dump(data, f, indent=2)

if __name__ == "__main__":
    generate_secrets()