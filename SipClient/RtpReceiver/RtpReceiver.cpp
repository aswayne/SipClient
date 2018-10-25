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

// PES ��stream_id ֮��������ֽ��д�Ÿ�PES���ĳ���
typedef union littel_endian_size
{
    unsigned short int  length;
    unsigned char       byte[2];
}littel_endian_size_u;

/**
*   PES system header packet header
*/
typedef struct pes_system_header_packet_header
{
    unsigned char system_header_start_code[4];
    littel_endian_size_u header_length;
    unsigned char buf[6];                   //����ֶΣ����ISOIEC 13818-1.pdf Table 2-18
} pes_system_header_packet_header_t;

/**
*   PES program stream map packet header
*/
typedef struct pes_program_stream_map_packet_header
{
    unsigned char packet_start_code_prefix[3];
    unsigned char map_stream_id;
    littel_endian_size_u program_stream_map_length;
    unsigned char buf[6];                   //����ֶΣ����ISOIEC 13818-1.pdf Table 2-18
} pes_program_stream_map_packet_header_t;

/**
*   PES video h264 packet header
*/
typedef struct pes_video_h264_packet_header
{
    unsigned char start_code[3];
    unsigned char stream_id[1];
    littel_endian_size_u packet_size;
    unsigned char buf[2];                   //����ֶΣ����ISOIEC 13818-1.pdf Table 2-18
    unsigned char pes_packet_header_stuff_size;
    unsigned char struct_stuff[3];             //��ֹ�Զ�����, ���¼���ṹ���С����
} pes_video_h264_packet_header_t;

/**
*   PES audio packet header
*/
typedef struct pes_audio_packet_header
{
    unsigned char start_code[3];
    unsigned char stream_id[1];
    littel_endian_size_u packet_size;
    unsigned char buf[2];                   //����ֶΣ����ISOIEC 13818-1.pdf Table 2-18
    unsigned char pes_packet_header_stuff_size;
    unsigned char struct_stuff[3];             //��ֹ�Զ�����, ���¼���ṹ���С����
} pes_audio_packet_header_t;

/**
*   PS packet header
*/
typedef struct ps_packet_header
{
    unsigned char start_code[4];
    unsigned char Buf1[9];              // ps��ͷ�����ݣ���������ϸ����
    unsigned char Buf2;                 // ps���е�14���ֽڵĺ�3λ����˵��������ݵĳ���
}ps_packet_header_t;

int CRtpReceiver::find_next_hx_str(unsigned char* source, int source_length, unsigned char* seed, int seed_length, int* offset)
{
    if (source && seed)
    {
    }
    else
    {
        //failure
        return -1;
    }

    unsigned char* pHeader = source;

    int total_length = source_length;
    int processed_length = 0;

    int src_offset = 0;
    while (total_length - processed_length > seed_length)
    {
        for (int i = 0; i < seed_length && (pHeader[i] == seed[i]); i++)
        {
            if (seed_length - 1 == i)
            {
                //find ok
                *offset = processed_length;
                return 0;
            }
        }

        processed_length++;
        pHeader = source + processed_length;
    }

    return -1;
}


