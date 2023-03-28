#!/bin/bash

echo "Enter name: "
read name
echo "Enter version: "
read ver
echo "Enter version type (release/debug): "
read type

g++ -o $name"_"$type"_v"$ver vkSaver.cpp -I/usr/include/jsoncpp -ljsoncpp -lcurl
sudo chmod +x $name"_"$type"_v"$ver
./$name"_"$type"_v"$ver
