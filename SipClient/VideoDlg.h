#pragma once

#include "MediaSession\MediaSession.h"
#include "StreamManager\StreamManager.h"
#include "video_decoder\Demuxer.h"

#define STREAM_BUFFER_SIZE (8 * 1024 * 1024)

// CVideoDlg dialog

class CVideoDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CVideoDlg)

public:
	CVideoDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CVideoDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_VIDEODLG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
    unsigned char* m_ScrBuf[4] = { 0 };     //video buffer

    CMediaSession* m_pMediaSession;
    CDemuxer* m_pDemux;
    unsigned char* m_stream_buffer;

public:
    static void PlayThreadProc(void* pParam);   //�̺߳���
    HANDLE m_playThreadHandle;                  //�߳̾��
    bool m_bplayThreadRuning;                   //�߳�����״̬

    bool StartPlay();
    bool StopPlay();
    int Play();
    char* getSdpInfo();
};
