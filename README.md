# Postgres Mass Spectrometry Extension

![GitHub release (latest SemVer)](https://img.shields.io/github/v/release/marekmosna/pgms)
[![Build Status](https://github.com/marekmosna/pgms/workflows/CI/badge.svg)](https://github.com/marekmosna/pgms/actions)

The extension was developed and tested on Linux Ubuntu 20.04.3 against Postgresql 12.9

## Requirements

On Debian based Linux distributions install following packages
```bash
sudo apt-get install git postgresql build-essential
```
For Debian 11

```bash
sudo apt install postgresql-server-dev-13
```

For Ubuntu 20.04

```bash
sudo apt install postgresql-server-dev-12
```

## Build

```bash
git clone https://github.com/marekmosna/pgms.git
cd pgms
make
```

## Install

```bash
sudo make install
```

## Test
```bash
make installcheck PGUSER=postgres
```

