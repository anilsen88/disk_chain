# disk_chain

## Overview
This is a project aimed at enhancing secure boot processes. It provides tools for managing boot configs, including NVRAM emulation, key injection, and rollback features.

## Getting Started
Clone the repo:
```bash
git clone https://github.com/anilsen88/disk_chain.git
cd disk_chain
```

## Features

disk_chain has NVRAM emulation which simulates NVRAM using SQLite for persistent variable storage. This allows for easy management and retrieval of configuration variables to make sure that they remain available across sessions

It also includes key injection so you can enable the injection of cryptographic keys directly into memory. This is important for scenarios where secure keys must be managed dynamically during the boot process

Additionally, the rollback feature restores previous variable states from the database allowing you to revert changes and maintain system integrity

## Usage
Check the source files for usage examples. Each component has its own logic and can be tested individually :)
