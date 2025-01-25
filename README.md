# oresat-canopend

A CANopen system service for OreSat Linux cards.

## Setup

Install build dependencies

```bash
pip install meson ninja
```

Initialize CANopenLinux submodule

```bash
git submodule update --init --recursive
```

Build

```bash
meson setup build
```

Compile

```bash
cd build/
meson compile
```

## Run

Run with the internal OD (Object Dictionary).

```bash
./oresat-canopend can0
```

## Run with a dcf

A `.dcf` file is a `.conf`-like file to describe a OD. `oresat-canopend` can load
in a dcf to use an app-specific OD instead of the internal OD.

OreSat dcfs can be generated with `oresat-configs` (in this case the oresat gps dcf).

```bash
oresat-configs dcf gps
```

Run with the generated dcf.

```bash
./oresat-canopend can0 -d gps.dcf
```

**Note:** If the dcf fails to load for any reason, the internal OD will be used
as a backup.
