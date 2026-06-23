# DeviceInfo PA Sublayer Simulation

This directory contains the simulated Telux platform component implementation, including `DeviceInfoManager`.

## DeviceInfoManager GetIMEI behavior

`DeviceInfoManager::getIMEI()` reads the IMEI value from an optional runtime JSON file.

The expected runtime file path is:

```text
/tmp/IDeviceInfoManager.json
```

If this file is not present, or if the file cannot be parsed correctly, `getIMEI()` returns the default IMEI value:

```text
000000000000000
```

## Providing a custom IMEI

To make `getIMEI()` return a custom IMEI, explicitly place a JSON file at:

```text
/tmp/IDeviceInfoManager.json
```

The JSON file must use the following format:

```json
{
  "IDeviceInfoManager": {
    "GetIMEI": {
      "imei": "353050681787294"
    }
  }
}
```

After this file is available, calls to `DeviceInfoManager::getIMEI()` will return:

```text
353050681787294
```

## Running inside a container

This simulation runs inside a container. Therefore, the JSON file must exist inside the container filesystem at:

```text
/tmp/IDeviceInfoManager.json
```

Creating the file only on the host machine is not enough unless the file or directory is mounted into the container.

### Option 1: Copy the file into a running container

Create the JSON file on the host:

```bash
cat > IDeviceInfoManager.json <<'EOF'
{
  "IDeviceInfoManager": {
    "GetIMEI": {
      "imei": "353050681787294"
    }
  }
}
EOF
```

Copy it into the running container:

```bash
docker cp IDeviceInfoManager.json <container_name_or_id>:/tmp/IDeviceInfoManager.json
```

### Option 2: Create the file from inside the container

Enter the running container:

```bash
docker exec -it <container_name_or_id> /bin/bash
```

Create the JSON file inside the container:

```bash
cat > /tmp/IDeviceInfoManager.json <<'EOF'
{
  "IDeviceInfoManager": {
    "GetIMEI": {
      "imei": "353050681787294"
    }
  }
}
EOF
```

## Notes

- The JSON file is read from `/tmp/IDeviceInfoManager.json` at runtime.
- The file is not automatically installed by this component.
- `/tmp` is a temporary location and may be cleared after container restart, reboot, or cleanup.
- If the JSON file is missing, invalid, or does not contain `IDeviceInfoManager.GetIMEI.imei`, the default IMEI `000000000000000` is returned.
