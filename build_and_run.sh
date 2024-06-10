#!/bin/bash
g++ receiver_shared.cpp -lrt -o receiver_shared
g++ allocator_shared.cpp -lrt -o sender_shared

./sender_shared &
sleep 1
./receiver_shared