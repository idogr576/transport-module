# Transport Module
## Build
```bash
./build.sh
```

## Test
```bash
# send "hello world!" packet to localhost at port 1337
echo 127.0.0.1 1337 hello world! | sudo tee /dev/udp
# check kernel logs for more debug info
journalctl --since "1 minute ago" | grep kernel
```
