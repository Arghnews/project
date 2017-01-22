#!/bin/bash

cmd='g++  -I/modules/cs324/glew-1.11.0/include -O3 -std=c++11 -L/usr/X11R6/lib -L/modules/cs324/glew-1.11.0/lib -Wl,-rpath,/modules/cs324/glew-1.11.0/lib Shape.cpp Cuboid.cpp AABB.cpp Octtree.cpp arm.cpp State.cpp Movement.cpp -lglut -lGL -lGLU -lX11 -lm -lGLEW -o arm'
echo $cmd
$cmd
[ $? -eq 0 ] && ./arm
