#!/bin/sh
echo "Bash Script to Start 4 Servers"
echo "Creating Root Directory for Server"
mkdir ./DFS
echo "Making Clean of old files"
make clean
echo "Making all of all files"
make all
echo ""
./dfserv ./DFS1 10001 &
./dfserv ./DFS1 10001 &
./dfserv ./DFS3 10003 &
./dfserv ./DFS4 10004 &
echo "Run the client now"
