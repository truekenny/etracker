
var_dump(pack('Nn', ip2long('49.50.51.52'), 256 * 53 + 54));
// 123456

var_dump(long2ip(16777343));
var_dump(ip2long('127.0.0.1'));

// string(9) "1.0.0.127"
// int(2130706433)

INT:2130706433 - HEX:7F 00 00 01 - CHAR:????
INT:12345 - HEX:30 39 - CHAR:09

// ip to long
https://www.smartconversion.com/unit_conversion/IP_Address_Converter.aspx
// num to hex
https://www.rapidtables.com/convert/number/decimal-to-hex.html?x=12345
// ascii
https://upload.wikimedia.org/wikipedia/commons/thumb/4/4f/ASCII_Code_Chart.svg/1200px-ASCII_Code_Chart.svg.png


"GET /announce?info_hash=D%09%f6H%b0%ef%db%f0%40%0b%be%24%b7y%87%b9%1eY%e42&peer_id=-qB4210-mDyDc3Ju_I5Y&port=4096&uploaded=0&downloaded=0&left=451936256&corrupt=0&key=3AF13DE0&event=stopped&numwant=0&compact=1&no_peer_id=1&supportcrypto=1&redundant=0 HTTP/1.1\r\nHost: 192.168.144.5:3000\r\nUser-Agent: qBittorrent/4.2.1\r\nAccept-Encoding: gzip\r\nConnection: close\r\n\r\n"

curl -i "http://he/announce?info_hash=12345678901234567890&event=started&port=12345&peer_id=11111222223333344444"
d8:completei0e10:downloadedi0e10:incompletei1e8:intervali1720e12:min intervali860e5:peers6:Oov;09e

curl -i "http://tracker.bt4g.com:2095/announce?info_hash=12345678901234567890&event=started&port=12345&peer_id=11111222223333344444&left=10&downloaded=10&uploaded=10"
curl -i "http://tracker.bt4g.com:2095/announce?info_hash=12345678901234567890&event=started&port=12345&peer_id=11111222223333344444&left=10&downloaded=10&uploaded=10&no_peer_id=1"
d8:completei0e10:incompletei2e8:intervali1200e12:min intervali900e5:peersld4:porti12345e7:peer id20:111112222233333444442:ip13:93.157.234.32eee

curl -i "http://tracker.bt4g.com:2095/announce?info_hash=12345678901234567890&event=started&port=12345&peer_id=11111222223333344444&left=10&downloaded=10&uploaded=10&compact=1"
d8:completei0e10:incompletei2e8:intervali1200e12:min intervali900e5:peers6:]?? 09e

curl -i "http://tracker.files.fm:6969/announce?info_hash=12345678901234567890&event=started&port=12345&peer_id=11111222223333344444&left=10&downloaded=10&uploaded=10"
d8:completei0e10:downloadedi0e10:incompletei1e8:intervali1856e12:min intervali928e5:peers6:?EA?09e

curl -i "http://tracker.noobsubs.net:63000/announce?info_hash=12345678901234567890&event=started&port=12345&peer_id=11111222223333344444&left=10&downloaded=10&uploaded=10"
d8:completei0e10:downloadedi0e10:incompletei1e8:intervali1924e12:min intervali962e5:peers6:Oov;09e

d14:failure reason7:messagee

curl -i "http://tracker.bt4g.com:2095/scrape?info_hash=12345678901234567890&event=started&port=12345&peer_id=11111222223333344444&left=10&downloaded=10&uploaded=10&compact=1"
d5:filesd20:12345678901234567890d8:completei0e10:incompletei0eeee

d
5:files
    d
    20:12345678901234567890
        d
        8:complete i0e
        10:incompletei0e
        e
    e
e

curl -i "http://tracker.bt4g.com:2095/scrape?info_hash=12345678901234567890&info_hash=02345678901234567890"
d5:filesd20:12345678901234567890d8:completei0e10:incompletei0ee20:02345678901234567890d8:completei0e10:incompletei0eeee

d
5:files
    d
    20:12345678901234567890
        d
        8:completei0e
        10:incompletei0e
        e
    20:02345678901234567890
        d
        8:completei0e
        10:incompletei0e
        e
    e
e
