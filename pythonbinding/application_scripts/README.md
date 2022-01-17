# test scripts

The test scripts in this folder are python script based on the python binding of the stack.
Most of the application needs the serial number of the device so that the application knows to which device the command applies.

## requirements

- see python bindings
- device does not have security turned on.

## application scripts


### list_devices.py

script to list all the devices, issuing:

- discovery with query : rt=urn:knx:dpa.*

```bash
python list_devices.py -h
```

### reset_device.py

script to reset the device, issuing:

- discovery with serial number
- POST to /a/sen with the reset command

```bash
python reset_device.py -h
```


### list_device_with_ia.py

script to list the device with internal address, issuing:

- discovery with query : if=urn:knx:ia.[ia]

```bash
python list_device_with_ia.py -h
```

### install_config.py

script to configuring a device, issuing:

- discovery a device with sn
- putting the device in programming mode to set:
   - internal address (ia)
   - installation id (iid)
- putting the device in loading state
   - configure Group Object Table
   - configure Recipient Table
   - configure Publisher Table

```bash
python install_config.py -h
```

Note: the internal address is kept out of the configuration file.
so that the config can be reused for more than 1 device in the installation.
The internal address needs to be given as command line option

```bash
python install_config.py -sn 000003 -ia 1 -file LSAB_config.json
```

#### configuration file

The configuration file is a json formatted file.
config data:
- installation id: key = "iid"
- group object table: Key = "groupobject"
- recipient table: Key = "recipient"
- publisher table: Key = "publisher"

The content of the tables are the items in an array.
The items have the json keys (e.g. "id" instead of 0)
The application converts the json data in to data with integer keys and then convert the contents to cbor.

##### group object table

The group object table contains the json keys for an Group Object Table entry.

```bash
"groupobject" : [ 
    { "id": 1, "href": "p/push", "ga" :[1], "cflag" : ["w"] },
    { "id": 1, "href": "p/push", "ga" :[1], "cflag" : ["r"] }] 
```

##### publisher table
The group object table contains the json keys for an Publisher entry.
note that this table contains the info of the sending side.
Note that the ia (and path) needs to be defined or the url.
if ia is defined and path is not there, the path will have the default value ".knx".

```bash
"publisher" : [ 
      {
         "id": "1",
          ia": 5,
         "ga":[2305, 2401],
         "path": ".knx",
     },
     {
         "id": "2",
         "url": "coap://<IP multicast, unicast address or fqdn>/<path>",
         "ga": [2305, 2306, 2307, 2308]
      }] 
```

##### recipient table
The group object table contains the json keys for an Publisher entry.
note that this table contains the info of the receiving side.
Note that the ia (and path) needs to be defined or the url.
if ia is defined and path is not there, the path will have the default value ".knx".

```bash
"recipient" : [ 
 *    {
 *       "id": "1", ia": 5, "ga":[2305, 2401], "path": ".knx",
 *    },
 *    {
 *        "id": "2","url": "coap://<IP multicast, unicast address or fqdn>/<path>", 
 * "ga": [2305, 2306, 2307, 2308]
 *     }] 
```

### s-mode.py

script to issue an s-mode command

- works on multicast all coap nodes
- has option to set the various values in the command.

```bash
python s-mode.py 
```

### install_demo.py

convieniance script to install the LSAB + LSSB demo.
 
the script to does an predefined installation

- works for LSAB & LSSB devices
- works only for devices with serial numbers:
  - 000001 (LSAB)
  - 000002 (LSSB)
  - 000003 (LSAB)
  - 000004 (LSSB)
```bash
python install_devices.py
```

