# test scripts

The test scripts in this folder are python script based on the python binding of the stack.
Most of the application needs the serial number of the device so that the application knows to which device the command applies.

## requirements

- see python bindings
- device does not have security turned on.

## application scripts

### reset_device.py

script to reset the device, issuing:

- discovery with serial number
- POST to /a/sen with the reset command

```bash
python reset_device.py -h
```

### list_devices.py

script to list the device, issuing:

- discovery of device with internal address using query : if=urn:knx:ia.[ia]
- discovery of device in programming mode using query : if=urn:knx:if.pm
- discovery of device with a serial_number using query : ep=urn:knx:sn.[serial_number]
- discovery of device using a group_address using query : d=urn:knx:g.s.[group_address]
- discovery with query : rt=urn:knx:dpa.*

```bash
python list_devices.py -h
```

### s-mode.py

script to issue an s-mode command

- has option to set the various values in the command.

```bash
python s-mode.py 
```

### sniffer-s-mode.py

script to listen to s-mode commands

- has option to set iid and max group number

```bash
python sniffer-s-mode.py 
```

### programming_mode.py

script to issue set a specific device (via serial_number) in programming mode

```bash
python programming_mode.py 
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
- access token table: Key = "auth"
- parameter (table): Key = "memparameter"

The content of the tables are the items in an array.
The items have the json keys (e.g. "id" instead of 0)
The application converts the json data in to data with integer keys and then convert the contents to cbor.

##### group object table

The group object table contains the array of json objects for an Group Object Table entry.

- id (0): identifier in the group object table
- href (11) : the href of the point api url
- ga (7): the array of group addresses
- cflags (8) : the communication flags (as strings)
  the cflags array will be converted into the bit flags.

```bash
{
....
"groupobject" : [ 
    { "id": 1, "href": "p/push", "ga" :[1], "cflag" : ["w"] },
    { "id": 1, "href": "p/push", "ga" :[1], "cflag" : ["r"] }] 
....
```

##### publisher table

The group object table contains the array of json objects for an Publisher entry.
note that this table contains the info of the sending side.
Note that the ia (and path) needs to be defined or the url.
if ia is defined and path is not there, the path will have the default value ".knx".

- id (0): identifier in the group object table
- ia (12) : internal address
- ga (7): the array of group addresses

```bash
{
....
"publisher" : [ 
      {
         "id": "1",
         "ia": 5,
         "ga":[2305, 2401],
         "path": ".knx",
     },
     {
         "id": "2",
         "url": "coap://<IP multicast, unicast address or fqdn>/<path>",
         "ga": [2305, 2306, 2307, 2308]
      }] 
....
}
```

##### recipient table

The group object table contains the array of json objects for an Publisher entry.
note that this table contains the info of the receiving side.
Note that the ia (and path) needs to be defined or the url.
if ia is defined and path is not there, the path will have the default value ".knx".

```bash
....
{
"recipient" : [ 
    {
        "id": "1", ia": 5, "ga":[2305, 2401], "path": ".knx",
    },
    {
        "id": "2","url": "coap://<IP multicast, unicast address or fqdn>/<path>", 
        "ga": [2305, 2306, 2307, 2308]
      }] 
....
}
```

##### access token table

The access token table contains the array of json objects for an auth/at entry.

- id (0): identifier of the entry
- profile (38): oscore (2) 
- scope (9) : either list of integers for group scope or list of access (if) scopes
- the oscore information is in 2 layers: cnf & osc as json objects
- cnf(8):osc(4):id(0) : the oscore identifier
- cnf(8):osc(4):ms(2) : the master secret (32 bytes)
- cnf(8):osc(4):contextId(6) : the OSCORE context id

```bash
{
....
"auth" : [ 
  {
     "id": "id1", 
     "aud": "xx", 
     "profile": 2, 
     "scope": [ 1, 2 , 3 ], 
     "cnf": {"osc" : { "id": "osc_1",  "ms" : "ms_1" }}
  },
  {
      "id": "id2", 
      "aud": "yyy", 
      "profile": 2,
      "scope": [ 4], 
      "cnf": {"osc" : { "id": "osc_2", "ms" : "ms_2" }}
  },
  {
      "id": "id3", 
      "aud": "yyy", 
      "profile": 2,
      "scope": [ if.sec], 
      "cnf": {"osc" : { "id": "osc_3", "ms" : "ms_2" }}
  }
  ] 
....
}
```

##### parameter

The parameters can be set on /p.

```bash
..
 "memparameter": [{
            "href": "/p/P-1/R-1",
            "value": "255"
        }, {
            "href": "/p/P-2/R-2",
            "value": "255"
        }, {
            "href": "/p/MD-2/M-1/MI-1/P-1/R-1",
            "value": "17"
        }, {
            "href": "/p/MD-2/M-1/MI-1/P-2/R-2",
            "value": "1"
        }, {
            "href": "/p/MD-2/M-2/MI-1/P-1/R-1",
            "value": "17"
        }
 ]
....
}
```

##### example files

test_server_config.json

Example file for the test_server_all program
