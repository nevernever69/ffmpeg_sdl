//working to save color frame from whole video 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include<SDL2/SDL.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include<libavutil/imgutils.h>
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)

#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)

int thread_exit = 0;
int thread_pause = 0;

/* int sfp_refresh_thread(void *opaque) { */
/*     thread_exit = 0; */
/*     thread_pause = 0; */

/*     while (!thread_exit) { */
/*         if (!thread_pause) { */
/*             SDL_Event event; */
/*             event.type = SFM_REFRESH_EVENT; */
/*             SDL_PushEvent(&event); */
/*         } */
/*         SDL_Delay(40); */
/*     } */
/*     thread_exit = 0; */
/*     thread_pause = 0; */
/*     //Break */
/*     SDL_Event event; */
/*     event.type = SFM_BREAK_EVENT; */
/*     SDL_PushEvent(&event); */

/*     return 0; */
/* } */
int displayit(AVFrame *frame){
	if(SDL_INIT_VIDEO!=NULL){
		printf("Error initialize SDL %s\n",SDL_GetError());
		return;
	}
	SDL_Windows *window = SDL_CreateWindow("video stream",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,frame->width,frame->height,SDL_WINDOW_SHOWN);
	if(window == NULL){
		printf("Error creating windows %s\n",SDL_GetError());
	}
	SDL_Renderer *renderer = SDL_CreateRenderer(window,-1,0);
	if(renderer==NULL){
		printf("Error creating windows %s\n",SDL_GetError());

	}
	SDL_Texture *texture = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_IYUV,SDL_TEXTUREACCESS_STREAMING,frame->width,frame->height);
	if(texture==NULL){
		printf("Error creating windows %s\n",SDL_GetError());

	}
	    // Lock the texture to allow access to its pixel data
    void *pixels;
    int pitch;
    if (SDL_LockTexture(texture, NULL, &pixels, &pitch) != 0) {
        printf("Error locking texture: %s\n", SDL_GetError());
        return;
    }

    // Copy the frame's pixel data to the texture
    memcpy(pixels, frame->pixels, frame->pitch * frame->height);

    // Unlock the texture to allow it to be displayed
    SDL_UnlockTexture(texture);

    // Clear the renderer's screen
    SDL_RenderClear(renderer);

    // Copy the texture to the renderer's buffer
    SDL_RenderCopy(renderer, texture, NULL, NULL);

    // Display the rendered image on the screen
    SDL_RenderPresent(renderer);

    // Delay to limit the frame rate
    SDL_Delay(10);

    // Clean up resources
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

		


}
int main(int argc, char *argv[]) {
    AVFormatContext *pFormatCtx = NULL;
    AVCodecContext *pCodecCtx = NULL;
    AVCodec *pCodec = NULL;
    AVFrame *pFrame = NULL;
    AVPacket *packet;

    int videoStream = -1;
    int frameCounter = 0;
    struct SwsContext *sws_ctx = NULL;
    uint8_t *buffer = NULL;
    int numBytes;
    int screen_w, screen_h;
    SDL_Window *screen;
    SDL_Renderer *sdlRenderer;
    SDL_Texture *sdlTexture;
    SDL_Rect sdlRect;
    SDL_Thread *video_tid;
    SDL_Event event; 
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
   AVFrame* pFrameYUV = av_frame_alloc();

    // Read frames from the file
    /* int framec = 40; */
                // Convert the image from its native format to RGB
                sws_ctx = sws_getContext(pCodecCtx->width,
                                         pCodecCtx->height,
                                         pCodecCtx->pix_fmt,
                                         pCodecCtx->width,
                                         pCodecCtx->height,
					 AV_PIX_FMT_YUV420P,
                                         SWS_BICUBIC,
                                         NULL,
                                         NULL,
                                         NULL);

                // Allocate memory for the RGB image buffer
                numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
                buffer = (uint8_t*) av_malloc(numBytes * sizeof(uint8_t));
		uint8_t* dst_data[4];
		int dst_linesize[4];
		int dst_bufsize = av_image_alloc(dst_data, dst_linesize, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, 1);

		// Convert the frame to RGB and save it as a color image
		  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }
screen_w = pCodecCtx->width;
    screen_h = pCodecCtx->height;
    screen = SDL_CreateWindow("Simplest ffmpeg player's Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              screen_w, screen_h, SDL_WINDOW_OPENGL);

      if (!screen) {
        printf("SDL: could not create window - exiting:%s\n", SDL_GetError());
        return -1;
    }
      sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
    //IYUV: Y + U + V  (3 planes)
    //YV12: Y + V + U  (3 planes)
    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width,
                                   pCodecCtx->height);
        sdlRect.x = 0;
    sdlRect.y = 0;
    sdlRect.w = screen_w;
    sdlRect.h = screen_h;

    packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    /* video_tid = SDL_CreateThread(sfp_refresh_thread, NULL, NULL); */

    while(av_read_frame(pFormatCtx, packet) == 0) {
	    if(packet->stream_index == videoStream) {
		    // Decode video frame
		    avcodec_send_packet(pCodecCtx, packet);
		    while(avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
		sws_scale(sws_ctx,
				(const uint8_t* const*) pFrame->data,
				pFrame->linesize,
				0,
				pCodecCtx->height,
				dst_data,
				dst_linesize);
		    SDL_UpdateTexture(sdlTexture, NULL, dst_data[0],*dst_linesize);
		     SDL_RenderClear(sdlRenderer);
                //SDL_RenderCopy( sdlRenderer, sdlTexture, &sdlRect, &sdlRect );
                SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
                SDL_RenderPresent(sdlRenderer);
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
            }

	    /* if(--framec <= 0 ){ */
		/* break; */
	    /* } */
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

