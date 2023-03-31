// tutorial01.c
// Code based on a tutorial by Martin Bohme (boehme@inb.uni-luebeckREMOVETHIS.de)
// Tested on Gentoo, CVS version 5/01/07 compiled with GCC 4.1.1
// With updates from https://github.com/chelyaev/ffmpeg-tutorial
// Updates tested on:
// LAVC 54.59.100, LAVF 54.29.104, LSWS 2.1.101 
// on GCC 4.7.2 in Debian February 2015
//
// Updates tested on:
// Mac OS X 10.11.6
// Apple LLVM version 8.0.0 (clang-800.0.38)
//
// A small sample program that shows how to use libavformat and libavcodec to read video from a file.
//
// Use
//
// $ gcc -o tutorial01 tutorial01.c -lavutil -lavformat -lavcodec -lswscale -lz -lm
//
// to build (assuming libavutil/libavformat/libavcodec/libswscale are correctly installed your system).
//
// Run using
//
// $ tutorial01 myvideofile.mpg
//
// to write the first five frames from "myvideofile.mpg" to disk in PPM format.

// comment by breakpointlab@outlook.com

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

#include <stdio.h>

// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

//Save PPM file
void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
	FILE *pFile;//Define file objects
	char szFilename[32];//Define output file name
	
	// Open file
	sprintf(szFilename, "frame%d.ppm", iFrame);//Format output file name
	pFile = fopen(szFilename, "wb");//Open output file
	if (pFile == NULL) {//Check whether the output file is opened successfully
		return;
	}
  
	// Write header indicated how wide & all the image is
	fprintf(pFile, "P6\n%d %d\n255\n", width, height);
  
	// Write pixel data, write the file one line a time
	int y;
	for (y = 0; y < height; y++) {
		fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
	}
  
	// Close file
	fclose(pFile);
}

int main(int argc, char *argv[]) {
/*--------------Parameter definition-------------*/
	// Initalizing these to NULL prevents segfaults!
	AVFormatContext *pFormatCtx = NULL;//A structure for storing the encapsulation information and stream parameters of file container
	AVCodecContext *pCodecCtxOrig = NULL;//Decoder context object, decoder dependent environment, state, resource and interface pointer of parameter set
	AVCodecContext *pCodecCtx = NULL;//Encoder context object for PPM file output
	AVCodec *pCodec = NULL;//It can be regarded as a global variable of encoder and decoder
	AVPacket packet;//A structure that is responsible for storing information related to compressed coding data. Each frame of image is composed of one or more packet s
	AVFrame *pFrame = NULL;//Save audio and video decoded data, such as state information, codec information, macroblock type table, QP table, motion vector table and so on
	AVFrame *pFrameRGB = NULL;//Save the PPM file data of 24 bit RGB output
	struct SwsContext *sws_ctx = NULL;//Structure describing the converter parameters

	int numBytes;//Data length in RGB24 format
	uint8_t *buffer = NULL;//Decode data output cache pointer
	int i,videoStream;//Loop variable, video stream type label
	int frameFinished;//Identification of success of decoding operation

/*-------------Parameter initialization------------*/
	if (argc<2) {//Check whether the number of input parameters is correct
		printf("Please provide a movie file\n");
		return -1;
	}

	// Register all available formats and codecs to register all ffmpeg supported multimedia formats and codecs
	av_register_all();

	/*-----------------------
	 * Open video file，Open the video file, read the file header content, get the package information and code stream parameters of the file container, and store them in pFormatCtx
	 * read the file header and stores information about the file format in the AVFormatContext structure  
	 * The last three arguments are used to specify the file format, buffer size, and format options
	 * but by setting this to NULL or 0, libavformat will auto-detect these
	 -----------------------*/
	if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0) {
		return -1; // Couldn't open file.
	}

	/*-----------------------
	 * Get the code stream information saved in the file and fill in the pformatctx > stream field
	 * check out & Retrieve the stream information in the file
	 * then populate pFormatCtx->stream with the proper information 
	 * pFormatCtx->streams is just an array of pointers, of size pFormatCtx->nb_streams
	 -----------------------*/
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		return -1; // Couldn't find stream information.
	}

	// Dump information about file onto standard error to print the code stream information in pFormatCtx
	av_dump_format(pFormatCtx, 0, argv[1], 0);

	// Find the first video stream.
	videoStream=-1;//The video stream type label is initialized to - 1
	for (i=0;i<pFormatCtx->nb_streams;i++) {//Traverse all streaming media types (video stream, audio stream, subtitle stream, etc.) contained in the file
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {//If the file contains a video stream
			videoStream = i;//Modify the ID with the label of the video stream type so that it is not - 1
			break;//Exit loop
		}
	}
	if (videoStream==-1) {//Check if there is a video stream in the file
		return -1; // Didn't find a video stream.
	}

	// Get a pointer to the codec context for the video stream, and get the decoder context corresponding to the video stream from pformatctx > streams according to the stream type label
	pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;
	/*-----------------------
	 * Find the decoder for the video stream，Find the corresponding decoder according to the decoder context corresponding to the video stream, and return the corresponding decoder (information structure)
	 * The stream's information about the codec is in what we call the "codec context.
	 * This contains all the information about the codec that the stream is using
	 -----------------------*/
	pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
	if (pCodec == NULL) {//Check if the decoder matches
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found.
	}

	// Copy is used to extract the video codec from the context
	pCodecCtx = avcodec_alloc_context3(pCodec);//Create AVCodecContext structure object pCodecCtx
	if (avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {//Copy codec context object
		fprintf(stderr, "Couldn't copy codec context");
		return -1; // Error copying codec context.
	}

	// Open codec, open decoder
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
		return -1; // Could not open codec.
	}

	// Allocate video frame to allocate space for the decoded video information structure and complete the initialization operation (the image cache in the structure is manually installed according to the following two steps)
	pFrame = av_frame_alloc();

	// Allocate an AVFrame structure to allocate space for the structure of the converted PPM file and complete the initialization operation
	pFrameRGB = av_frame_alloc();
	if (pFrameRGB == NULL) {//Check whether the initialization operation is successful
		return -1;
	}

	// Determine required buffer size and allocate buffer to calculate the memory size according to the pixel format and image size
	numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);
	buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));//Configure cache space for converted RGB24 images

	// Assign appropriate parts of buffer to image planes in pFrameRGB Note that pFrameRGB is an AVFrame, but AVFrame is a superset of AVPicture
	// Install the image cache for the AVFrame object and set the_ The buffer cache is attached to the pframeyuv > data pointer structure
	av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);
  
	// Initialize SWS context for software scaling to set the image conversion pixel format to AV_PIX_FMT_RGB24
	sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL, NULL);

