INSTRUCTIONS TO COMPILE encode_video_opencv.cpp
---------------------------------------------------------------------------------------------------------------------------------------------
g++ encode_video_opencv.cpp -o encode_video_opencv `pkg-config --cflags --libs opencv` -lavutil -lavformat -lavcodec -lswscale -lz -lavdevice -Wall
---------------------------------------------------------------------------------------------------------------------------------------------
On running the executable, test.h264 is generated
The bitstream can be parsed by using the following command:
ffmpeg -i test.h264 -c copy -bsf:v trace_headers -f null - 2> NALUS.txt
------------------------------------------------------------------------------------------------------------------
