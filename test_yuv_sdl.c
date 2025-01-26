//
// Created by Admin on 2024/10/28.
//
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "SDL2/SDL.h"

//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)

#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)

int thread_exit=0;

int sfp_refresh_thread(void *opaque){
    thread_exit=0;
    while (!thread_exit) {
        SDL_Event event;
        event.type = SFM_REFRESH_EVENT;
        SDL_PushEvent(&event);
        SDL_Delay(20);
    }
    thread_exit=0;
    //Break
    SDL_Event event;
    event.type = SFM_BREAK_EVENT;
    SDL_PushEvent(&event);

    return 0;
}

#undef main
int main(int argc, char* argv[])
{

    AVFormatContext	*pFormatCtx;
    int				i, videoindex;
    AVCodecContext	*pCodecCtx;
    AVCodec			*pCodec;
    AVFrame	*pFrame,*pFrameYUV;
    int  buf_size;
    uint8_t *out_buffer;
    AVPacket *packet;
    int ret, got_picture;

    //------------SDL----------------
    int screen_w,screen_h;
    SDL_Window *screen;
    SDL_Renderer* sdlRenderer;
    SDL_Texture* sdlTexture;
    SDL_Rect sdlRect;
    SDL_Thread *video_tid;
    SDL_Event event;

    struct SwsContext *img_convert_ctx;

    char filepath[]="Titanic.ts";

    // AVFormatContext holds the header information from the format (Container)
    // Allocating memory for this component
    // http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html
    pFormatCtx = avformat_alloc_context();

    // Open the file and read its header. The codecs are not opened.
    // The function arguments are:
    // AVFormatContext (the component we allocated memory for),
    // url (filename),
    // AVInputFormat (if you pass NULL it'll do the auto detect)
    // and AVDictionary (which are options to the demuxer)
    // http://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga31d601155e9035d5b0e7efedc894ee49
    if(avformat_open_input(&pFormatCtx,filepath,NULL,NULL)!=0){
        printf("Couldn't open input stream.\n");
        return -1;
    }

    // now we have access to some information about our file
    // since we read its header we can say what format (container) it's
    // and some other information related to the format itself.
    printf("format: %s, duration: %lld, bit_rate: %lld\n", pFormatCtx->iformat->name, pFormatCtx->duration, pFormatCtx->bit_rate);
    printf("long_name: %s, extensions: %s\n", pFormatCtx->iformat->long_name, pFormatCtx->iformat->extensions);

    // read Packets from the Format to get stream information
    // this function populates pFormatContext->streams
    // (of size equals to pFormatContext->nb_streams)
    // the arguments are:
    // the AVFormatContext
    // and options contains options for codec corresponding to i-th stream.
    // On return each dictionary will be filled with options that were not found.
    // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#gad42172e27cddafb81096939783b157bb
    if(avformat_find_stream_info(pFormatCtx,NULL)<0){
        printf("Couldn't find stream information.\n");
        return -1;
    }
    videoindex=-1;

    AVCodecParameters *pCodecParameters =  NULL;

    printf("finding stream info from format.\n");

    // loop though all the streams and print its main information
    for (int i = 0; i < pFormatCtx->nb_streams; i++)
    {
        AVCodecParameters *pLocalCodecParameters =  NULL;
        pLocalCodecParameters = pFormatCtx->streams[i]->codecpar;
        printf("AVStream->time_base before open coded %d/%d\n", pFormatCtx->streams[i]->time_base.num, pFormatCtx->streams[i]->time_base.den);
        printf("AVStream->r_frame_rate before open coded %d/%d\n", pFormatCtx->streams[i]->r_frame_rate.num, pFormatCtx->streams[i]->r_frame_rate.den);
        printf("AVStream->start_time %lld\n" PRId64, pFormatCtx->streams[i]->start_time);
        printf("AVStream->duration %lld\n" PRId64, pFormatCtx->streams[i]->duration);

        printf("finding the proper decoder (CODEC)");

        AVCodec *pLocalCodec = NULL;

        // finds the registered decoder for a codec ID
        // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
        pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

        if (pLocalCodec==NULL) {
            printf("ERROR unsupported codec!");
            // In this example if the codec is not found we just skip it
            continue;
        }

        // when the stream is a video we store its index, codec parameters and codec
        if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            if (videoindex == -1) {
                videoindex = i;
                pCodec = pLocalCodec;
                pCodecParameters = pLocalCodecParameters;
            }

            printf("Video Codec: resolution %d x %d\n", pLocalCodecParameters->width, pLocalCodecParameters->height);
        } else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            printf("Audio Codec: %d channels, sample rate %d\n", pLocalCodecParameters->channels, pLocalCodecParameters->sample_rate);
        }

        // print its name, id and bitrate
        printf("\tCodec %s ID %d bit_rate %lld\n", pLocalCodec->name, pLocalCodec->id, pLocalCodecParameters->bit_rate);
    }

    if(videoindex==-1){
        printf("Didn't find a video stream.\n");
        return -1;
    }


    pCodecCtx = avcodec_alloc_context3(pCodec);

    if (!pCodecCtx)
    {
        printf("failed to allocated memory for AVCodecContext\n");
        return -1;
    }

    // Fill the codec context based on the values from the supplied codec parameters
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
    if (avcodec_parameters_to_context(pCodecCtx, pCodecParameters) < 0)
    {
        printf("failed to copy codec params to codec context\n");
        return -1;
    }

    // Initialize the AVCodecContext to use the given AVCodec.
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    {
        printf("failed to open codec through avcodec_open2\n");
        return -1;
    }


    /*
    * 在此处添加输出视频信息的代码
    * 取自于pFormatCtx，使用fprintf()
    */
    pFrame=av_frame_alloc();
    pFrameYUV=av_frame_alloc();

    // 为AVFrame.*data[]手工分配缓冲区，用于存储sws_scale()中目的帧视频数据
    //     pFrame的data_buffer由av_read_frame()分配，因此不需手工分配
    //     pFrameYUV的data_buffer无处分配，因此在此处手工分配
    buf_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
                                        pCodecCtx->width,
                                        pCodecCtx->height,
                                        1
    );
    // buffer将作为p_frm_yuv的视频数据缓冲区
    out_buffer = (uint8_t *)av_malloc(buf_size);
    // 使用给定参数设定pFrameYUV->data和pFrameYUV->linesize
    av_image_fill_arrays(pFrameYUV->data,           // dst data[]
                         pFrameYUV->linesize,       // dst linesize[]
                         out_buffer,                    // src buffer
                         AV_PIX_FMT_YUV420P,        // pixel format
                         pCodecCtx->width,        // width
                         pCodecCtx->height,       // height
                         1                          // align
    );
    packet=(AVPacket *)av_malloc(sizeof(AVPacket));
    //Output Info-----------------------------
    printf("--------------- File Information ----------------\n");
    av_dump_format(pFormatCtx,0,filepath,0);
    printf("-------------------------------------------------\n");
    // A7. 初始化SWS context，用于后续图像转换
    //     此处第6个参数使用的是FFmpeg中的像素格式，对比参考注释B4
    //     FFmpeg中的像素格式AV_PIX_FMT_YUV420P对应SDL中的像素格式SDL_PIXELFORMAT_IYUV
    //     如果解码后得到图像的不被SDL支持，不进行图像转换的话，SDL是无法正常显示图像的
    //     如果解码后得到图像的能被SDL支持，则不必进行图像转换
    //     这里为了编码简便，统一转换为SDL支持的格式AV_PIX_FMT_YUV420P==>SDL_PIXELFORMAT_IYUV
    img_convert_ctx = sws_getContext(pCodecCtx->width,    // src width
                                     pCodecCtx->height,   // src height
                                     pCodecCtx->pix_fmt,  // src format
                                     pCodecCtx->width,    // dst width
                                     pCodecCtx->height,   // dst height
                                     AV_PIX_FMT_YUV420P,    // dst format
                                     SWS_BICUBIC,           // flags
                                     NULL,                  // src filter
                                     NULL,                  // dst filter
                                     NULL                   // param
    );

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
        printf( "Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }
    //SDL 2.0 Support for multiple windows
    screen_w = pCodecCtx->width;
    screen_h = pCodecCtx->height;
    screen = SDL_CreateWindow("Simplest ffmpeg player's Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              screen_w, screen_h,SDL_WINDOW_OPENGL);

    if(!screen) {
        printf("SDL: could not create window - exiting:%s\n",SDL_GetError());
        return -1;
    }
    sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
    //IYUV: Y + U + V  (3 planes)
    //YV12: Y + V + U  (3 planes)
    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,pCodecCtx->width,pCodecCtx->height);

    sdlRect.x=0;
    sdlRect.y=0;
    sdlRect.w=screen_w;
    sdlRect.h=screen_h;

    packet=(AVPacket *)av_malloc(sizeof(AVPacket));

    video_tid = SDL_CreateThread(sfp_refresh_thread,NULL,NULL);
    //------------SDL End------------
    //Event Loop
    for (;;) {
        //Wait
        SDL_WaitEvent(&event);
        if(event.type==SFM_REFRESH_EVENT){
            //------------------------------
            if(av_read_frame(pFormatCtx, packet)>=0){

                if(packet->stream_index==videoindex) {

                    // Supply raw packet data as input to a decoder
                    // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3
                    ret = avcodec_send_packet(pCodecCtx, packet);
                    if (ret < 0) {
                        printf("Error while sending a packet to the decoder: %s\n", av_err2str(ret));
                        return ret;
                    }

                    while (ret >= 0) {
                        // Return decoded output data (into a frame) from a decoder
                        // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c
                        ret = avcodec_receive_frame(pCodecCtx, pFrame);
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                            break;
                        } else if (ret < 0) {
                            printf("Error while receiving a frame from the decoder: %s \n", av_err2str(ret));
                            return ret;
                        }


                        sws_scale(img_convert_ctx, (const uint8_t *const *) pFrame->data, pFrame->linesize, 0,
                                  pCodecCtx->height,
                                  pFrameYUV->data, pFrameYUV->linesize);

                        //SDL---------------------------
                        SDL_UpdateTexture(sdlTexture, NULL, pFrameYUV->data[0], pFrameYUV->linesize[0]);
                        SDL_RenderClear(sdlRenderer);
                        //SDL_RenderCopy( sdlRenderer, sdlTexture, &sdlRect, &sdlRect );
                        SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
                        SDL_RenderPresent(sdlRenderer);
                        //SDL End-----------------------

                    }

                    av_packet_unref(packet);
                    av_frame_unref(pFrame);
                }
            }else{
                //Exit Thread
                thread_exit=1;
            }
        }else if(event.type==SDL_QUIT){
            thread_exit=1;
        }else if(event.type==SFM_BREAK_EVENT){
            break;
        }

    }

    sws_freeContext(img_convert_ctx);
    SDL_Quit();
    //--------------
    av_frame_free(&pFrameYUV);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    return 0;
}