# oresat-cand

A CAN system service for OreSat Linux cards.

## Setup

Install dependencies

```bash
# Arch Linux
sudo pacman -S zeromq meson ninja

# Debian
sudo apt install libzmq3-dev meson ninja-build
```

Install oresat-configs

```bash
pip install --user oresat-configs
```

Initialize [CANopenLinux] git submodule

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

## Unit Tests

Unit tests are done with [Ceedling].

To install Ceedling, install Ruby with your system package manager

```bash
# Arch Linux
sudo pacman -S ruby ruby-stdlib

# Debian Linux
sudo apt install ruby-full
```

Install Ceedling with Gem (Ruby package manager)

```bash
gem install ceedling
```

Run Ceedling

```bash
ceedling test:all
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

[Ceedling]:https://github.com/ThrowTheSwitch/Ceedling
[CANopenLinux]:https://github.com/CANopenNode/CANopenLinux
