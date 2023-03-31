//working to save color frame from whole video 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include<libavutil/imgutils.h>
int iFrame = 0;
void saveframe(uint8_t* dst_data[], int width,int height,int dst_bufsize){
	printf("\n%d\n",iFrame);
	char filename[128];
	sprintf(filename, "frame%d.ppm", iFrame);
	FILE *fp = fopen(filename, "wb");
	if(fp == NULL) {
		fprintf(stderr, "Could not open file: %s\n", filename);
		exit(1);
	}
	fprintf(fp, "P6\n%d %d\n255\n", width, height);
	fwrite( dst_data[0],1, dst_bufsize, fp);
	fclose(fp);               
}
void decode_it(struct SwsContext *sws_ctx, AVCodecContext* pCodecCtx,AVFrame *pFrame,AVPacket *packet,uint8_t *buffer){
		int numBytes;
           avcodec_send_packet(pCodecCtx, packet);
	 
		    while(avcodec_receive_frame(pCodecCtx, pFrame)==0){
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
                // Allocate memory for the RGB image buffer
                numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);
                buffer = (uint8_t*) av_malloc(numBytes * sizeof(uint8_t));
		uint8_t* dst_data[4];
		int dst_linesize[4];
		int dst_bufsize = av_image_alloc(dst_data, dst_linesize, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, 1);

		AVFrame *pFrameRGB = av_frame_alloc();
		// Convert the frame to RGB and save it as a color image
		sws_scale(sws_ctx,
				(const uint8_t* const*) pFrame->data,
				pFrame->linesize,
				0,
				pCodecCtx->height,
				dst_data,
				dst_linesize);
		
	saveframe(dst_data,pCodecCtx->width,pCodecCtx->height,dst_bufsize);
	iFrame++;

            }



}
int main(int argc, char *argv[]) {
    AVFormatContext *pFormatCtx = NULL;
    AVCodecContext *pCodecCtx = NULL;
    AVCodec *pCodec = NULL;
    AVFrame *pFrame = NULL;
    AVPacket *packet;

    int videoStream = -1;
    struct SwsContext *sws_ctx = NULL;
    uint8_t *buffer = NULL;
    int numBytes;

    if(argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        exit(1);
    }

    // Open the input file
    if(avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0) {
        fprintf(stderr, "Could not open input file: %s\n", argv[1]);
        exit(1);
    }

    // Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }

    // Find the first video stream
    for(int i = 0; i < pFormatCtx->nb_streams; i++) {
        if(pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }

    if(videoStream == -1) {
        fprintf(stderr, "Could not find a video stream\n");
        exit(1);
    }

    // Get a pointer to the codec context for the video stream
    pCodecCtx = avcodec_alloc_context3(NULL);
    if(pCodecCtx == NULL) {
        fprintf(stderr, "Could not allocate codec context\n");
        exit(1);
    }

    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar);

    // Find the decoder for the video stream
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec == NULL) {
        fprintf(stderr, "Unsupported codec\n");
        exit(1);
    }

    // Open the codec
    if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }

    // Allocate video frame
    pFrame = av_frame_alloc();
    if(pFrame == NULL) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    // Read frames from the file
    int hw = 500;
    while(av_read_frame(pFormatCtx, packet) == 0) {
        if(packet->stream_index == videoStream) {
            // Decode video frame
            /* avcodec_send_packet(pCodecCtx, &packet); */
            /* while(avcodec_receive_frame(pCodecCtx, pFrame) == 0) { */
            /*     // Convert the image from its native format to RGB */
            /*     sws_ctx = sws_getContext(pCodecCtx->width, */
            /*                              pCodecCtx->height, */
            /*                              pCodecCtx->pix_fmt, */
            /*                              pCodecCtx->width, */
            /*                              pCodecCtx->height, */
            /*                              AV_PIX_FMT_RGB24, */
            /*                              SWS_BILINEAR, */
            /*                              NULL, */
            /*                              NULL, */
            /*                              NULL); */

            /*     // Allocate memory for the RGB image buffer */
            /*     numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1); */
            /*     buffer = (uint8_t*) av_malloc(numBytes * sizeof(uint8_t)); */
		/* uint8_t* dst_data[4]; */
		/* int dst_linesize[4]; */
		/* int dst_bufsize = av_image_alloc(dst_data, dst_linesize, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, 1); */

		/* AVFrame *pFrameRGB = av_frame_alloc(); */
		/* // Convert the frame to RGB and save it as a color image */
		/* sws_scale(sws_ctx, */
				/* (const uint8_t* const*) pFrame->data, */
				/* pFrame->linesize, */
				/* 0, */
				/* pCodecCtx->height, */
				/* dst_data, */
				/* dst_linesize); */
                // Convert the frame to RGB and save it as a color image
                
                // Save the color image
                /* char filename[128]; */
                /* sprintf(filename, "frame%d.ppm", frameCounter); */
                /* FILE *fp = fopen(filename, "wb"); */
                /* if(fp == NULL) { */
                /*     fprintf(stderr, "Could not open file: %s\n", filename); */
                /*     exit(1); */
                /* } */
                /* fprintf(fp, "P6\n%d %d\n255\n", pCodecCtx->width, pCodecCtx->height); */
                /* fwrite(dst_data[0], 1, dst_bufsize, fp); */
		/* fclose(fp); */               
		/* frameCounter++; */

            /* } */

	     decode_it(sws_ctx,pCodecCtx,pFrame,packet,buffer);
	    /* if(response < 0){ */
		/* break; */
		
	    /* } */
	    if(--hw<=0){
		    break;
	    }
        }
        av_packet_unref(packet);
    }

    // Free resources
    av_frame_free(&pFrame);
    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pFormatCtx);
    avformat_free_context(pFormatCtx);
    av_free(buffer);
    sws_freeContext(sws_ctx);

    return 0;
}

