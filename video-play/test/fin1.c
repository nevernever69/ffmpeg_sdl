#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

int main(int argc, char *argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        exit(1);
    }

    // Open the input video file
    AVFormatContext *formatCtx = NULL;
    if (avformat_open_input(&formatCtx, argv[1], NULL, NULL) != 0) {
        fprintf(stderr, "Could not open video file %s\n", argv[1]);
        exit(1);
    }

    // Find the first video stream in the file
    int videoStream = -1;
    for (int i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }
    if (videoStream == -1) {
        fprintf(stderr, "Could not find video stream in %s\n", argv[1]);
        exit(1);
    }

    // Get a pointer to the codec context for the video stream
    AVCodecParameters *codecPar = formatCtx->streams[videoStream]->codecpar;

    // Find the decoder for the video stream
    AVCodec *codec = avcodec_find_decoder(codecPar->codec_id);
    if (codec == NULL) {
        fprintf(stderr, "Unsupported codec in %s\n", argv[1]);
        exit(1);
    }

    // Allocate a codec context for the decoder
    AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codecCtx, codecPar) != 0) {
        fprintf(stderr, "Could not initialize codec context for %s\n", argv[1]);
        exit(1);
    }

    // Open the codec
    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec for %s\n", argv[1]);
        exit(1);
    }

    // Allocate a video frame and a converted frame
    AVFrame *frame = av_frame_alloc();
    AVFrame *convertedFrame = av_frame_alloc();
    if (frame == NULL || convertedFrame == NULL) {
        fprintf(stderr, "Could not allocate video frames\n");
        exit(1);
    }

    // Allocate a buffer for the converted frame data
    int convertedFrameBufferSize = av_image_alloc(convertedFrame->data, convertedFrame->linesize, codecCtx->width, codecCtx->height, AV_PIX_FMT_RGB24, 1);
    if (convertedFrameBufferSize < 0) {
        fprintf(stderr, "Could not allocate buffer for converted frame\n");
        exit(1);
    }

    // Allocate a sws context for frame conversion
    struct SwsContext *swsCtx = sws_getContext(codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
codecCtx->width, codecCtx->height, AV_PIX_FMT_RGB24,
SWS_BILINEAR, NULL, NULL, NULL);
    // Allocate an SDL window and renderer
SDL_Window *window = SDL_CreateWindow("Video Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, codecCtx->width, codecCtx->height, SDL_WINDOW_SHOWN);
if (window == NULL) {
    fprintf(stderr, "Could not create SDL window: %s\n", SDL_GetError());
    exit(1);
}
SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
if (renderer == NULL) {
    fprintf(stderr, "Could not create SDL renderer: %s\n", SDL_GetError());
    exit(1);
}

// Initialize the packet and set up the frame timer
AVPacket packet;
int frameFinished = 0;
uint64_t frameTimer = SDL_GetPerformanceCounter();
uint64_t lastFrameTime = 0;
double delay = 0;

// Read frames from the file and display them in the SDL window
while (av_read_frame(formatCtx, &packet) >= 0) {
    // If the packet is from the video stream, decode the frame
    if (packet.stream_index == videoStream) {
        avcodec_send_packet(codecCtx, &packet);
        while (avcodec_receive_frame(codecCtx, frame) == 0) {
            // Convert the frame to RGB24
            sws_scale(swsCtx, frame->data, frame->linesize, 0, codecCtx->height, convertedFrame->data, convertedFrame->linesize);

            // Update the SDL texture with the converted frame data
            SDL_UpdateTexture(texture, NULL, convertedFrame->data[0], convertedFrame->linesize[0]);

            // Clear the renderer and copy the texture to the renderer
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);

            // Update the frame timer and delay
            frameTimer = SDL_GetPerformanceCounter();
            delay = (double)(frameTimer - lastFrameTime) / (double)SDL_GetPerformanceFrequency();
            lastFrameTime = frameTimer;
        }
    }

    // Free the packet
    av_packet_unref(&packet);

    // Delay until the next frame time
    if (delay > 0) {
        SDL_Delay((uint32_t)(delay * 1000.0));
        delay = 0;
    }
}

// Clean up resources
sws_freeContext(swsCtx);
av_free(convertedFrame->data[0]);
av_frame_free(&frame);
av_frame_free(&convertedFrame);
avcodec_close(codecCtx);
avformat_close_input(&formatCtx);
SDL_DestroyRenderer(renderer);
SDL_DestroyWindow(window);
SDL_Quit();

return 0;
}

