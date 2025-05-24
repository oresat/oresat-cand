# oresat-cand

A CAN system service for OreSat Linux cards.

## Setup

Install dependencies

```bash
sudo apt install libzmq3-dev meson ninja-build
```

Install oresat-configs

```bash
pip install --user oresat-configs
```

Initialize CANopenLinux git submodule

```bash
git submodule update --init --recursive
```

Setup build directory

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
./oresat-cand -i can0
```

## Run with an od config

A `od.csv` file is a file to describe the extra objects to add the standard
CANopen OD.

OreSat `od.csv` files can be generated with `oresat-configs` from a project's
`od.yaml` (the yaml lives in project repos).

```bash
oresat-configs cand-config od.yaml
```

Run with the generated conf.

```bash
./oresat-cand -i can0 -o od.csv
```

**Note:** If the csv fails to load for any reason, the internal OD will still
be used.

## Build Debian package

Install dependencies

```bash
sudo apt install debhelper build-essential dh-make

```

Build deb package

```bash
./makedeb.sh
```
