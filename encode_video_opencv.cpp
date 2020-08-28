//Encode SEI data into NAL units with H.264 encoding
//Raw frames are obtained using opencv and are encoded with metdata using H.264 standard
//Flow: Initialize codec info--> Capture raw data using OpenCV VideoCapture --> Convert from AV_PIX_FMT_BGR24 to AV_PIX_FMT_YUV_420P
//-->Encode SEI info to videostream

#include <iostream>
#include <vector>


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
    char ch;
    cout<<"Waiting for input..."<<endl;
    cin>>ch;
}

// void display_image(string winname,cv::Mat img)
// {
//     cv::namedWindow(winname,CV_WINDOW_NORMAL);
//     cv::imshow(winname,img);
//     cv::waitKey(1);
// }
// cv::Mat read_image(int dst_width,int dst_height)
// {
//     cv::VideoCapture cap(0);
//     if(!cap.isOpened())
//     {
//         std::cerr<<"Couldn't open videostream"<<endl;
//     }

//     cap.set(cv::CAP_PROP_FRAME_WIDTH,dst_width);
//     cap.set(cv::CAP_PROP_FRAME_HEIGHT,dst_height);

//     //Allocate cv::Mat with extra bytes
//     std::vector<uint8_t> imgbuf(dst_height * dst_width * 3 + 16); //16 is arbitrary?
//     cv::Mat frame(dst_height, dst_width, CV_8UC3, imgbuf.data(), dst_width * 3);

//     //Read from webcam
//     // while(true)
//     // {
//     cap >> frame;
//     if(!frame.empty())
//     {
//         // break;
//         // display_image("frame",frame);
//         char c = (char)cv::waitKey(25);
//         if(c == 27)
//         {
//             cv::destroyAllWindows();
//             // break;
//         }
//     }
//     // }
//     return frame;
// }



static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,
                   FILE *outfile)
{
    int ret;

    /* send the frame to the encoder */
    if (frame)
    {
        // printf("Send frame %3"PRId64"\n", frame->pts);
        cout<<"Send frame"<<endl;
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
        cout<<"Write packet:"<<pkt->pts<<"size:"<<pkt->size<<endl;

        // printf("Write packet %3"PRId64" (size=%5d)\n", pkt->pts, pkt->size);

        /* for sei */
        int len;
        char sei[1024];
        h264_sei_pack((uint8_t*)sei, &len, "bepartofyou", true);
        fwrite(sei, 1, len, outfile);
    
        fwrite(pkt->data, 1, pkt->size, outfile);
        av_packet_unref(pkt);
    }
}



int main(int argc)
{

    const char *filename, *codec_name;
    const AVCodec *codec;
    AVCodecContext *c= NULL;
    int i, ret, x, y;
    FILE *f;
    AVFrame *frame;
    AVPacket *pkt;
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };

    const int dst_width = 640;
    const int dst_height = 480;
    const AVRational dst_fps = {30,1};

    //Encode frames using ffmpeg
    codec_name = "libx264";
    filename = "test.h264";
    codec = avcodec_find_encoder_by_name(codec_name);
    if(!codec)
    {
        std::cerr << "Codec"<<codec_name<<"not found"<<endl;
        exit(1);
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        std::cerr<<"Could not allocate video codec context"<<endl;
        exit(1);
    }

    pkt = av_packet_alloc();
    if(!pkt)
    {
        exit(1);
    }
    //Streaming parameters
    c->bit_rate = 400000;
    c->width = dst_width;
    c->height = dst_height;
    c->time_base = av_inv_q(dst_fps);
    c->framerate = dst_fps;
    c->gop_size = 10;
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;

     if (codec->id == AV_CODEC_ID_H264)
        av_opt_set(c->priv_data, "preset", "slow", 0);

     /* open it */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        // cerr<<"Could not open codec"<<av_err2str(ret)<<endl;
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
    SwsContext* swsctx = sws_getCachedContext(
        nullptr, dst_width, dst_height, AV_PIX_FMT_BGR24,
        dst_width, dst_height, c->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!swsctx) {
        std::cerr << "fail to sws_getCachedContext";
        return 2;
    }

    
    //Allocate frame buffer
    std::vector<uint8_t> framebuf(avpicture_get_size(c->pix_fmt, dst_width, dst_height));
    ret  = avpicture_fill(reinterpret_cast<AVPicture*>(frame), framebuf.data(), c->pix_fmt, dst_width, dst_height);
    if(ret < 0)
    {
        cout<<"Failed at avpicture_fill"<<endl;
    }
    frame->width = dst_width;
    frame->height = dst_height;
    frame->format = static_cast<int>(c->pix_fmt);
    // cout<<"frame->width:"<<frame->width<<endl;
    // cout<<"frame-height:"<<frame->height<<endl;
    // cout<<"frame->format:"<<frame->format<<endl;
    
    //Capture frames using OpenCV
    cv::VideoCapture cap(0);
    if(!cap.isOpened())
    {
        std::cerr<<"Couldn't open videostream"<<endl;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH,dst_width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT,dst_height);

    //Allocate cv::Mat with extra bytes
    std::vector<uint8_t> imgbuf(dst_height * dst_width * 3 + 16); //16 is arbitrary?
    cv::Mat image(dst_height, dst_width, CV_8UC3, imgbuf.data(), dst_width * 3);


    bool end_of_stream = false;
    int count = 0;
    //Encode 3 seconds of video
    do {
        if(!end_of_stream)
        {
            cap>>image;
            cv::imshow("Frame",image); //Read frames from webcam
            //Make frame writeable
            // ret = av_frame_make_writable(frame);
            // if (ret < 0)
            // {
            //     cerr<<"Could not make frame writeable"<<endl;
            //     exit(1);
            // }
            const int stride[] = {static_cast<int>(image.step[0])};
            sws_scale(swsctx,&image.data,stride,0,image.rows,frame->data,frame->linesize);
            frame->pts = count;
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
    return 0;

}