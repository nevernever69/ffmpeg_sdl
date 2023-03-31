#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

#define WINDOW_TITLE "Video Player"
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <video file>\n", argv[0]);
        return -1;
    }


    AVFormatContext* format_ctx = NULL;
    if (avformat_open_input(&format_ctx, argv[1], NULL, NULL) != 0) {
        printf("Failed to open file: %s\n", argv[1]);
        return -1;
    }

    if (avformat_find_stream_info(format_ctx, NULL) < 0) {
        printf("Failed to find stream info\n");
        return -1;
    }

    int video_stream_index = -1;
    for (int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }

    if (video_stream_index == -1) {
        printf("Failed to find video stream\n");
        return -1;
    }

    AVCodecParameters* codec_params = format_ctx->streams[video_stream_index]->codecpar;
    AVCodec* codec = avcodec_find_decoder(codec_params->codec_id);
    if (codec == NULL) {
        printf("Failed to find codec\n");
        return -1;
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (codec_ctx == NULL) {
        printf("Failed to allocate codec context\n");
        return -1;
    }

    if (avcodec_parameters_to_context(codec_ctx, codec_params) < 0) {
        printf("Failed to copy codec parameters to codec context\n");
        return -1;
    }

    if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
        printf("Failed to open codec\n");
        return -1;
    }

    AVFrame* frame = av_frame_alloc();
    AVPacket packet;
    int frame_count = 0;

    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        0
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_YV12,
        SDL_TEXTUREACCESS_STREAMING,
        codec_ctx->width,
        codec_ctx->height
    );

    struct SwsContext* sws_ctx = sws_getContext(
        codec_ctx->width,
        codec_ctx->height,
        codec_ctx->pix_fmt,
        codec_ctx->width,
        codec_ctx->height,
    AV_PIX_FMT_YUV420P,
    SWS_BILINEAR,
    NULL,
    NULL,
    NULL
);

while (av_read_frame(format_ctx, &packet) >= 0) {
    if (packet.stream_index == video_stream_index) {
        int ret = avcodec_send_packet(codec_ctx, &packet);
        if (ret < 0) {
            printf("Error sending packet to codec\n");
            break;
        }

        while (ret >= 0) {
            ret = avcodec_receive_frame(codec_ctx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            }
            else if (ret < 0) {
                printf("Error receiving frame from codec\n");
                break;
            }

            SDL_UpdateYUVTexture(
                texture,
                NULL,
                frame->data[0],
                frame->linesize[0],
                frame->data[1],
                frame->linesize[1],
                frame->data[2],
                frame->linesize[2]
            );

            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);

            // Save the frame as a BMP image
            char filename[100];
            sprintf(filename, "frame%d.bmp", frame_count++);
            SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
                frame->data[0],
                codec_ctx->width,
                codec_ctx->height,
                24,
                frame->linesize[0],
                SDL_PIXELFORMAT_BGR24
            );
            SDL_SaveBMP(surface, filename);
            SDL_FreeSurface(surface);

            av_frame_unref(frame);
        }
    }

    av_packet_unref(&packet);
}

sws_freeContext(sws_ctx);

av_frame_free(&frame);
avcodec_free_context(&codec_ctx);
avformat_close_input(&format_ctx);

SDL_DestroyTexture(texture);
SDL_DestroyRenderer(renderer);
SDL_DestroyWindow(window);
SDL_Quit();

return 0;
}


