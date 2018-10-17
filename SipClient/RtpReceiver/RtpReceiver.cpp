#include "RtpReceiver.h"
#include "DataRepository\include\DataRepository.h"

extern char g_ClientIp[20]; //sip UA IP
extern char g_ClientId[20]; //sip UA Id

//���PS�������ݲֿ⣬ÿһ������洢һ֡������PS��
extern CDataRepository<unsigned char*> g_PsPacketRepo;

//���ES�������ݲֿ⣬ÿһ������洢һ֡������ES��
extern CDataRepository<unsigned char*> g_EsPacketRepo;

CRtpReceiver::CRtpReceiver(unsigned short rtpPort)
    :m_mediaPort(rtpPort)
{
    m_offset = 0;
}

CRtpReceiver::~CRtpReceiver()
{
}

char* CRtpReceiver::getFrame()
{
    return nullptr;
}

char* CRtpReceiver::getSdpInfo()
{
    return m_SdpInfo;
}

int CRtpReceiver::generateSdpInfo()
{
    char pMediaPort[10] = { 0 };
    _itoa_s(m_mediaPort, pMediaPort, 10);

    char pSsrc[50] = { 0 };
    sprintf_s(pSsrc, "%s", "999999");

    sprintf_s(m_SdpInfo, 4 * 1024,
        "v=0\r\n"
        "o=%s 0 0 IN IP4 %s\r\n"
        "s=Play\r\n"
        "c=IN IP4 %s\r\n"
        "t=0 0\r\n"
        "m=video %s RTP/AVP 96 98 97\r\n"
        "a=recvonly\r\n"
        "a=rtpmap:96 PS/90000\r\n"
        "a=rtpmap:98 H264/90000\r\n"
        "a=rtpmap:97 MPEG-4/90000\r\n"
        "y=%s\r\n"
        "f=\r\n",
        g_ClientId,
        g_ClientIp,
        g_ClientIp,
        pMediaPort,
        pSsrc);

    return 0;
}

uint16_t CRtpReceiver::getMediaPort()
{
    return m_mediaPort;
}

int CRtpReceiver::StartProc()
{
    m_threadHandle = (HANDLE)_beginthread(ThreadProc, 0, (void*)this);
    if (0 == m_threadHandle)
    {
        //�߳�����ʧ��
        return -1;
    }
    m_bThreadRuning = true;

    //����sdp��Ϣ
    generateSdpInfo();

    return 0;
}

void CRtpReceiver::StopProc()
{
    m_bThreadRuning = false;

    m_RtpSession.BYEDestroy(RTPTime(10, 0), 0, 0);
}

void CRtpReceiver::ThreadProc(void* pParam)
{
    CRtpReceiver* pThis = (CRtpReceiver*)pParam;

    (pThis->m_Sessparams).SetOwnTimestampUnit(1.0 / 8000.0);
    (pThis->m_Sessparams).SetAcceptOwnPackets(true);
    (pThis->m_Transparams).SetPortbase(pThis->m_mediaPort);

    int status, i, num;

    status = (pThis->m_RtpSession).Create((pThis->m_Sessparams), &(pThis->m_Transparams));

    while (pThis->m_bThreadRuning)
    {
        (pThis->m_RtpSession).BeginDataAccess();

        // check incoming packets
        if ((pThis->m_RtpSession).GotoFirstSourceWithData())
        {
            do
            {
                RTPPacket *pack;

                while ((pack = (pThis->m_RtpSession).GetNextPacket()) != NULL)
                {
                    //pThis->assemleFrame(pack);
                    pThis->handlePacket(pack);
                    // we don't longer need the packet, so
                    // we'll delete it
                    (pThis->m_RtpSession).DeletePacket(pack);
                }
            } while ((pThis->m_RtpSession).GotoNextSourceWithData());
        }
        (pThis->m_RtpSession).EndDataAccess();
        RTPTime::Wait(RTPTime(1, 0));
    }
}

