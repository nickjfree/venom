# Venom

 Venom is an udp based tun implementation.  Deadlly simple but has incredible powers.
 
### Server Setup

 compile tun.c
```sh
$ gcc tun.c -o tun
```
just run it
```sh
$ ./tun
```
At this point, v has created a "tun1" interface on your server.
setup ip address for tun1 and enable iptables for tun1 to eth0 routing
```sh
$ ip link set tun1 up
$ ip addr add 192.168.0.2/24 dev tun1
$ iptables --table nat --append POSTROUTING --out-interface tun1 -j MASQUERADE
$ iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
```
done

### Client Setup
  Windows

  - Firtst install Tap-windows driver (NDIS 6)
  - Then run venom
```sh
 c:\> start venom [ServerIP]
```
Now you can find a "TAP-Windows Adapter V9" adapter in your adapter list and it should be up and running.
The default ip address for this adapter is set to 192.168.0.1 . Then you can setup routing to make 192.168.0.2, the server address as your default gateway and delete the old default gw. This makes all the traffic goes through the TAP interface except the real udp traffic.
```sh
 c:\> route add [serverip]/32 [your default gw]
 c:\> route add 0.0.0.0/0 192.168.0.2 
 c:\> route delete 0.0.0.0/0 [your default gw]
```

You may asle need to do the follwing to disable windows using the dafault gw.

1. Open the Properties of your default network adapter.
2. Open the properties of Internet Protocol Version 4 (TCP/IPv4).
3. Click on Advanced.
4. Uncheck Automatic Metric and set the Interface Metric value to a high number, say 2000.
5. Hit OK until you close the screens.

Done. Enjoy it. 

Linux

run with one argument as a client
```sh
 $ ./tun [ServerIP]
```

set tun'ip address to 192.168.0.1 

```sh
$ ip link set tun1 up
$ ip addr add 192.168.0.2/24 dev tun1
```

After adding some fancy routings. 
Done.

Just remember. Anytime you get lost a good dns server always saves your ass.

#### Limitations

  - The windows version use a hardcoded 192.168.0.1 as the local client address. so you may need to change it in the code.
  - Be carefull with the routing setup on windows you may lose network connections. Be happy to restart.

 

