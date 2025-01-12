#include <stdio.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

int main(void) {

    AVFormatContext	*pFormatCtx;
    int				i, videoindex;
    AVCodecContext	*pCodecCtx;
    AVCodec			*pCodec;
    AVFrame	*pFrame,*pFrameYUV;
    uint8_t *out_buffer;
    AVPacket *packet;
    int y_size;
    int ret, got_picture;
    struct SwsContext *img_convert_ctx;
    //输入文件路径
    char filepath[]="Titanic.ts";

    int frame_cnt;

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
    FILE *info_fp = fopen("info.txt","wb+");
    printf("format: %s, duration: %lld, bit_rate: %lld\n", pFormatCtx->iformat->name, pFormatCtx->duration, pFormatCtx->bit_rate);
    printf("long_name: %s, extensions: %s\n", pFormatCtx->iformat->long_name, pFormatCtx->iformat->extensions);

    fprintf(info_fp,"format: %s, duration: %lld, bit_rate: %lld\n", pFormatCtx->iformat->name, pFormatCtx->duration, pFormatCtx->bit_rate);
    fprintf(info_fp,"long_name: %s, extensions: %s\n", pFormatCtx->iformat->long_name, pFormatCtx->iformat->extensions);

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

    fprintf(info_fp,"width: %d,height: %d\n",pCodecCtx->width,pCodecCtx->height);

     /*
     * 在此处添加输出视频信息的代码
     * 取自于pFormatCtx，使用fprintf()
     */
    pFrame=av_frame_alloc();
    pFrameYUV=av_frame_alloc();

    out_buffer=(uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
    avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
    packet=(AVPacket *)av_malloc(sizeof(AVPacket));
    //Output Info-----------------------------
    printf("--------------- File Information ----------------\n");
    av_dump_format(pFormatCtx,0,filepath,0);
    printf("-------------------------------------------------\n");
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                     pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    FILE *fp_h264 = fopen("test.h264","wb+");
    FILE *fp_yuv = fopen("test_yuv.yuv","wb+");

    frame_cnt=0;
    while(av_read_frame(pFormatCtx, packet)>=0){
        if(packet->stream_index==videoindex){
            /*
             * 在此处添加输出H264码流的代码
             * 取自于packet，使用fwrite()
             */
            fwrite(packet->data,1, packet->size,fp_h264);

            // Supply raw packet data as input to a decoder
            // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga58bc4bf1e0ac59e27362597e467efff3
            ret = avcodec_send_packet(pCodecCtx, packet);
            if (ret < 0) {
                printf("Error while sending a packet to the decoder: %s\n", av_err2str(ret));
                return ret;
            }

            while (ret >= 0)
            {
                // Return decoded output data (into a frame) from a decoder
                // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga11e6542c4e66d3028668788a1a74217c
                ret = avcodec_receive_frame(pCodecCtx, pFrame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    printf("Error while receiving a frame from the decoder: %s \n", av_err2str(ret));
                    return ret;
                }

                /*printf(
                        "Frame %d (type=%c, size=%d bytes, format=%d) pts %lld key_frame %d [DTS %d]",
                        pCodecCtx->frame_number,
                        av_get_picture_type_char(pFrame->pict_type),
                        pFrame->pkt_size,
                        pFrame->format,
                        pFrame->pts,
                        pFrame->key_frame,
                        pFrame->coded_picture_number
                );*/

                sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
                          pFrameYUV->data, pFrameYUV->linesize);
                printf("Decoded frame index: %d,%c\n",frame_cnt,av_get_picture_type_char(pFrame->pict_type));

                /*
                 * 在此处添加输出YUV的代码
                 * 取自于pFrameYUV，使用fwrite()
                 */
                fwrite(pFrameYUV->data[0],1,pCodecCtx->width * pCodecCtx->height,fp_yuv);
                fwrite(pFrameYUV->data[1],1,pCodecCtx->width * pCodecCtx->height/4,fp_yuv);
                fwrite(pFrameYUV->data[2],1,pCodecCtx->width * pCodecCtx->height/4,fp_yuv);

                frame_cnt++;

            }

        }
        av_packet_unref(packet);
        av_frame_unref(pFrame);
    }

    fclose(info_fp);
    fclose(fp_h264);
    fclose(fp_yuv);
    sws_freeContext(img_convert_ctx);

    av_frame_free(&pFrameYUV);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);

    return 0;
}
