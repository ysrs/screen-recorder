#include "XVideoWriter.h"

#include <iostream>

extern "C"
{
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}


#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "swresample.lib")


using namespace std;
class CXVideoWriter : public XVideoWriter
{
public:
	bool Init(const char *file) override
	{
		Close();

		// ��װ�ļ������������
		avformat_alloc_output_context2(&ic, nullptr, nullptr, file);
		if (!ic)
		{
			cerr << "avformat_alloc_output_context2 failed!" << endl;
			return false;
		}

		fileName = file;

		return true;
	}

	void Close() override
	{
		if (ic)
		{
			avformat_close_input(&ic);
		}

		if (vc)
		{
			// �����ȹرգ����ͷ�
			avcodec_close(vc);
			avcodec_free_context(&vc);
		}

		if (ac)
		{
			avcodec_close(ac);
			avcodec_free_context(&ac);
		}

		if (vsc)
		{
			sws_freeContext(vsc);
			vsc = nullptr;
		}

		if (asc)
		{
			swr_free(&asc);
		}

		if (yuv)
		{
			av_frame_free(&yuv);
		}

		if (pcm)
		{
			av_frame_free(&pcm);
		}
	}

	bool AddVideoStream() override
	{
		if (!ic)
		{
			return false;
		}
		// 1. ��Ƶ����������
		AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!codec)
		{
			cerr << "avcodec_find_encoder AV_CODEC_ID_H264 failed!" << endl;
			return false;
		}

		vc = avcodec_alloc_context3(codec);
		if (!vc)
		{
			cerr << "avcodec_alloc_context3 AV_CODEC_ID_H264 failed!" << endl;
			return false;
		}
		// �����ʣ�ѹ����ÿ���С
		vc->bit_rate = vBitRate;
		vc->width = outWidth;
		vc->height = outHeight;

		// ʱ�����
		vc->time_base = { 1,outFPS };
		vc->framerate = { outFPS,1 };

		// �������С������֡һ���ؼ�֡
		vc->gop_size = 50;

		// 
		vc->max_b_frames = 0;

		vc->pix_fmt = AV_PIX_FMT_YUV420P;
		vc->codec_id = AV_CODEC_ID_H264;
		
		av_opt_set(vc->priv_data, "preset", "superfast", 0);
		vc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		//vc->thread_count = 8;

		// �򿪱�����
		int ret = avcodec_open2(vc, codec, nullptr);
		if (ret != 0)
		{
			cerr << "avcodec_open2 failed!" << endl;
			return false;
		}
		cout << "avcodec_open2 success!" << endl;

		// �����Ƶ�������������
		vs = avformat_new_stream(ic, nullptr);
		vs->codecpar->codec_tag = 0;
		avcodec_parameters_from_context(vs->codecpar, vc);

		av_dump_format(ic, 0, fileName.c_str(), 1);

		// ���أ��ߴ磩ת��������,rgb to yuv
		vsc = sws_getCachedContext(
			vsc,
			inWidth, inHeight, (AVPixelFormat)inPixFmt,
			outWidth, outHeight, AV_PIX_FMT_YUV420P,
			SWS_BICUBIC,
			nullptr, nullptr, nullptr
		);
		if (!vsc)
		{
			cerr << "sws_getCachedContext failed!" << endl;
			return false;
		}
		cout << "sws_getCachedContext success!" << endl;

		if (!yuv)
		{
			yuv = av_frame_alloc();
			yuv->format = AV_PIX_FMT_YUV420P;
			yuv->width = outWidth;
			yuv->height = outHeight;
			yuv->pts = 0;
			ret = av_frame_get_buffer(yuv, 32);
			if (ret != 0)
			{
				cerr << "av_frame_get_buffer failed!" << endl;
				return false;
			}
			cout << "av_frame_get_buffer success!" << endl;
		}

