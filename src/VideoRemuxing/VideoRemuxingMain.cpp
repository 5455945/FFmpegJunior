#include "common.h"

/*************************************************
	Function:		hello
	Description:	解析命令行传入的参数
	Calls:			无
	Called By:		main
	Input:			(in)argc : 默认命令行参数
					(in)argv : 默认命令行参数					
	Output:			(out)io_param : 解析命令行的结果
	Return:			true : 命令行解析正确
					false : 命令行解析错误
*************************************************/
static bool hello(int argc, char **argv, IOFiles &io_param)
{
	printf("FFMpeg Remuxing Demo.\nCommand format: %s inputfile outputfile\n", argv[0]);
	if (argc != 3)
	{
		printf("Error: command line error, please re-check.\n");
		return false;
	}

	io_param.inputName = argv[1];
	io_param.outputName = argv[2];

	return true;
}


/*************************************************
Function:		main
Description:	视频文件的封装格式转换
1 打开原始文件、扩展名(封装格式)
2 根据输出文件的格式，创建输出视频文件的句柄
3 获取输入视频文件中流的信息，并在输出文件中添加相应的流
4 在输出文件中添加文件头
5 从输入视频文件中读取音视频的包，并写入到输出文件中
6 向输出文件写入文件尾，并进行收尾工作
$(OutDir)..\..\..\video\vdem.mp4 $(OutDir)..\..\..\video\vrem.flv
第一次运行出错，第二次运行成功
ffplay -i ../video/vrem.flv
*************************************************/
int main(int argc, char **argv)
{
	IOFiles io_param;
	if (!hello(argc, argv, io_param))
	{
		return -1;
	}

	AVOutputFormat *ofmt = NULL;
	AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
	AVPacket pkt;
	int ret = 0;

	av_register_all();

	do {
		//按封装格式打开输入视频文件
		if ((ret = avformat_open_input(&ifmt_ctx, io_param.inputName, NULL, NULL)) < 0)
		{
			printf("Error: Open input file failed.\n");
			break;
		}

		//获取输入视频文件中的流信息
		if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0)
		{
			printf("Error: Failed to retrieve input stream information.\n");
			break;
		}
		av_dump_format(ifmt_ctx, 0, io_param.inputName, 0);

		//按照文件名获取输出文件的句柄
		avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, io_param.outputName);
		if (!ofmt_ctx)
		{
			printf("Error: Could not create output context.\n");
			break;
		}
		ofmt = ofmt_ctx->oformat;

		for (unsigned int i = 0; i < ifmt_ctx->nb_streams; i++)
		{
			AVStream *inStream = ifmt_ctx->streams[i];
			AVStream *outStream = avformat_new_stream(ofmt_ctx, inStream->codec->codec);
			if (!outStream)
			{
				printf("Error: Could not allocate output stream.\n");
				break;
			}

			ret = avcodec_copy_context(outStream->codec, inStream->codec);
			outStream->codec->codec_tag = 0;
			if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			{
				outStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
			}
		}

		av_dump_format(ofmt_ctx, 0, io_param.outputName, 1);

		if (!(ofmt->flags & AVFMT_NOFILE))
		{
			ret = avio_open(&ofmt_ctx->pb, io_param.outputName, AVIO_FLAG_WRITE);
			if (ret < 0)
			{
				printf("Error: Could not open output file.\n");
				break;
			}
		}

		ret = avformat_write_header(ofmt_ctx, NULL);
		if (ret < 0)
		{
			printf("Error: Could not write output file header.\n");
			break;
		}

		while (1)
		{
			AVStream *in_stream, *out_stream;

			ret = av_read_frame(ifmt_ctx, &pkt);
			if (ret < 0)
				break;

			in_stream = ifmt_ctx->streams[pkt.stream_index];
			out_stream = ofmt_ctx->streams[pkt.stream_index];

			/* copy packet */
			pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
			pkt.pos = -1;

			ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
			if (ret < 0)
			{
				fprintf(stderr, "Error muxing packet\n");
				break;
			}
			av_free_packet(&pkt);
		}

		av_write_trailer(ofmt_ctx);

	} while (false);
    
	avformat_close_input(&ifmt_ctx);

	/* close output */
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_closep(&ofmt_ctx->pb);

	avformat_free_context(ofmt_ctx);

	if (ret < 0 && ret != AVERROR_EOF) 
	{
		fprintf(stderr, "Error failed to write packet to output file.\n");
		return 1;
	}
	return 0;
}