#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#define INBUF_SIZE 4096

int main(int argc, char *argv[]) {
    AVFormatContext *pFormatCtx = NULL;
    int             i, videoStream;
    AVCodecContext  *pCodecCtxOrig = NULL, *pCodecCtx = NULL;
    AVCodec         *pCodec = NULL;
    AVFrame         *pFrame = NULL;
    AVPacket        packet;
    int             frameFinished;
    struct SwsContext *sws_ctx = NULL;

    AVDictionary    *optionsDict = NULL;
    int             numBytes;
    uint8_t         *buffer = NULL;

    if(argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        exit(1);
    }

    // Open video file
    if(avformat_open_input(&pFormatCtx, argv[1], NULL, NULL)!=0) {
        fprintf(stderr, "Could not open video file\n");
        exit(1);
    }

    // Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx, NULL)<0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }

    // Dump information about file onto standard error
    av_dump_format(pFormatCtx, 0, argv[1], 0);

    // Find the first video stream
    videoStream=-1;
    for(i=0; i<pFormatCtx->nb_streams; i++) {
        if(pFormatCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO) {
            videoStream=i;
            break;
        }
    }
    if(videoStream==-1) {
        fprintf(stderr, "Could not find a video stream\n");
        exit(1);
    }

    // Get a pointer to the codec context for the video stream
    pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;
    pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
    if(pCodec==NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        exit(1);
    }

    // Copy context
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if(avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar) < 0) {
        fprintf(stderr, "Failed to copy context\n");
        exit(1);
    }

    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    // Allocate video frame
    pFrame = av_frame_alloc();

    // Initialize packet
    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;

    // Read frames and save as color images
    int frameCounter = 0;
    while(av_read_frame(pFormatCtx, &packet)>=0) {
        // Is this a packet from the video stream?
           if(packet.stream_index==videoStream) {
        // Decode video frame
        avcodec_send_packet(pCodecCtx, &packet);
        while(avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
            // Convert the image from its native format to RGB
            sws_ctx = sws_getContext(pCodecCtx->width,
                                     pCodecCtx->height,
                                     pCodecCtx->pix_fmt,
                                     pCodecCtx->width,
                                     pCodecCtx->height,
                                     AV_PIX_FMT_RGB24,
                                     SWS_BILINEAR,
                                     NULL,
                                     NULL,
                                     NULL);

            if(sws_ctx == NULL) {
                fprintf(stderr, "Could not initialize the conversion context\n");
                exit(1);
            }

            // Allocate RGB image buffer
            numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);
            buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

            // Convert image to RGB
            av_image_fill_arrays(pFrame->data, pFrame->linesize, buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);
            sws_scale(sws_ctx, (uint8_t const * const *) pFrame->data,
                      pFrame->linesize, 0, pCodecCtx->height, pFrame->data, pFrame->linesize);

            // Save RGB image as ppm
            char filename[32];
            sprintf(filename, "frame%04d.ppm", frameCounter);
            FILE *f = fopen(filename, "wb");
            if(f == NULL) {
                fprintf(stderr, "Could not open file %s\n", filename);
                exit(1);
            }
            fprintf(f, "P6\n%d %d\n255\n", pCodecCtx->width, pCodecCtx->height);
            fwrite(buffer, 1, numBytes, f);
            fclose(f);

            frameCounter++;
            av_freep(&buffer);
            sws_freeContext(sws_ctx);
        }
    }

    // Free the packet that was allocated by av_read_frame
    av_packet_unref(&packet);
}

// Free the RGB image buffer
av_free(buffer);

// Free the frame
av_frame_free(&pFrame);

// Close the codec
avcodec_close(pCodecCtx);

// Close the video file
avformat_close_input(&pFormatCtx);

return 0;
}
