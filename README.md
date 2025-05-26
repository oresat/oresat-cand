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

```bash
./oresat-cand -i can0
```

## Build Debian package

Install dependencies

```bash
sudo apt install debhelper build-essential dh-make

```

Build deb package

```bash
./makedeb.sh
```
