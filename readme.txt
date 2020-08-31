sem:
https://stackoverflow.com/questions/17202931/shared-object-in-pthread
https://stackoverflow.com/questions/27736618/why-are-sem-init-sem-getvalue-sem-destroy-deprecated-on-mac-os-x-and-w

info_hash 20
event stopped started completed
port
ip
peer_id 20
compact bool
no_peer_id bool

-uploaded num
-downloaded num
-left num



var_dump(pack('Nn', ip2long('49.50.51.52'), 256 * 53 + 54));
// 123456

var_dump(long2ip(16777343));
var_dump(ip2long('127.0.0.1'));

// string(9) "1.0.0.127"
// int(2130706433)

"GET /announce?info_hash=D%09%f6H%b0%ef%db%f0%40%0b%be%24%b7y%87%b9%1eY%e42&peer_id=-qB4210-mDyDc3Ju_I5Y&port=4096&uploaded=0&downloaded=0&left=451936256&corrupt=0&key=3AF13DE0&event=stopped&numwant=0&compact=1&no_peer_id=1&supportcrypto=1&redundant=0 HTTP/1.1\r\nHost: 192.168.144.5:3000\r\nUser-Agent: qBittorrent/4.2.1\r\nAccept-Encoding: gzip\r\nConnection: close\r\n\r\n"