# Shared memory example 

This is a small example that showcases how to send a buffer between two processes using shared memory and zero copy.
It also works if the filedesriptor is allocated elsewhere (for example gstreamer).


To run do
```
./build_and_run.sh
```

Expected output
```
Waiting for a connection...
Received fd: 4
Message from Process A: Hello from Process A!
```
