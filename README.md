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
1. Listen TCP, UDP;
1. Processing announce, scrape;
1. Variable interval (4-30 minutes, depending on the load average)

### Settings

1. Optional *full scrape* (`data_render.h` `ENABLE_FULL_SCRAPE: 0 -> 1`).

### Statictics

- http://host:port/stats

### Platforms tested on

- Debian 10.5, gcc version 8.3.0
- MacOS 10.11.6, Apple LLVM version 8.0.0

### Efficiency

- **1200 RPS** on one core Xeon (cpu family 6) 2100MHz take **30%**
- **760k** peers + **520k** torrents take **91 MByte**

For best efficiency – all you need one core CPU.

### Similar applications

- [opentracker](https://github.com/masroore/opentracker).

##### Comparison (same pc, rps)

![opentracker](https://raw.githubusercontent.com/truekenny/sc6/master/Pictures/opentracker.png)

Where these peaks come from on the chart - I don't know.
If this were the result of dos scrape requests, then net-out would be clearly visible, but it will not be at all.

### Example output

#### Terminal

    Starting configuration: port = 80, interval = 299
    This system has 1 processors configured and 1 processors available.
    Garbage data thread NO change priority 0 -> 5
    Garbage socket pool thread NO change priority 0 -> 5
    Starting UDP worker 0/0
    Starting TCP worker 0/0
    Join TCP Thread
    Waiting UDP for incoming packets...
    Waiting TCP for incoming connections...
    Mon Sep 14 13:02:19 GRBG:       0 TP       0 TT       0 MP       0 MT       0 RP       0 RT    5144 µs  LA: 0.21 0.31 0.29  ML: 0.50  I:  299-> 299 s  RPS: 0.00                                         
    Mon Sep 14 13:17:19 GRBG:  313789 TP  209763 TT     728 MP      39 MT       0 RP    1380 RT   42281 µs  LA: 0.35 0.36 0.31  ML: 0.50  I:  299-> 299 s  RPS: 934.00                                       
    Mon Sep 14 13:32:19 GRBG:  432703 TP  301025 TT    1294 MP      83 MT       0 RP    1441 RT   54135 µs  LA: 0.40 0.33 0.29  ML: 0.50  I:  299-> 299 s  RPS: 1028.00                                      
    Mon Sep 14 13:47:19 GRBG:  518963 TP  346012 TT    1547 MP      83 MT       0 RP    1332 RT   61624 µs  LA: 0.31 0.27 0.27  ML: 0.50  I:  299-> 299 s  RPS: 1176.00                                      
    Mon Sep 14 14:02:19 GRBG:  606304 TP  390698 TT    1776 MP     113 MT       0 RP    1525 RT   67142 µs  LA: 0.22 0.26 0.25  ML: 0.50  I:  299-> 299 s  RPS: 1139.00                                      
    Mon Sep 14 14:17:19 GRBG:  628489 TP  411405 TT    1994 MP     113 MT   62860 RP   22438 RT   92262 µs  LA: 0.31 0.27 0.26  ML: 0.50  I:  299-> 299 s  RPS: 1096.00                                      
    Mon Sep 14 14:32:19 GRBG:  646326 TP  428162 TT    1978 MP     112 MT   67572 RP   26730 RT   94163 µs  LA: 0.11 0.16 0.21  ML: 0.50  I:  299-> 299 s  RPS: 1170.00                                      
    Mon Sep 14 14:47:19 GRBG:  670317 TP  448315 TT    1980 MP     113 MT   62177 RP   24205 RT  107990 µs  LA: 0.34 0.33 0.28  ML: 0.50  I:  299-> 299 s  RPS: 1255.00                                      
    Mon Sep 14 15:02:19 GRBG:  691101 TP  465761 TT    2009 MP      89 MT   68942 RP   30134 RT  116955 µs  LA: 0.48 0.36 0.29  ML: 0.50  I:  299-> 299 s  RPS: 1235.00                                      
    Mon Sep 14 15:17:19 GRBG:  711483 TP  480333 TT    2006 MP      72 MT   68684 RP   30954 RT  116881 µs  LA: 0.43 0.40 0.33  ML: 0.50  I:  299-> 299 s  RPS: 1270.00                                      
    Mon Sep 14 15:32:19 GRBG:  729638 TP  491629 TT    2020 MP      81 MT   69394 RP   30973 RT  125544 µs  LA: 0.28 0.36 0.36  ML: 0.50  I:  299-> 299 s  RPS: 1305.00                                      
    Mon Sep 14 15:47:19 GRBG:  742089 TP  501496 TT    2039 MP      84 MT   72357 RP   31561 RT  121588 µs  LA: 0.23 0.27 0.31  ML: 0.50  I:  299-> 299 s  RPS: 1262.00                                      
    Mon Sep 14 16:02:20 GRBG:  749703 TP  509232 TT    2063 MP      83 MT   75393 RP   32606 RT  123046 µs  LA: 0.29 0.29 0.28  ML: 0.50  I:  299-> 299 s  RPS: 1373.00                                      
    Mon Sep 14 16:17:20 GRBG:  757180 TP  516118 TT    2090 MP      67 MT   71700 RP   30877 RT  120166 µs  LA: 0.65 0.42 0.36  ML: 0.50  I:  299-> 299 s  RPS: 1285.00                                      
    Mon Sep 14 16:32:20 GRBG:  763808 TP  519396 TT    2112 MP      77 MT   72696 RP   32996 RT  133611 µs  LA: 0.25 0.28 0.31  ML: 0.50  I:  299-> 299 s  RPS: 1319.00                                      
    
- `TP` – Total Peers
- `TT` – Total Torrents
- `MP` – Max Peers for single torrent
- `MT` – Max Torrents for single list
- `RP` – Removed Peers by garbage collector
- `RT` – Removed Torrents by garbage collector
- `LA` - Load Average
- `ML` - Max Allowed Load Average
- `I` - Interval
- `RPS` - Request per seconds (TCP+UDP)

#### Stats page

    start_time = Mon Sep 14 13:02:19 2020
    thread_number = 0
    
    Load Avg = 0.29 0.32 0.31
    Interval = 299
    Active sockets: 289 (rlimit 64,000/1,048,576)
    
    Request per second ~ 1265.00
    
    rusage.ru_maxrss =       91,400
    rusage.ru_ixrss  =            0
    rusage.ru_idrss  =            0
    rusage.ru_isrss  =            0
    
    Malloc =            3
    Calloc =  141,206,870
    *alloc =  141,206,873
    free   =  139,860,090
    *alloc - free =    1,346,783
    
    stats.http_200 =   14,674,338
    stats.http_400 =      186,890
    stats.http_403 =            5 (Full Scrape)
    stats.http_404 =          839
    stats.http_405 =           80 (Not GET)
    stats.http_408 =      885,892 (Timeout)
    stats.http_413 =            1 (Oversize)
    
    stats.close_pass  =   15,748,042
    stats.send_pass   =   15,719,309
    stats.recv_pass   =   14,862,153
    stats.accept_pass =   15,748,334
    
    stats.close_failed  =            3
    stats.send_failed   =       28,736
    stats.recv_failed   =            1
    stats.accept_failed =            0
    
    stats.send_pass_udp =       37,727
    stats.recv_pass_udp =       38,129
    
    stats.send_failed_udp =            0
    stats.recv_failed_udp =            0
    
    stats.keep_alive    =            0
    stats.no_keep_alive =   14,862,072
    
    stats.sent_bytes =   3,517,540,558
    stats.recv_bytes =   5,285,457,997
    
    stats.sent_bytes_udp =       1,476,968
    stats.recv_bytes_udp =       2,081,336
    
    stats.announce =   14,045,611
    stats.scrape   =      815,558
    
    stats.connect_udp  =       19,050
    stats.announce_udp =       16,192
    stats.scrape_udp   =        2,485

### Author

I am not a C developer. This application is my hobby and my first program on C.
