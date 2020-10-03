# Encode Metadata 

Encode Unregistered user data(SEI) into H.264 bitstream

## Dependencies
- **Opencv 3.2**
- **Libav dependencies:** The easiest way to get Libav dependencies is to download ffmpeg from source. The following link provides detailed instructions: https://trac.ffmpeg.org/wiki/CompilationGuide/Ubuntu.
You can also try to directly install FFmpeg using apt-get,however this has not been tested.

## Usage 
- g++ encode_video_opencv.cpp -o encode_video_opencv `pkg-config --cflags --libs opencv` -lavutil -lavformat -lavcodec -lswscale -lz -lavdevice -Wall
- On running the executable, test.h264 is created(This can be played using VLC).

- The bitstream can be parsed by using the following command:
ffmpeg -i test.h264 -c copy -bsf:v trace_headers -f null - 2> NALUS.txt

- **Additional Information:** 
     - NALUS.txt would contain the inserted SEI information(timestamp) as sequence of ASCII codes.
     - bitstream_parser_output.txt is the result of parsing the test.h264 videofile using the tool h264bitstream.







