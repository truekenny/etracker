## &nbsp;<img src="https://raw.githubusercontent.com/truekenny/etracker/master/web/apple-touch-icon.png" width="16" height="16"> etracker - open-source BitTorrent tracker

**etracker** aims for maximum stability and minimal resource usage.

**etracker** implements: 
- [The BitTorrent Protocol Specification](https://www.bittorrent.org/beps/bep_0003.html);
- [UDP Tracker Protocol for BitTorrent](https://www.bittorrent.org/beps/bep_0003.html).

### Build

    git clone https://github.com/truekenny/etracker
    cd etracker
	make server	

### Run

    ./etracker -p 6969
    ./etracker --help

The port is indicated simultaneously for TCP and UDP.

#### Shell script

File: [run](https://github.com/truekenny/etracker/blob/master/run). <br>
Example use: `./run 6969`

##### Chroot

Debian 10 example: [run_chroot](https://github.com/truekenny/etracker/blob/master/run_chroot). <br>
Example use: `./run_chroot 6969`

### Interfaces

    http://host:port/announce 
    udp://host:port/announce

### Features

1. IPv4;
1. Listen TCP, UDP;
1. Processing announce, scrape;
1. Variable interval (4-30 minutes, depending on the load average).

### Settings

1. Optional *full scrape* (`data.h` `ENABLE_FULL_SCRAPE: 0 -> 1`).

### Statictics

    http://host:port/stats

### Realtime options

    http://host:port/set?param=value

### Platforms tested on

- Debian 10.5, gcc version 8.3.0;
- MacOS 10.11.6, Apple LLVM version 8.0.0.

### Efficiency

#### CPU usage

**1200 RPS** on one core Xeon (cpu family 6) 2100MHz take **30%**

For best efficiency – all you need one core CPU.

#### Memory usage

Revision | Bytes per torrent | Bytes per peer | Bytes per torrent (compact structures) | Bytes per peer (compact structures) | Startup | **760k** peers + **520k** torrents | **760k** peers + **520k** torrents (compact)
--- | --- | --- | --- | --- | --- | --- | ---
[HEAD](../..) | 124 | 84 | 107 | 75 | 9.2M | 138M | 120M
[0ec61ac](../../commit/0ec61ac54407d99cc59d84df4cb00cff96309936) | 56 | 56 | 48 | 43 | 6.6M | 71M | 59M

0ec61ac torrent = structure torrent<br>
0ec61ac peer = structure peer<br>

HEAD torrent = structure item + hash + structure dataTorrent + structure list<br>
HEAD peer = structure item + hash + structure dataPeer  

*Better code structure comes with a price tag.*

### Similar applications

- [opentracker](https://github.com/masroore/opentracker)

##### Comparison (same pc, rps)

![opentracker](https://raw.githubusercontent.com/truekenny/etracker/master/Pictures/opentracker.png)

Where these peaks come from on the chart - I don't know.
If this were the result of dos scrape requests, then net-out would be clearly visible, but it will not be at all.

### Example output

#### Terminal

    Revision: da52b18                                                                                                                                                                                         
    Starting configuration:                                                                                                                                                                                   
      port = 80                                                                                                                                                                                               
      interval = 239                                                                                                                                                                                          
      workers = 1                                                                                                                                                                                             
      maxPeersPerResponse = 300                                                                                                                                                                               
      socketTimeout = 3                                                                                                                                                                                       
      keepAlive = 1                                                                                                                                                                                           
      minInterval = 239                                                                                                                                                                                       
      maxInterval = 1799                                                                                                                                                                                      
      noTcp = 0                                                                                                                                                                                               
      noUdp = 0                                                                                                                                                                                               
      charset = utf-8                                                                                                                                                                                         
      locale = en_US.UTF-8                                                                                                                                                                                    
    This system has 1 processors available.                                                                                                                                                                   
    Current 7 -> soft=1,024, hard=1,048,576                                                                                                                                                                   
    New 7 -> soft=64,000, hard=1,048,576                                                                                                                                                                      
    Current 4 -> soft=0, hard=18,446,744,073,709,551,615                                                                                                                                                      
    New 4 -> soft=18,446,744,073,709,551,615, hard=18,446,744,073,709,551,615                                                                                                                                 
    Starting UDP worker 0/0                                                                                                                                                                                   
    Waiting UDP for incoming packets...                                                                                                                                                                       
    webRoot: '/web/'                                                                                                                                                                                          
    Starting TCP worker 0/0                                                                                                                                                                                   
    Waiting TCP for incoming connections...                                                                                                                                                                   
    Join TCP Thread                                                                                                                                                                                           
    Wed Oct  7 14:47:17    1475 TP    1402 TT       0 TL       7 MPT       7 MPL       2 MTL       0 RP       0 RT    2492 µs  LA: 0.44 0.48 0.50  ML: 0.50  I:  239-> 239 s  RPS: 719.00/3.00                                                
    Wed Oct  7 14:51:16  276808 TP  165813 TT      11 TL     510 MPT     248 MPL      28 MTL       0 RP     710 RT   88001 µs  LA: 0.46 0.47 0.50  ML: 0.50  I:  239-> 239 s  RPS: 1817.00/8.00
    Wed Oct  7 14:55:15  402633 TP  237029 TT      24 TL     629 MPT     256 MPL      59 MTL       0 RP    1155 RT   98540 µs  LA: 0.34 0.46 0.49  ML: 0.50  I:  239-> 239 s  RPS: 1810.00/9.00
    Wed Oct  7 14:59:14  465851 TP  284450 TT      34 TL     671 MPT     244 MPL      74 MTL       0 RP    1071 RT  111428 µs  LA: 0.31 0.37 0.44  ML: 0.50  I:  239-> 239 s  RPS: 1631.00/6.00
    Wed Oct  7 15:03:13  523564 TP  328667 TT      37 TL     701 MPT     257 MPL     101 MTL       0 RP     978 RT  111912 µs  LA: 0.72 0.50 0.48  ML: 0.50  I:  239-> 239 s  RPS: 1831.00/10.00
    Wed Oct  7 15:07:12  580239 TP  371851 TT      41 TL     735 MPT     255 MPL     136 MTL       0 RP     918 RT  136560 µs  LA: 0.30 0.40 0.44  ML: 0.50  I:  239-> 239 s  RPS: 1778.00/4.00
    
- `TP` – Total Peers;
- `TT` – Total Torrents;
- `TL` – Torrents with expanded peer's list;
- `MPT` – Max Peers for single torrent;
- `MPL` – Max Peers for single list;
- `MTL` – Max Torrents for single list;
- `RP` – Removed Peers by garbage collector;
- `RT` – Removed Torrents by garbage collector;
- `µs` - Time spent on garbage collect;
- `LA` - Load Average;
- `ML` - Max Allowed Load Average;
- `I` - Interval;
- `RPS` - Requests per second (TCP/UDP).

#### Stats page

    start_time = Wed Oct  7 14:47:16 2020 (0d) (0f)
    thread_number = 0
    
    load_avg = 0.32 0.41 0.42
    interval = 239
    active_sockets = 1,055 (rlimit 64,000/1,048,576)
    
    requests_per_second = tcp: 1373.00, udp: 4.00
    
    rusage.ru_maxrss =      171,312
    
    malloc        =      59,760,492
    calloc        =      91,548,937
    *alloc        =     151,309,429
    free          =     147,722,712
    *alloc - free =       3,586,717
    
    stats.http_200 =   25,410,251
    stats.http_400 =      271,504
    stats.http_401 =            0
    stats.http_403 =            1 (Full Scrape)
    stats.http_404 =          511
    stats.http_405 =          162 (Not GET)
    stats.http_408 =    2,892,382 (Timeout)
    stats.http_413 =           82 (Oversize)
    
    stats.close_pass  =   26,281,984
    stats.send_pass   =   28,574,705
    stats.recv_pass   =   25,682,430
    stats.accept_pass =   26,283,039
    
    stats.close_failed  =            0
    stats.send_failed   =          188
    stats.recv_failed   =       86,090
    stats.accept_failed =            0
    
    stats.recv_failed_read_0         =   14,497,481
    stats.recv_failed_read_sub_0     =       86,008
    stats.recv_failed_read_not_equal =            0
    
    stats.send_pass_udp =      133,397
    stats.recv_pass_udp =      133,931
    
    stats.send_failed_udp =            0
    stats.recv_failed_udp =            0
    
    stats.keep_alive    =   16,876,398
    stats.no_keep_alive =    8,805,869
    
    stats.sent_bytes =   9,444,661,553
    stats.recv_bytes =   9,049,406,160
    
    stats.sent_bytes_udp =       6,739,906
    stats.recv_bytes_udp =       7,555,839
    
    stats.announce =   24,186,205
    stats.scrape   =    1,495,343
    
    stats.connect_udp  =       66,372
    stats.announce_udp =       62,177
    stats.scrape_udp   =        4,848

### Author

I am not a C developer. This application is my hobby and my first program on C.
