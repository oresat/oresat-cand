# oresat-cand (Python)

Python library to interface with the `oresat-cand` daemon.

## Install

```bash
pip install .
```

## Example

```python
from time import sleep

from oresat_cand import Entry, NodeClient


class CardEntry(Entry)
    INPUT_1 = 0x4123, 0x2, Datatype.BOOLEAN, False
    INPUT_2 = 0x4123, 0x5, Datatype.UNSIGNED16, 0
    OUTPUT = 0x4123, 0x7, Datatype.UNSIGNED32, 0


client = NodeClient()

while True:
    input_1 = client.od_read(CardEntry.INPUT_1)
    input_2 = client.od_read(CardEntry.INPUT_2)

    output = do_things(input_1, input_2)

    client.od_write(CardEntry.OUTPUT, output)

    sleep(1)
```
