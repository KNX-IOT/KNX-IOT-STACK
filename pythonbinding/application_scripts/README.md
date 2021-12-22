# test scripts

The test scripts in this folder are python script based on the python binding of the stack.

## requirements

- see python bindings
- device does not have security turned on.

## application scripts


### list_devices.py

script to list all the devices, issuing:

- discovery with query : rt=urn:knx:dpa.*

```bash
python list_devices/py -h
```

### reset_device.py

script to reset the device, issuing:

- discovery with serial number
- POST to /a/sen with the reset command

```bash
python reset_device/py -h
```


### list_device_with_ia.py

script to list the device with internal address, issuing:

- discovery with query : if=urn:knx:ia.[ia]

```bash
python list_device_with_ia/py -h
```


### s-mode.py

script to issue an s-mode command

- works on multicast all coap nodes
- has option to set the various values in the command.

```bash
python s-mode.py 
```

## test scripts

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

