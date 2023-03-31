#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

int main(int argc, char *argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }

    // Initialize FFmpeg
    av_register_all();
    avcodec_register_all();

    // Open the video file
    AVFormatContext *formatCtx = NULL;
    if (avformat_open_input(&formatCtx, argv[1], NULL, NULL) != 0) {
        fprintf(stderr, "Could not open video file\n");
        return -1;
    }

    // Find the first video stream
    int videoStream = -1;
    AVCodecParameters *codecParams = NULL;
    AVCodec *codec = NULL;
    AVCodecContext *codecCtx = NULL;
    for (int i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            codecParams = formatCtx->streams[i]->codecpar;
            codec = avcodec_find_decoder(codecParams->codec_id);
            codecCtx = avcodec_alloc_context3(codec);
            avcodec_parameters_to_context(codecCtx, codecParams);
            avcodec_open2(codecCtx, codec, NULL);
            break;
        }
    }
    if (videoStream == -1) {
        fprintf(stderr, "Could not find video stream\n");
        return -1;
    }

    // Allocate an SDL window
    SDL_Window *window = SDL_CreateWindow("Video Player",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SCREEN_WIDTH,
                                          SCREEN_HEIGHT,
                                          SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "Could not create SDL window - %s\n", SDL_GetError());
        return -1;
    }

    // Allocate an SDL renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "Could not create SDL renderer - %s\n", SDL_GetError());
        return -1;
    }

    // Allocate an SDL texture
    SDL_Texture *texture = SDL_CreateTexture(renderer,
                                              SDL_PIXELFORMAT_RGB24,
                                              SDL_TEXTUREACCESS_STREAMING,
                                              codecCtx->width,
                                              codecCtx->height);
    if (!texture) {
        fprintf(stderr, "Could not create SDL texture - %s\n", SDL_GetError());
        return -1;
    }

    // Allocate a buffer for the decoded video frame
    AVFrame *frame = av_frame_alloc();
    uint8_t *buffer = NULL;
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, codecCtx->width, codecCtx->height, 1);
    buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(frame->data, frame->linesize, buffer, AV_PIX_FMT_RGB24, codecCtx->width, codecCtx->height, 1);

    // Allocate a buffer for the converted video frame
    AVFrame *convertedFrame = av_frame_alloc();
int convertedFrameBufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, codecCtx->width, codecCtx->height, 1);
uint8_t *convertedFrameBuffer = (uint8_t *)av_malloc(convertedFrameBufferSize);
av_image_fill_arrays(convertedFrame->data, convertedFrame->linesize, convertedFrameBuffer, AV_PIX_FMT_RGB24, codecCtx->width, codecCtx->height, 1);

// Allocate a sws context for frame conversion
struct SwsContext *swsCtx = sws_getContext(codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
                                           codecCtx->width, codecCtx->height, AV_PIX_FMT_RGB24,
                                           SWS_BILINEAR, NULL, NULL, NULL);

// Read frames from the video stream and display them in the SDL window
AVPacket packet;
int frameFinished;
while (av_read_frame(formatCtx, &packet) >= 0) {
    if (packet.stream_index == videoStream) {
        // Decode the video frame
        avcodec_send_packet(codecCtx, &packet);
        while (avcodec_receive_frame(codecCtx, frame) == 0) {
            // Convert the video frame to RGB24 format
            sws_scale(swsCtx, frame->data, frame->linesize, 0, codecCtx->height, convertedFrame->data, convertedFrame->linesize);

            // Update the SDL texture with the converted video frame
            SDL_UpdateTexture(texture, NULL, convertedFrame->data[0], convertedFrame->linesize[0]);

            // Clear the SDL renderer
            SDL_RenderClear(renderer);

            // Draw the SDL texture to the SDL renderer
            SDL_RenderCopy(renderer, texture, NULL, NULL);

            // Present the SDL renderer
            SDL_RenderPresent(renderer);

            // Delay to control the video playback speed
            SDL_Delay(1000 / 30); // 30 FPS
        }
    }

    // Free the packet
    av_packet_unref(&packet);
}

// Free resources
sws_freeContext(swsCtx);
av_free(buffer);
av_free(convertedFrameBuffer);
av_frame_free(&frame);
av_frame_free(&convertedFrame);
avcodec_close(codecCtx);
avformat_close_input(&formatCtx);
SDL_DestroyTexture(texture);
SDL_DestroyRenderer(renderer);
SDL_DestroyWindow(window);
SDL_Quit();


return 0;
}


