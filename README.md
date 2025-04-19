# oresat-canopend

A CANopen system service for OreSat Linux cards.

## Setup

Install build dependencies

```bash
pip install -r requirements.txt
```

Initialize CANopenLinux git submodule

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

Run with only the internal OD (Object Dictionary).

```bash
./oresat-canopend can0
```

## Run with an od config

A `od.conf` file is a file to describe the extra objects to add the standard
CANopend OD.

OreSat `od.conf` files can be generated with `oresat-configs` from a project's
`od.yaml` (the yaml lives in project repos).

```bash
oresat-configs canopend-config od.yaml
```

Run with the generated conf.

```bash
./oresat-canopend can0 -o od.conf
```

**Note:** If the conf fails to load for any reason, the internal OD will still
be used.
