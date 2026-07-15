# TCP/IP WS25/26 Final Project (HS Fulda)

This repository contains the **Simple Message Broker (SMB)** final project for the TCP/IP course (WS25/26) at Hochschule Fulda.

## Project Overview

The project implements a UDP-based publish/subscribe system:
- **smbbroker**: broker that receives subscriptions and publishes, then forwards matching messages
- **smbpublish**: sends a single publish message
- **smbsubscribe**: subscribes to a topic and receives forwarded messages
- **smbpublish_loop**: periodically sends publish messages
- **smbpublish_interactive**: interactive publisher with runtime topic changes

Topics support hierarchical names (for example `zimmer/temperatur`) and wildcard subscriptions (`#`, `prefix/#`).

## Repository Structure

- `/smb`: source code, Makefile, and detailed project documentation
- `/smb.pdf`: assignment/project reference document

## Build

```bash
cd smb
make all
```

## Run (local example)

In separate terminals:

```bash
cd smb
./bin/smbbroker 8080
```

```bash
cd smb
./bin/smbsubscribe localhost zimmer/#
```

```bash
cd smb
./bin/smbpublish localhost zimmer/temperatur "22.5C"
```

## More Details

See `/smb/README.md` for protocol details, topic rules, and extended usage.