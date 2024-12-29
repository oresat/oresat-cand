# oresat-canopend

A CANopen system service for OreSat Linux cards.

## Setup

Install dependencies

```bash
sudo apt install meson
```

Initialize CANopenLinux submodule

```bash
git submodule udpate --init --recursive
```

Build

```bash
meson setup build
```

## Run

Compile

```bash
cd build/
meson compile
```

Run

```bash
./oresat-canopend can0
```
