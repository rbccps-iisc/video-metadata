
/*
 * @file main.cpp
 * @author Varghese, Vishal, Srikrishna (bacharya@iisc.ac.in)
 * @brief
 * Encode Supplemental Enhancement Information(SEI) into NAL units
 * of the H.264 Byte-Stream
 * Tested using integrated laptop camera
 * @version 0.1
 * @date 2021-07-25
 *
 * Encode SEI data into NAL units with H.264 encoding
 * Raw frames are obtained using opencv and are encoded with metadata using H.264 standard
 * Flow: Initialize codec info--> Capture raw data using OpenCV VideoCapture
 * --> Convert from AV_PIX_FMT_BGR24 to AV_PIX_FMT_YUV_420P
 * -->Encode SEI info to video stream
 *
 * @copyright Copyright (c) 2021
 */

#include <iostream>
#include <vector>
#include <ctime>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <sstream>


// OpenCV
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

//FFmpeg
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}

#include "h264_sei_pack.h"

//Macros
#define SWS_BICUBIC 4
using namespace std;
//For debug
void breakpoint()
{
    /**
    * Breakpoint
    *
    */
    char ch;
    cout<<"Waiting for input..."<<endl;
    cin>>ch;
}

void display_image(string winname,cv::Mat img)
{
    /**
     * @brief Display image in a new cv::named Window object
     *
     */
    //cv::namedWindow(winname,CV_WINDOW_NORMAL);
    cv::namedWindow(winname,cv::WINDOW_NORMAL);
    cv::imshow(winname,img);
    cv::waitKey(1);
}
//---------------------------------------Function prototypes-----------------------------------------//
static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,
                   FILE *outfile);
char* get_timestamp();
char* get_microseconds();
//---------------------------------------------------------------------------------------------------//

class metadata_encode
{
    /**
     * @brief Utils for encoding metdata
     *
     */
public:
    //Data Members
    const char *filename, *codec_name;
    const AVCodec *codec;
    AVCodecContext *c= NULL;
    int i, ret, x, y;
    FILE *f;
    AVFrame *frame;
    AVPacket *pkt;
    uint8_t endcode[4] = { 0, 0, 1, 0xb7 };
    SwsContext* swsctx = NULL;

    const int dst_width = 640;
    const int dst_height = 480;
    const AVRational dst_fps = {30,1};

    //Member functions
    int initialize_codec();
    void capture_data();
};

int metadata_encode::initialize_codec()
{

    /**
     * @brief Initialize codec parameters for H.264 Encoding
     *
     */
    codec_name = "libx264";
    filename = "test.h264";
    av_register_all();

    codec = avcodec_find_encoder_by_name(codec_name);
    if(!codec)
    {
        std::cerr << "Codec"<<codec_name<<"not found"<<endl;
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c)
    {
        std::cerr<<"Could not allocate video codec context"<<endl;
        exit(1);
    }

    pkt = av_packet_alloc();
    if(!pkt)
    {
        exit(1);
    }
    //encoder codec params

    c->width = dst_width;
    c->height = dst_height;
    c->pix_fmt = AV_PIX_FMT_YUV420P;

    c->framerate = dst_fps;
    c->gop_size = 10;


    //control rate
    c->bit_rate = 2 * 1000 * 1000;
    c->max_b_frames = 1;
    c->rc_buffer_size = 4 * 1000 * 1000;
    c->rc_max_rate = 2 * 1000 * 1000;
    c->rc_min_rate = 2.5 * 1000 * 1000;

    //time base
    c->time_base = av_inv_q(dst_fps);

    if (codec->id == AV_CODEC_ID_H264)
        av_opt_set(c->priv_data, "preset", "slow", 0);


    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0)
    {
        cerr <<"Could not open codec"<<endl;
        exit(1);
    }

    f = fopen(filename, "wb");
    if (!f)
    {
        cerr<<"Could not open"<<filename<<endl;
        exit(1);
    }

    frame = av_frame_alloc();
    if (!frame)
    {
        cerr<<"Could not allocate video frame"<<endl;
        exit(1);
    }

    // initialize sample scaler
    //For conversion from AV_PIX_FMT_BGR24 to AV_PIX_FMT_YUV_420P
    swsctx = sws_getCachedContext(
            nullptr, dst_width, dst_height, AV_PIX_FMT_BGR24,
            dst_width, dst_height, c->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!swsctx)
    {
        std::cerr << "fail to sws_getCachedContext";
        return 2;
    }


    //Allocate frame buffer
    //std::vector<uint8_t> framebuf(avpicture_get_size(c->pix_fmt, dst_width, dst_height));
    int size = avpicture_get_size(c->pix_fmt, dst_width, dst_height);
    uint8_t  *out_buffer = (uint8_t *)av_malloc(size);
    ret  = avpicture_fill(reinterpret_cast<AVPicture*>(frame), out_buffer, c->pix_fmt, dst_width, dst_height);
    if(ret < 0)
    {
        cout<<"Failed at avpicture_fill"<<endl;
    }
    frame->width = dst_width;
    frame->height = dst_height;
    frame->format = static_cast<int>(c->pix_fmt);

}

