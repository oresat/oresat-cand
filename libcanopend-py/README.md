# oresat-libcanopend (Python)

Python library to interface with oresat-canopend.

## Example

```python
from time import sleep

from oresat_canopend import NodeClient

client = NodeClient()

while True:
    value = client.od_read(0x4123, 0x5, Datatype.UNSIGNED16)
    output = do_things(value)
    client.od_write(0x4123, 0x7, Datatype.UNSIGNED32, output)
    sleep(1)
```
