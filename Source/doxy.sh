#!/bin/sh
java  -Djava.awt.headless=true -jar plantuml.jar -v -o $PWD/images  "./**.(c|cpp|h)"
doxygen