int CRtpReceiver::deal_ps_packet(unsigned char * packet, int length)
{
    int ps_packet_header_size;
    int ps_packet_header_stuffed_size;

    int pes_system_header_header_length;
    int pes_program_stream_map_length;
    int pes_video_h264_packet_size;
    int pes_video_h264_packet_stuffed_size;

    int pes_packet_header_size;
    int pes_packet_header_stuffed_size;

    int packet_total_length = length;
    int packet_processed_length = 0;
    
    unsigned char h264_es_header[4];
    h264_es_header[0] = 0x00;
    h264_es_header[1] = 0x00;
    h264_es_header[2] = 0x01;
    h264_es_header[3] = 0xe0;

    int find_next_h264_return_value;
    int next_h264_es_headet_offset;


    ps_packet_header* ps_head = NULL;
    pes_system_header_packet_header_t* pes_system_header;
    pes_program_stream_map_packet_header_t* pes_psm_header;
    pes_video_h264_packet_header_t* pes_video_h264_packet_header;
    pes_audio_packet_header_t* pes_audio_packet_header;
    //pes_packet_header_t* pes_pack_header;

    unsigned char* next_pes_packet = NULL;

    // deal with ps packet header.
    if (packet)
    {
        ps_head = (ps_packet_header*)packet;

        ps_packet_header_size = sizeof(ps_packet_header);
        ps_packet_header_stuffed_size = ps_head->Buf2 & 0x07;

        packet_processed_length += sizeof(ps_packet_header) + ps_packet_header_stuffed_size;

        next_pes_packet = packet + packet_processed_length;
    }

    while (next_pes_packet && (packet_total_length - packet_processed_length >= 0))
    {
        if (next_pes_packet
            && next_pes_packet[0] == 0x00
            && next_pes_packet[1] == 0x00
            && next_pes_packet[2] == 0x01
            && next_pes_packet[3] == 0xbb)
        {
            //this pes packet is system_header or psm, which data is not usefull for es_h264
            pes_system_header = (pes_system_header_packet_header_t*)next_pes_packet;

            littel_endian_size_u tmp_size;
            tmp_size.byte[0] = pes_system_header->header_length.byte[1];
            tmp_size.byte[1] = pes_system_header->header_length.byte[0];

            pes_system_header_header_length = tmp_size.length;

            // +6��ԭ����pes_packet_header_t�г����ֽ�֮ǰ����6���ֽ�
            packet_processed_length += (6 + pes_system_header_header_length);

            next_pes_packet = packet + packet_processed_length;
        }
        else if (next_pes_packet
            && next_pes_packet[0] == 0x00
            && next_pes_packet[1] == 0x00
            && next_pes_packet[2] == 0x01
            && next_pes_packet[3] == 0xbc)
        {
            //this pes packet is program stream map, which data is not usefull for es_h264
            pes_psm_header = (pes_program_stream_map_packet_header_t*)next_pes_packet;

            littel_endian_size_u tmp_size;
            tmp_size.byte[0] = pes_psm_header->program_stream_map_length.byte[1];
            tmp_size.byte[1] = pes_psm_header->program_stream_map_length.byte[0];

            pes_program_stream_map_length = tmp_size.length;
            pes_packet_header_size = sizeof(pes_program_stream_map_packet_header_t);

            // -6��ԭ����pes_packet_header_t��Ϊ�˻�ȡpes��ͷ�Ĵ�С�ͷ�ֹ�ṹ���Զ���䣬�������6���ֽڵĿռ�
            // +2��ԭ���ǴӺ������񴦻�ȡ��ps������stream_map_lengthָ���Ĵ�С֮�󻹶���2���ֽڲŵ���Ƶ��pes��
            packet_processed_length += pes_packet_header_size - 6 + pes_program_stream_map_length + 2;

            next_pes_packet = packet + packet_processed_length;
        }

        else if (next_pes_packet
            && next_pes_packet[0] == 0x00
            && next_pes_packet[1] == 0x00
            && next_pes_packet[2] == 0x01
            && next_pes_packet[3] == 0xe0)
        {
            //contain video es stream
            pes_video_h264_packet_header = (pes_video_h264_packet_header_t*)next_pes_packet;

            littel_endian_size_u tmp_size;
            tmp_size.byte[0] = pes_video_h264_packet_header->packet_size.byte[1];
            tmp_size.byte[1] = pes_video_h264_packet_header->packet_size.byte[0];

            pes_video_h264_packet_size = tmp_size.length;
            pes_video_h264_packet_stuffed_size = pes_video_h264_packet_header->pes_packet_header_stuff_size;

            // +9 ��ԭ����pes_video_h264_packet_stuffed_size֮ǰ����9���ֽڵ�ͷ������
            // -3 ��ԭ������pes_video_h264_packet����packet_size�ֽ�֮��ĵ������ֽ�˵����ͷ��ʣ�������ֽ���
            //writeLog("E://new_mediaplay.h264",
            //    next_pes_packet + 9 + pes_video_h264_packet_stuffed_size,
            //    pes_video_h264_packet_size - pes_video_h264_packet_stuffed_size - 3);

            find_next_h264_return_value = find_next_hx_str(next_pes_packet + 4,
                packet_total_length - packet_processed_length,
                h264_es_header, 4, &next_h264_es_headet_offset);

            if (0 == find_next_h264_return_value)
            {
                write_media_data_to_file("E://new_mediaplay.h264",
                    next_pes_packet + 9 + pes_video_h264_packet_stuffed_size,
                    next_h264_es_headet_offset - 5 - pes_video_h264_packet_stuffed_size);

                packet_processed_length += (next_h264_es_headet_offset + 9);
                next_pes_packet = packet + packet_processed_length;
            }

            //next_pes_packet = next_pes_packet + pes_packet_size;
        }

        else if (next_pes_packet
            && next_pes_packet[0] == 0x00
            && next_pes_packet[1] == 0x00
            && next_pes_packet[2] == 0x01
            && next_pes_packet[3] == 0xc0)
        {
            //contain audio es stream
            pes_audio_packet_header = (pes_audio_packet_header_t*)next_pes_packet;
        }

        // next ps packet, return.
        else if (next_pes_packet
            && next_pes_packet[0] == 0x00
            && next_pes_packet[1] == 0x00
            && next_pes_packet[2] == 0x01
            && next_pes_packet[3] == 0xba)
        {
            //next ps stream, return , not continue deal
            break;
        }
        else
        {
            // bad data
            break;
        }
    }

    return packet_processed_length;
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

        write_media_data_to_file("E://buf_mediaplay.ps", m_pFrame, m_frameSize);
        write_media_data_to_file("E://src_mediaplay.ps", packet->GetPayloadData(), packet->GetPayloadLength());

        deal_ps_packet(m_pFrame, m_frameSize);

        m_frameSize = 0;
        m_offset = 0;
    }
    else
    {
        memcpy(m_pFrame + m_offset, packet->GetPayloadData(), packet->GetPayloadLength());
        m_offset += packet->GetPayloadLength();
        write_media_data_to_file("E://src_mediaplay.ps", packet->GetPayloadData(), packet->GetPayloadLength());
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

void CRtpReceiver::write_media_data_to_file(char* file_name, void* pLog, int nLen)
{
    if (pLog != NULL && nLen > 0)
    {
        if (NULL == m_pLogFile && strlen(file_name) > 0)
        {
            ::fopen_s(&m_pLogFile, file_name, "ab+");
        }

        if (m_pLogFile != NULL)
        {
            ::fwrite(pLog, nLen, 1, m_pLogFile);
            ::fclose(m_pLogFile);
            m_pLogFile = NULL;
        }
    }
}