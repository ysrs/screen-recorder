#include "XScreenRecord.h"
#include "XCaptureThread.h"
#include "XAudioThread.h"
#include "XVideoWriter.h"

#include <iostream>


using namespace std;
XScreenRecord::XScreenRecord()
{
}


XScreenRecord::~XScreenRecord()
{
}


void XScreenRecord::run()
{
	while (!isExit)
	{
		mutext.lock();
		// д����Ƶ
		char *rgb = XCaptureThread::Get()->GetRGB();
		if (rgb)
		{
			AVPacket*pkt = XVideoWriter::Get()->EncodeVideo((unsigned char *)rgb);
			delete rgb;
			XVideoWriter::Get()->WriteFrame(pkt);
			cout << "@";
		}

		// д����Ƶ
		char *pcm = XAudioThread::Get()->GetPCM();
		if (pcm)
		{
			AVPacket *pkt = XVideoWriter::Get()->EncodeAudio((unsigned char *)pcm);
			delete pcm;
			XVideoWriter::Get()->WriteFrame(pkt);
			cout << "#";
		}

		mutext.unlock();
	}
}

bool XScreenRecord::Start(const char *filename)
{
	if (!filename)
	{
		return false;
	}

	Stop();
	mutext.lock();
	isExit = false;
	// ��ʼ����Ļ¼��
	XCaptureThread::Get()->fps = fps;
	XCaptureThread::Get()->Start();

	// ��ʼ����Ƶ¼��
	XAudioThread::Get()->Start();

	// ��ʼ��������
	XVideoWriter::Get()->inWidth = XCaptureThread::Get()->width;
	XVideoWriter::Get()->inHeight = XCaptureThread::Get()->height;
	XVideoWriter::Get()->outWidth = outWidth;
	XVideoWriter::Get()->outHeight = outHeight;
	XVideoWriter::Get()->outFPS = fps;
	XVideoWriter::Get()->Init(filename);
	XVideoWriter::Get()->AddVideoStream();
	XVideoWriter::Get()->AddAudioStream();
	if (!XVideoWriter::Get()->WriteHeader())
	{
		mutext.unlock();
		Stop();
		return false;
	}

	mutext.unlock();
	start();

	return true;
}

void XScreenRecord::Stop()
{
	mutext.lock();
	isExit = true;
	mutext.unlock();
	wait();

	mutext.lock();
	XVideoWriter::Get()->WriteEnd();
	XVideoWriter::Get()->Close();
	XCaptureThread::Get()->Stop();
	XAudioThread::Get()->Stop();
	mutext.unlock();
}

