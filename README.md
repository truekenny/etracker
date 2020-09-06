## sc6 - open-source BitTorrent tracker

**sc6** aims for maximum stability and minimal resource usage.

**sc6** implements 
[The BitTorrent Protocol Specification](https://www.bittorrent.org/beps/bep_0003.html),
[UDP Tracker Protocol for BitTorrent](https://www.bittorrent.org/beps/bep_0003.html).

### Build

    git clone git@github.com:truekenny/sc6.git
	make server	

### Run

    ./server.o port

Example: `./server.o 6969`
The port is indicated simultaneously for TCP and UDP.

### Interfaces

- http://host:port/announce 
- udp://host:port/announce

### Features

1. IPv4;
1. Listen TCP / UDP;
1. Processing *announce*, *scrape*;

### Settings

1. Optional *full scrape* (`data_render.h` `ENABLE_FULL_SCRAPE: 0 -> 1`).
1. Optional *interval* (`data_structure.h` `INTERVAL 1800`)

### Statictics

- http://host:port/stats

### Platforms tested on

- Debian 10.5, gcc version 8.3.0
- MacOS 10.11.6, Apple LLVM version 8.0.0

### Efficiency

On Xeon (cpu family 6) 2100MHz, one core<br>
500 request per second (stored: 620k peers, 520k torrents)

- 33% cpu
- 105 MB

### Similar applications

- [opentracker](https://github.com/masroore/opentracker).

##### Comparison (same pc, rps)

![opentracker](https://raw.githubusercontent.com/truekenny/sc6/master/Pictures/opentracker.png)

Where these peaks come from on the chart - I don't know.
If this were the result of dos scrape requests, then net-out would be clearly visible, but it will not be at all.

### Author

I am not a C developer. This application is my hobby and my first program on C.
