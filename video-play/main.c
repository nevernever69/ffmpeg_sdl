#include<libavcodec/avcodec.h>
#include<libavformat/avformat.h>
#include<stdlib.h>
#include<inttypes.h>
#include<libswscale/swscale.h>
#include<libavutil/imgutils.h>
#include<stdio.h>

void SaveFrame(uint8_t *dst_data[0], int width, int height, int iFrame) {
	FILE *pFile;
	char szFilename[32];
	int  y;

	// Open file
	sprintf(szFilename, "frame%d.ppm", iFrame);
	pFile=fopen(szFilename, "wb");
	if(pFile==NULL)
		return;

	// Write header
	fprintf(pFile, "P6\n%d %d\n255\n", width, height);

	// Write pixel data
	for(y=0; y<height; y++)
		fwrite( dst_data[0],1, width*3, pFile);

	// Close file
	fclose(pFile);
}
int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame){
	int response = avcodec_send_packet(pCodecContext,pPacket);
  if (response < 0) {
    printf("Error while sending a packet to the decoder");
    return response;
  }

  while (response >= 0)
  {
    // Return decoded output data (into a frame) from a decoder
    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c
    response = avcodec_receive_frame(pCodecContext, pFrame);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
      break;
    } else if (response < 0) {
      printf("Error while receiving a frame from the decoder");
      return response;
    }


    if (response >= 0) {
      printf(
          "Frame %d (type=%c, size=%d bytes, format=%d) pts %d key_frame %d [DTS %d]",
          pCodecContext->frame_number,
          av_get_picture_type_char(pFrame->pict_type),
          pFrame->pkt_size,
          pFrame->format,
          pFrame->pts,
          pFrame->key_frame,
          pFrame->coded_picture_number
      );

    }

  }
}

int main(int argc, char *argv[]){
	/* av_register_all(); */

	/* AVFormatContext *pformat = avformat_alloc_context(); */
	AVFormatContext *pFormatCtx = NULL;

	if (avformat_open_input(&pFormatCtx,argv[1],NULL,NULL)!=0) {   //opens media file for decoding
		return -1;
	}
	if(avformat_find_stream_info(pFormatCtx,NULL)<0){ 	//to read about streams in a media file like codec, bitrate
		return -1;
	}
	int record = -1;     //here is var for storing the number or representation of video strems
	AVCodecParameters* pCodecCtxparam = NULL;
	/* AVCodec *pCodec = NULL; */

	for(int i=0;i<pFormatCtx->nb_streams;i++){   	 	//loop to find the various streams in a container(video file)
		AVCodecParameters *pCodecParam = pFormatCtx->streams[i]->codecpar; 	 //pFormatCtx->streams is an array which stores nb_streams pointer
		/* AVCodec *pcodec = avcodec_find_decoder(pCodecParam->codec_id);//just for finding the decoder */ 
		/* if(pcodec==NULL){ */
		/* 	printf("unsupported codec before"); */
		/* } */
		if(pFormatCtx->streams[i]->codecpar->codec_type== AVMEDIA_TYPE_VIDEO){// it is same as AVCodecParam(pCodecParam)
			/* printf("video codec %d X %d\n", pCodecParam->width,pCodecParam->height);//just print the resolution of video */
			/* printf("\n codec number for video %d", i); */     
			record = i;
			/* pCodec = pcodec; */
			/* pCodecCtxparam = pCodecParam; */
			break;
		}
		if(record==-1){
			return -1;
		}
		/* if(pCodecParam->codec_type == AVMEDIA_TYPE_AUDIO){ */
		/* 	printf("sample rate %d\n",pCodecParam->sample_rate); */
		/* } */
		/* printf("codec is %s, bit_rate %d",avcodec->long_name,pCodecParam->bit_rate); */
	}
	printf("%d",record);
	pCodecCtxparam = pFormatCtx->streams[record]->codecpar;//store the pointer to codec context for the video stream
	AVCodec *pCodec = NULL;
	pCodec = avcodec_find_decoder(pCodecCtxparam->codec_id);
	if(pCodec == NULL){
		printf("unsupported codec");
	}

	AVCodecContext *pCodecCtx= avcodec_alloc_context3(pCodec);
	if(avcodec_parameters_to_context(pCodecCtx,pCodecCtxparam) != 0){
		printf("couldn't copy codec context");
	}
	if(avcodec_open2(pCodecCtx,pCodec,0)<0)
	{
		printf("couldn't open codec");
		return -1;
	}
	AVFrame *pFrame = av_frame_alloc();

	if(!pFrame){
		printf("failed to allocatea memory for AVFrame");
	}
	AVPacket *pPacket = NULL;
	uint8_t *buffer = NULL;
	int numbyte;

	int align = 1;


	struct SwsContext *sws_ctx = NULL;
	int framefinished;
	AVPacket *packet;

	int i=0;
	while(av_read_frame(pFormatCtx, packet)>=0){
		if(packet->stream_index==record){
			int framefinished = decode_packet(packet,pCodecCtx,pFrame);

			if(framefinished>=0){
				printf("enetred");
				sws_ctx = sws_getContext(pCodecCtx->width,pCodecCtx->height,pCodecCtx->pix_fmt,pCodecCtx->width,pCodecCtx->height,AV_PIX_FMT_RGB24,SWS_BICUBIC,NULL,NULL,NULL);
				numbyte =av_image_get_buffer_size(AV_PIX_FMT_RGB24,pCodecCtx->width,pCodecCtx->height,1);
				buffer = (uint8_t *)av_malloc(numbyte*sizeof(uint8_t));
				uint8_t* dst_data[4];
				int dst_linesize[4];

				int dst_bufsize = av_image_alloc(dst_data,dst_linesize,pCodecCtx->width,pCodecCtx->height,AV_PIX_FMT_RGB24,1);
				sws_scale(sws_ctx,(uint8_t const *const *)pFrame->data,pFrame->linesize,0,pCodecCtx->height,dst_data,dst_linesize);

				int frameCounter = 0;
				if(frameCounter<=10){
					char filename[128];
					sprintf(filename, "frame%d.ppm", frameCounter);
					FILE *fp = fopen(filename, "wb");
					if(fp == NULL) {
						fprintf(stderr, "Could not open file: %s\n", filename);
						exit(1);
					}
					fprintf(fp, "P6\n%d %d\n255\n", pCodecCtx->width, pCodecCtx->height);
					fwrite(dst_data[0], 1, dst_bufsize, fp);
					fclose(fp);               
					frameCounter++;
				}
			}
		}

		/* av_packet_free(&packet); */

	}




	av_free(buffer);

	// Free the YUV frame
	av_free(pFrame);

	// Close the codecs
	avcodec_close(pCodecCtx);

	// Close the video file
	avformat_close_input(&pFormatCtx);

	return 0;
	/* av_dump_format(pFormatCtx,0,argv[1],0);    		//to dump and print the details of media file */
	/* avformat_close_input(&pFormatCtx); */

}

