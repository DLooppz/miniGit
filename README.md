# miniGit
Implementation of a dummy git with server/cloud storage

## Notes
**zLib Usage Example**

[Link](https://zlib.net/manual.html) - Edu whatsapp link

**Is it possible to establish a connection between two PCs (not in LAN)?**

[Link](https://stackoverflow.com/questions/18021189/how-to-connect-two-computers-over-internet-using-socket-programming-in-c) - Short answert, No

**Multithreading server using TCP/IP**

[Link](https://dzone.com/articles/parallel-tcpip-socket-server-with-multi-threading) - Very nice explanation and code

**How to transfer files using UDP**

[Link](https://www.geeksforgeeks.org/c-program-for-file-transfer-using-udp/) - I think this will be important

**Some usefull things about forking and threading**

[Link](https://stackoverflow.com/questions/16354460/forking-vs-threading) - This is not so important. I leave it here just in case

## Task list
- [x] Transfer files between client and server using TCP/IP with multiple threads (i.e. with SOCK_STREAM)
- [x] Logic of miniGit
- [x] Minimal example of one client, one socket and two threads.
- [x] More complex example using two threads per client/socket
- [ ] Logic of data and command sockets in thread @klostermati
- [ ] Simple login @klostermati
- [ ] Minimal example of: compress, send, receive and uncompress file through a socket @DLooppz
- [ ] Modify .h of miniGit logic @DLooppz