int CRtpReceiver::handlePacket(RTPPacket* packet)
{
    int packetSize = 0;
    uint8_t packetPayloadType;

    if (NULL == packet)
    {
        return 0;
    }

    packetSize = packet->GetPacketLength();
    packetPayloadType = packet->GetPayloadType();

    //���������ʹ���������
    switch (packetPayloadType)
    {
    case PS: //96
    {
        handlePsPacket( packet );
        break;
    }
    case MPEG4: //97
    {
        break;
    }
    case H264: //98
    {
        break;
    }
    case SVAC: //99
    {
        break;
    }
    default:
    {
        break;
    }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

union littel_endian_size
{
    unsigned short int  length;
    unsigned char       byte[2];
};

typedef struct pes_pack_start_code
{
    unsigned char start_code[3];
    unsigned char stream_id[1];
} pes_pack_start_code_t;

typedef struct pes_pack_header
{
    pes_pack_start_code start_code;
    littel_endian_size packet_size;
} pes_pack_header_t;

struct pack_start_code
{
    unsigned char start_code[3];
    unsigned char stream_id[1];
};

struct program_stream_pack_header
{
    unsigned char ps_packet_start_code[4];   
    unsigned char Buf[9];                   // ps��ͷ�����ݣ���������ϸ����
    littel_endian_size pack_stuffed_data; // ps���е�14���ֽڵĺ�3λ����˵��������ݵĳ���
};

int get_ps_header_size(unsigned char* pPacket)
{
    int length = 0;

    if (pPacket
        && pPacket[0] == 0x00
        && pPacket[1] == 0x00
        && pPacket[2] == 0x01
        && pPacket[3] == 0xba)
    {
        program_stream_pack_header *PsHead = (program_stream_pack_header *)pPacket;
        unsigned char pack_stuffed_length = PsHead->pack_stuffed_data.length & 0x07;

        return sizeof(program_stream_pack_header) + pack_stuffed_length;
    }
    else
    {
        // not a PS Packet
        return 0;
    }
}

int get_system_header_size(unsigned char* pPacket, int offset)
{
    if (pPacket
        && pPacket[0] == 0x00
        && pPacket[1] == 0x00
        && pPacket[2] == 0x01
        && pPacket[3] == 0xbb)
    {

        program_stream_pack_header *PsHead = (program_stream_pack_header *)((unsigned char*)pPacket + offset);
        unsigned char pack_stuffed_length = PsHead->pack_stuffed_data.length & 0x07;

        return sizeof(program_stream_pack_header) + pack_stuffed_length;
    }
    else
    {
        // not a system header PES Packet
        return 0;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CRtpReceiver::handlePsPacket(RTPPacket* packet)
{
    if (NULL == packet)
    {
        return 0;
    }

    if (packet->HasMarker())   //�����ݰ�, asMarker() Returns true is the marker bit was set
    {
        //���յ�������һ֡��������Ƶ֡���У��������������������ռ��ͷţ�����һ֡���ݴ�š�
        memcpy(m_pFrame + m_offset, packet->GetPayloadData(), packet->GetPayloadLength());
        m_frameSize = m_offset + packet->GetPayloadLength();
        m_pTmpFrame = new uint8_t(m_frameSize);

        g_PsPacketRepo.putData(m_pTmpFrame);    //��PS���ֿ�
        //memcpy(m_pTmpFrame, m_pFrame, m_frameSize);

        writeLog("E://buf_mediaplay.h264", m_pFrame, m_frameSize);
        writeLog("E://src_mediaplay.h264", packet->GetPayloadData(), packet->GetPayloadLength());

        int ps_packet_header_size = get_ps_header_size(m_pFrame);
        int system_header_size = get_system_header_size(m_pFrame, ps_packet_header_size);

        m_frameSize = 0;
        m_offset = 0;
    }
    else
    {
        memcpy(m_pFrame + m_offset, packet->GetPayloadData(), packet->GetPayloadLength());
        m_offset += packet->GetPayloadLength();
        writeLog("E://src_mediaplay.h264", packet->GetPayloadData(), packet->GetPayloadLength());
    }
    return packet->GetPayloadLength();
}

int CRtpReceiver::handleMPEG4Packet(RTPPacket* packet)
{
    return 0;
}

int CRtpReceiver::handleH264Packet(RTPPacket* packet)
{
    return 0;
}

void CRtpReceiver::writeLog(char* file_name, void* pLog, int nLen)
{
    if (pLog != NULL && nLen > 0)
    {
        if (NULL == m_pLogFile && strlen(file_name) > 0)
        {
            ::fopen_s(&m_pLogFile, file_name, "a+");
        }

        if (m_pLogFile != NULL)
        {
            ::fwrite(pLog, nLen, 1, m_pLogFile);
            ::fclose(m_pLogFile);
            m_pLogFile = NULL;
        }
    }
}