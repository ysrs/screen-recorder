#pragma once
#include <string>



enum XSAMPLEFMT
{
	X_S16 = 1,
	X_FLATP = 8
};

struct AVPacket;
class XVideoWriter
{
public:
	~XVideoWriter();
	static XVideoWriter *Get(unsigned short index = 0);

	virtual bool Init(const char *file) = 0;
	virtual void Close() = 0;
	virtual bool AddVideoStream() = 0;
	virtual bool AddAudioStream() = 0;
	virtual AVPacket *EncodeVideo(const unsigned char *rgb) = 0;
	virtual AVPacket *EncodeAudio(const unsigned char *d) = 0;
	virtual bool WriteHeader() = 0;
	// ���ͷ�pkt�Ŀռ䣬���۳ɹ����
	virtual bool WriteFrame(AVPacket *pkt) = 0;
	virtual bool WriteEnd() = 0;
	virtual bool IsVideoBefore() = 0;

public:
	std::string fileName;
	
	// ��Ƶ�������
	int inWidth = 848;
	int inHeight = 480;
	// AV_PIX_FMT_BGRA
	int inPixFmt = 30;

	// ��Ƶ�������
	int vBitRate = 4000000;
	int outWidth = 848;
	int outHeight = 480;
	int outFPS = 25;


	// ��Ƶ���������ÿ֡����ÿͨ����������
	int nb_sample = 1024;

	// ��Ƶ�������
	int inSampleRate = 44100;
	int inChannels = 2;
	XSAMPLEFMT inSampleFmt = X_S16;

	// ��Ƶ�������
	int aBitrate = 64000;
	int outChannels = 2;
	int outSampleRate = 44100;
	XSAMPLEFMT outSampleFmt = X_FLATP;

protected:
	XVideoWriter();
};