void metadata_encode::capture_data()
{
    /**
     * Capture frames from the camera using OpenCV
     * Currently uses default backend.
     *
     */

    cv::VideoCapture cap(cv::CAP_V4L2);
    //cv::VideoCapture cap(0);
    if(!cap.isOpened())
    {
        std::cerr<<"Couldn't open videostream"<<endl;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH,dst_width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT,dst_height);

    //Allocate cv::Mat with extra bytes
    std::vector<uint8_t> imgbuf(dst_height * dst_width * 3 + 16);
    cv::Mat image(dst_height, dst_width, CV_8UC3, imgbuf.data(), dst_width * 3);


    bool end_of_stream = false;
    int count = 0;
    //Encode 3 seconds of video
    do {
        if(!end_of_stream)
        {
            cap>>image;
            cv::imshow("Frame",image);

            const int stride[] = {static_cast<int>(image.step[0])};

            sws_scale(swsctx,&image.data,stride,0,image.rows,frame->data,frame->linesize);

            frame->pts = count; //Timestamp information
            //encode image
            encode(c,frame,pkt,f);
            count++;
            // end_of_stream = true;
        }

    }
    while(count <= 90);

    //flush the encoder
    encode(c,NULL,pkt,f);

    fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);

    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);
}
//------------------------------------------------------------------------------------------------------------------------------//


static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,
                   FILE *outfile)
{
    /**
     * @brief
     * @param *frame: Pointer of type AVFrame(describes raw video data)
     * @param *pkt: Pointer of type AVPacket(filled with compressed data in the function)
     * @param *outfile: Output H.264 Video File
     *
     */
    int ret;
    if (frame)
    {
        // printf("Send frame %3"PRId64"\n", frame->pts);
        //cout<<"Send frame"<<endl;
    }

    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0)
    {
        cout<<"ret:"<<ret<<endl;
        cerr<<"Error sending a frame for encoding"<<endl;
        // fprintf(stderr, "Error sending a frame for encoding\n");
        exit(1);
    }

    while (ret >= 0)
    {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            cout<<"ret:"<<ret<<endl;
            fprintf(stderr, "Error during encoding\n");
            exit(1);
        }
        //cout<<"Write packet:"<<pkt->pts<<"size:"<<pkt->size<<endl;

        // printf("Write packet %3"PRId64" (size=%5d)\n", pkt->pts, pkt->size);

        /* for sei */
        //Get timestamp//
        int len;
        char sei[1024];
        char *timestamp = NULL;
        //timestamp = get_timestamp();
        timestamp = get_microseconds();
        //h264_sei_pack((uint8_t*)sei, &len,timestamp, true);
        h264_sei_pack((uint8_t*)sei, &len,timestamp, true);
        delete [] timestamp;
        //fwrite(sei, 1, len, outfile);
        fwrite(sei, 1, len, stdout);

        fwrite(pkt->data, 1, pkt->size, stdout);
        av_packet_unref(pkt);
    }
}

char* get_timestamp()
{
    //Function to get current date and time
    time_t now = time(0);
    char *dt = ctime(&now);
    return dt;
}

char* get_microseconds() {

    static int64_t prev = 0;
    struct timespec tms;

    if (clock_gettime(CLOCK_REALTIME,&tms)) {
        return NULL;
    }
    /* seconds, multiplied with 1 million */
    int64_t micros = tms.tv_sec * 1000000;
    /* Add full microseconds */
    micros += tms.tv_nsec/1000;
    /* round up if necessary */
    if (tms.tv_nsec % 1000 >= 500) {
        ++micros;
    }

    //cout << "diff " << micros - prev << endl;
    prev = micros;
    std::ostringstream ss;
    ss << micros;
    char *cstr = new char[ss.str().length() + 1];
    strcpy(cstr, ss.str().c_str());
    return cstr;
}

int main(int argc)
{
    int err_code = 0;
    metadata_encode metadata_encode_obj;
    err_code = metadata_encode_obj.initialize_codec();
    metadata_encode_obj.capture_data();
    return 0;

}