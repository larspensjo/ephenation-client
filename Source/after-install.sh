# This is a script that is executed after installation of the deb package
echo Running after-install.sh...
udevadm trigger --subsystem-match=hidraw
