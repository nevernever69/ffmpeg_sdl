#include<stdio.h>
#include<libavcodec/avcodec.h>
#include<libavformat/avformat.h>
#include<inttypes.h>
static void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename)
{
	printf("\nentered\n");
    FILE *f;
    int i;
    f = fopen(filename,"w");
    // writing the minimal required header for a pgm file format
    // portable graymap format -> https://en.wikipedia.org/wiki/Netpbm_format#PGM_example
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);

    // writing line by line
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}
static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame)
{
  // Supply raw packet data as input to a decoder
  // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3
  int response = avcodec_send_packet(pCodecContext, pPacket);

  if (response < 0) {
    printf("Error while sending a packet to the decoder: %s", av_err2str(response));
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
	    printf("Error while receiving a frame from the decoder: %s", av_err2str(response));
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

      char frame_filename[1024];
      snprintf(frame_filename, sizeof(frame_filename), "%s-%d.pgm", "frame", pCodecContext->frame_number);
      // Check if the frame is a planar YUV 4:2:0, 12bpp
      // That is the format of the provided .mp4 file
      // RGB formats will definitely not give a gray image
      // Other YUV image may do so, but untested, so give a warning
      if (pFrame->format != AV_PIX_FMT_YUV420P)
      {
        printf("Warning: the generated file may not be a grayscale image, but could e.g. be just the R component if the video format is RGB");
      }
      // save a grayscale frame into a .pgm file
      save_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height, frame_filename);
    }
  }
  return 0;
}
int main(int argc, char *argv[]){
	AVFormatContext *pFormatCtx = NULL;   //it holds the header information 
	AVCodecContext *pCodecCtx = NULL;
	AVCodec *pcodec = NULL;
	AVFrame *pFrame = NULL;
	/* AVPacket packet; */


	if(argc < 2){        	       	  //check number of inputs
		printf("enter <video file>");
	}
	
	if(avformat_open_input(&pFormatCtx,argv[1],NULL,NULL)!=0){
		printf("cannot open input file");
	}
	if(avformat_find_stream_info(pFormatCtx,NULL)<0){
		printf("not able to find any stream");
	}
	int video_stream_index = -1;
	for(int i=0;i<pFormatCtx->nb_streams;i++){
		if(pFormatCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO){
			video_stream_index = i;
			break;
		}
	}
	if(video_stream_index==-1){
		printf("failed to find video stream");
		return -1;
	}
	pCodecCtx = avcodec_alloc_context3(NULL);
	if(pCodecCtx==NULL){
		printf("couldn't allocate codec context");
	}
	avcodec_parameters_to_context(pCodecCtx,pFormatCtx->streams[video_stream_index]->codecpar);
	pcodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if(pcodec==NULL){
		printf("not able to find decoder");
	}
	if(avcodec_open2(pCodecCtx,pcodec,NULL)<0){
		printf("couldn't open codec");
	}
	pFrame = av_frame_alloc();
	if(pFrame==NULL){
		printf("couldn't allocate a video frame");
	}

	AVPacket *packet = av_packet_alloc(); 
	int hw=50;
	int response =0;
	while(av_read_frame(pFormatCtx,packet)>=0){
		if(packet->stream_index== video_stream_index){
			/* response = decode_packet(packet,pCodecCtx,pFrame); */
			/* if(response<0){ */
			/* 	printf("entered"); */
			/* 	break; */
			/* } */
			/* if(--hw<=0){ */
			/* 	break; */
			/* } */
		/* av_packet_unref(packet); */


			response = avcodec_send_packet(pCodecCtx,packet);
			if(response < 0){
				break;
			}

			while(response>=0){
				response = avcodec_receive_frame(pCodecCtx,pFrame);
				if(response == AVERROR(EAGAIN) || response == AVERROR_EOF){

					break;
				}
				else if(response<0){
					printf("response is wrong");
					break;
				}
				/* if(response >= 0){ */
				/* 	printf("Frame %d (type=%c, size=%d bytes, format=%d) pts %d key_frame %d [DTS %d]", pCodecCtx->frame_number, av_get_picture_type_char(pFrame->pict_type), pFrame->pkt_size, pFrame->format, pFrame->pts, pFrame->key_frame, pFrame->coded_picture_number); */
			/* } */
				char frame_filename[1024];
				    snprintf(frame_filename, sizeof(frame_filename), "%s-%d.pgm", "frame", pCodecCtx->frame_number);
				if (pFrame->format != AV_PIX_FMT_YUV420P)
				{
					printf("Warning: the generated file may not be a grayscale image, but could e.g. be just the R component if the video format is RGB");
				}
				save_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height, frame_filename);
			}
		}
				if(--hw<=0){
					break;
				}
			
		
av_packet_unref(packet);
}


			avformat_close_input(&pFormatCtx);
			av_packet_free(&packet);
			av_frame_free(&pFrame);
			avcodec_free_context(&pCodecCtx);
	return 0;
}

