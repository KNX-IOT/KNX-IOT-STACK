# test scripts

The test scripts in this folder are python script based on the python binding of the stack.

## requirements

- see python bindings
- device does not have security turned on.

## Test scripts

### reset_devices

script to reset the device, issuing:

- POST to /a/sen with the reset command

```bash
python reset_devices/py
```

### knx_test_sequence

script to test functionallity

- POST to /a/sen with the reset command

```bash
python knx_test_sequence.py
```

### install_devices.py

script to do a installation, e.g. configure /fp/g

- works for LSAB & LSSB devices

```bash
python install_devices.py
```
