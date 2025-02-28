# oresat-libcanopend (Python)

Python library to interface with oresat-canopend.

## Install

```bash
pip install .
```

## Example

```python
from time import sleep

from oresat_canopend import Entry, NodeClient

INPUT_ENTRY_1 = Entry(0x4123, 0x2, Datatype.BOOLEAN, False)
INPUT_ENTRY_2 = Entry(0x4123, 0x5, Datatype.UNSIGNED16, 0)
OUTPUT_ENTRY = Entry(0x4123, 0x7, Datatype.UNSIGNED32, 0)

client = NodeClient()

while True:
    input_1 = client.od_read(INPUT_ENTRY_1)
    input_2 = client.od_read(INPUT_ENTRY_2)

    output = do_things(input_1, input_2))

    client.od_write(OUTPUT_ENTRY, output)

    sleep(1)
```
