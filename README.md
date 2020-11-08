## About
Simple DNS proxy-server that redirects DNS requests to system call `getaddrinfo`. The protocol is simple: server accepts continuous list of domain names and sends the lists of ip addresses that relate to above domain names. The server can handle multiple clients simultaneously and there are limits such as: active connections per ip and maximal request size.

## Protocol described
 - __Client__ sends domain names separated by `CRLF`
 - __Server__ responds with _ip_ addresses separated by `CRLF`

The _telnet_ is good choice for testing.
 
## Build
Due to usage of syscalls, build is only possible on _linux_ systems.
```shell-script
mkdir -p cmake-build && cd cmake-build
cmake -G "Unix Makefiles" .. && make
```

## Run
Server will be available at __127.0.0.1:1337__
```shell-script
cmake-build/dns-lookup-server
```
