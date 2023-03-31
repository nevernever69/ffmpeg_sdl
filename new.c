//starting of a ffmpeg learning
//for that i have to learn about how ffmpeg analyse file
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

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
	printf("streams %lld",pFormatContext->nb_streams);

}

