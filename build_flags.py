from scripts.settings import WifiSettings, MQTTSettings, building_flag

def main():
    for flag in building_flag(WifiSettings()): # pyright: ignore[reportCallIssue]
        print(flag)

    for flag in building_flag(MQTTSettings()):
        print(flag)

if __name__ == "__main__":
    main()