/*--------------Cyclic decoding-------------*/
	i = 0;// Read frames(2 packet) and save first five frames to disk,
	/*-----------------------
	 * read in a packet and store it in the AVPacket struct
	 * ffmpeg allocates the internal data for us,which is pointed to by packet.data
	 * this is freed by the av_free_packet()
	 -----------------------*/
	while (av_read_frame(pFormatCtx, &packet) >= 0) {//Each image coding data packet is read from video file or network streaming media and stored in AVPacket data structure
		// Is this a packet from the video stream
		if (packet.stream_index == videoStream) {
		    /*-----------------------
	 		 * Decode video frame，Decode a complete frame of data and set frameFinished to true
			 * You may not be able to get a complete video frame by decoding only one packet. You may need to read multiple packets
	 		 * avcodec_decode_video2()frameFinished is set to true when decoding to a full frame
			 * Technically a packet can contain partial frames or other bits of data
			 * ffmpeg's parser ensures that the packets we get contain either complete or multiple frames
			 * convert the packet to a frame for us and set frameFinisned for us when we have the next frame
	 	 	 -----------------------*/
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

			// Did we get a video frame to check whether a complete image is decoded
			if (frameFinished) {
				// Convert the image from its native format to RGB, / / convert the decoded image into RGB24 format
				sws_scale(sws_ctx, (uint8_t const * const *) pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

				if (++i <= 5) {// Save the frame to disk to save the first five frames of images to disk
					SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i);
				}
			}
		}
		// Free the packet that was allocated by av_read_frame to release the encoded data pointer in AVPacket data structure
		av_packet_unref(&packet);
	}

/*--------------Parameter undo-------------*/
	// Free the RGB image buffer
	av_free(buffer);
	av_frame_free(&pFrameRGB);

	// Free the YUV frame.
	av_frame_free(&pFrame);

	// Close the codecs.
	avcodec_close(pCodecCtx);
	avcodec_close(pCodecCtxOrig);

	// Close the video file.
	avformat_close_input(&pFormatCtx);

	return 0;
}