		return true;
	}

	bool AddAudioStream() override
	{
		if (!ic)
		{
			return false;
		}

		// �ҵ���Ƶ����
		AVCodec * codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
		if (!codec)
		{
			cerr << "avcodec_find_encoder failed!" << endl;
			return false;
		}
		cout << "avcodec_find_encoder success!" << endl;

		// 1. ����������Ƶ������
		ac = avcodec_alloc_context3(codec);
		if (!ac)
		{
			cerr << "avcodec_alloc_context3 failed!" << endl;
			return false;
		}
		cout << "avcodec_alloc_context3 success!" << endl;

		// ���ò���
		ac->bit_rate = aBitrate;
		ac->sample_rate = outSampleRate;
		ac->sample_fmt = (AVSampleFormat)outSampleFmt;
		ac->channels = outChannels;
		ac->channel_layout = av_get_default_channel_layout(outChannels);
		ac->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		
		int ret = avcodec_open2(ac, codec, nullptr);
		if (ret != 0)
		{
			avcodec_free_context(&ac);
			cerr << "avcodec_open2 failed!" << endl;
			return false;
		}
		cout << "avcodec_open2 AV_CODEC_ID_AAC success!" << endl;

		// 2. ������Ƶ��
		as = avformat_new_stream(ic, nullptr);
		if (!as)
		{
			cerr << "avformat_new_stream failed!" << endl;
			return false;
		}
		cout << "avformat_new_stream success!" << endl;

		as->codecpar->codec_tag = 0;
		ret = avcodec_parameters_from_context(as->codecpar, ac);
		if (ret < 0)
		{
			cerr << "avcodec_parameters_from_context failed!" << endl;
			return false;
		}
		cout << "avcodec_parameters_from_context success!" << endl;

		av_dump_format(ic, 0, fileName.c_str(), 1);

		// 3. ��Ƶ�ز��������Ĵ���
		asc = swr_alloc_set_opts(
			asc,
			ac->channel_layout, ac->sample_fmt, ac->sample_rate,	// �����ʽ
			av_get_default_channel_layout(inChannels), (AVSampleFormat)inSampleFmt, inSampleRate,
			0, nullptr
		);
		ret = swr_init(asc);
		if (ret != 0)
		{
			cerr << "swr_init failed!" << endl;
			return false;
		}
		cout << "swr_init success!" << endl;

		// 4. ��Ƶ�ز��������AVFrame
		if (!pcm)
		{
			pcm = av_frame_alloc();
			pcm->format = ac->sample_fmt;
			pcm->channels = ac->channels;
			pcm->channel_layout = ac->channel_layout;
			pcm->nb_samples = nb_sample;	// һ֡��Ƶ����������
			ret = av_frame_get_buffer(pcm, 0);
			if (ret < 0)
			{
				cerr << "av_frame_get_buffer failed!" << endl;
				return false;
			}
			cout << "av_frame_get_buffer success!" << endl;
		}

		cout << "audio AVFrame create success!" << endl;
	
		return true;
	}

	AVPacket *EncodeVideo(const unsigned char *rgb) override
	{
		if (!ic || !vsc || !yuv)
		{
			return nullptr;
		}

		AVPacket *pkt = nullptr;
		uint8_t *inData[AV_NUM_DATA_POINTERS] = { nullptr };
		inData[0] = (uint8_t *)rgb;
		int inSize[AV_NUM_DATA_POINTERS] = { 0 };
		inSize[0] = inWidth * 4;
		// rgb to yuv
		int h = sws_scale(vsc, inData, inSize, 0, inHeight,
			yuv->data, yuv->linesize);
		if (h < 0)
		{
			return nullptr;
		}
		//cout << h << "|";
		yuv->pts = vpts;
		vpts++;

		// encode,�ڶ�������ֱ�Ӵ���yuv���ᶪʧ���֡���ݣ�һ���β�ļ�֡���ݲ�̫��Ҫ�����Դ������ֱ�Ӷ�����
		// ����봦��Ļ�����Ҫ���ڶ���������nullptr����ȡ����֡���ݣ��Ժ����о���
		int ret = avcodec_send_frame(vc, yuv);
		if (ret != 0)
		{
			return nullptr;
		}
		pkt = av_packet_alloc();
		ret = avcodec_receive_packet(vc, pkt);
		if (ret != 0 || pkt->size <= 0)
		{
			av_packet_free(&pkt);
			return nullptr;
		}

		av_packet_rescale_ts(pkt, vc->time_base, vs->time_base);
		pkt->stream_index = vs->index;

		return pkt;
	}

	AVPacket *EncodeAudio(const unsigned char *d) override
	{
		if (!ic || !asc || !pcm)
		{
			return nullptr;
		}
		// 1 ��Ƶ�ز���
		const uint8_t *data[AV_NUM_DATA_POINTERS] = { nullptr };
		data[0] = (uint8_t *)d;
		int len = swr_convert(asc, pcm->data, pcm->nb_samples,
			data, pcm->nb_samples);
		//cout << len << "*";

		// 2 ��Ƶ�ı���
		int ret = avcodec_send_frame(ac, pcm);
		if (ret != 0)
		{
			return nullptr;
		}

		AVPacket *pkt = av_packet_alloc();
		av_init_packet(pkt);
		ret = avcodec_receive_packet(ac, pkt);
		if (ret != 0)
		{
			av_packet_free(&pkt);
			return nullptr;
		}
		cout << pkt->size << "|";
		pkt->stream_index = as->index;

		pkt->pts = apts;
		pkt->dts = pkt->pts;
		apts += av_rescale_q(pcm->nb_samples, { 1, ac->sample_rate }, ac->time_base);

		return pkt;
	}

	bool WriteHeader() override
	{
		if (!ic)
		{
			return false;
		}

		// ��io
		int ret = avio_open(&ic->pb, fileName.c_str(), AVIO_FLAG_WRITE);
		if (ret != 0)
		{
			cerr << "avio_open failed!" << endl;
			return false;
		}
		cout << "avio_open success!" << endl;

		// д���װͷ
		ret = avformat_write_header(ic, nullptr);
		if (ret != 0)
		{
			cerr << "avformat_write_header failed!" << endl;
			return false;
		}
		cout << "avformat_write_header success!" << endl;

		return true;
	}

	bool WriteFrame(AVPacket *pkt) override
	{
		if (!ic || !pkt || pkt->size<=0)
		{
			return false;
		}
		if (av_interleaved_write_frame(ic, pkt) != 0)
		{
			return false;
		}

		return true;
	}

	bool WriteEnd() override
	{
		if (!ic || !ic->pb)
		{
			return false;
		}

		// д��β����Ϣ����
		int ret = av_write_trailer(ic);
		if (ret != 0)
		{
			cerr << "av_write_trailer failed!" << endl;
			return false;
		}
		cout << "av_write_trailer success!" << endl;

		// �ر�����io
		ret = avio_closep(&ic->pb);
		//ret = avio_close(ic->pb);
		if (ret != 0)
		{
			cerr << "avio_close failed!" << endl;
			return false;
		}
		cout << "avio_close success!" << endl;

		cout << "WriteEnd success!" << endl;

		return true;
	}

	bool IsVideoBefore() override
	{
		if (!ic || !asc || !vsc)
		{
			return false;
		}
		int ret = av_compare_ts(vpts, vc->time_base, apts, ac->time_base);
		if (ret <= 0)
		{
			return true;
		}
	
		return false;
	}

public:
	// ��װmp4���������
	AVFormatContext *ic = nullptr;
	// ��Ƶ������������
	AVCodecContext *vc = nullptr;
	// ��Ƶ������������
	AVCodecContext *ac = nullptr;
	// ��Ƶ��
	AVStream *vs = nullptr;
	// ��Ƶ��
	AVStream *as = nullptr;
	// ����ת����������
	SwsContext *vsc = nullptr;
	// ��Ƶ�ز���������
	SwrContext *asc = nullptr;
	// ���yuv
	AVFrame *yuv = nullptr;
	// �ز����������pcm
	AVFrame *pcm = nullptr;
	// ��Ƶpts
	int vpts = 0;
	// ��Ƶpts
	int apts = 0;
};


XVideoWriter::XVideoWriter()
{
}


XVideoWriter::~XVideoWriter()
{
}


XVideoWriter *XVideoWriter::Get(unsigned short index)
{
	static  bool isFirst = true;
	if (isFirst)
	{
		av_register_all();
		avcodec_register_all();

		isFirst = false;
	}


	static CXVideoWriter wrs[65535];
	return &wrs[index];
}