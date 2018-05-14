#ifndef __RTPRECEIVER_H__
#define __RTPRECEIVER_H__

//jrtplib headers 
#include "rtpsession.h"
#include "rtppacket.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtperrors.h"
#include "rtpsourcedata.h"

using namespace jrtplib;

typedef struct
{
    //LITTLE_ENDIAN
    unsigned short   v : 2;     // packet type
    unsigned short   p : 1;     // padding flag
    unsigned short   x : 1;     // header extension flag
    unsigned short   cc : 4;    // CSRC count
    unsigned short   m : 1;     // marker bit 
    unsigned short   pt : 7;    // payload type
    unsigned short   seq;       // sequence number
    unsigned long    ts;        // timestamp
    unsigned long    ssrc;      //synchronization source
}rtp_hdr_t;

class CRtpReceiver
{
public:
    CRtpReceiver(unsigned short rtpPort = 9000);
    ~CRtpReceiver();

    char* getFrame();
    char* getSdpInfo();
    int generateSdpInfo();
    uint16_t getMediaPort();

    /**
    *   ���ܣ�
    *       �����յ��İ�ƴװ��������һ֡����
    *   function��
    *       assemle packet data to an full Frame
    */
    int assemleFrame(RTPPacket* packet);

    static void ThreadProc(void* pParam);   //�̺߳���
    int StartProc();
    void StopProc();
    HANDLE m_threadHandle;  //�߳̾��
    bool m_bThreadRuning;   //�߳�����״̬

    void writeLog(const char* pLog, int nLen);

private:
    RTPUDPv4TransmissionParams m_Transparams;
    RTPSessionParams m_Sessparams;
    RTPSession m_RtpSession;
    char m_SdpInfo[4 * 1024] = { 0 };
    uint16_t m_mediaPort;
    uint8_t m_pFrame[100 * 1024];   //�����ȡ����������Ƶ֡����������ظ�������Ƶ֡�����Խ��ÿռ�����
    int m_offset;                   //λ��
    bool m_isMarkerPacket;              //����֡rtp��ͷ���

    FILE* m_pLogFile;
};

#endif
