## Build
gcc -Wall rtcp_extract.c -o rtcp_extract $(pkg-config --cflags --libs gstreamer-1.0)

