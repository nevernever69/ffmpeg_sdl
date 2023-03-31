//starting of a ffmpeg learning
//for that i have to learn about how ffmpeg analyse file
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
static void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename)
{
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
int main(int argc, const char *argv[]){  //take two argument first file name 
	if(argc < 2){
		printf("enter a arrgument");
		return -1;
	}

	AVFormatContext *pFormatContext = avformat_alloc_context();   //allocate a memory for a AvFormatContext to load a container of file and extract the header 
	if(!pFormatContext){
		printf("cannot allcoate a memory for a file");
		return -1;
	}
	if(avformat_open_input(&pFormatContext, argv[1], NULL, NULL) !=0 ){   //now it's opens a headers
		printf("couldnot open a file");
		return -1;

	}
	printf("format %s, duration %lld us, bit_rate %lld", pFormatContext->iformat->name, pFormatContext->duration,pFormatContext->bit_rate);
	AVCodecParameters* av_codec_param;
	AVCodec *avcodec;

	avformat_find_stream_info(pFormatContext,NULL);

	printf("\nstreams %lld",pFormatContext->nb_streams); //print number of streams: generally there are two 
	for(int i=0;i<pFormatContext->nb_streams;i++){

		av_codec_param = pFormatContext->streams[i]->codecpar;
		avcodec = avcodec_find_decoder(av_codec_param->codec_id);
		if(av_codec_param->codec_type == AVMEDIA_TYPE_VIDEO){  //getting video info like resolution of it
			printf("\n%dX%d\n", av_codec_param->width, av_codec_param->height);
		}	
		if(av_codec_param->codec_type == AVMEDIA_TYPE_AUDIO){
			printf("\n sample rate= %d\n" /*av_codec_param->channels*/, av_codec_param->sample_rate);   //getting audio info like samplerate

		}
		printf("\ncodec %s ID %d bit_rate %lld\t",avcodec->long_name,avcodec->id, av_codec_param->bit_rate);   //getting audio and video both info like codec and bitrate

	}
	AVCodecContext *pCodecContext = avcodec_alloc_context3(avcodec);
	if(!pCodecContext){
		printf("couldn't allocate a memory");
	}
	avcodec_parameters_to_context(pCodecContext, av_codec_param);
	avcodec_open2(pCodecContext, avcodec, NULL);

	AVFrame* av_frame = av_frame_alloc();
	AVPacket* av_packet = av_packet_alloc(); 

	while(av_read_frame(pFormatContext, av_packet) >= 0){
		avcodec_send_packet(pCodecContext, av_packet);
		avcodec_receive_packet(pCodecContext, av_packet);
	}
	printf(
			"Frame %c (%d) pts %d dts %d key_frame %d [coded_picture_number %d, display_picture_number %d]",
			av_get_picture_type_char(av_frame->pict_type),
			pCodecContext->frame_number,
			av_frame->pts,
			av_frame->pkt_dts,
			av_frame->key_frame,
			av_frame->coded_picture_number,
			av_frame->display_picture_number
	      );
	      char frame_filename[1024];
  save_gray_frame(av_frame->data[0], av_frame->linesize[0], av_frame->width, av_frame->height, frame_filename);




}

