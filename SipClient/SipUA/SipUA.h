#ifndef __SIPUA_H__
#define __SIPUA_H__

/**
*   ������
*       ʵ��rfc3261�й涨��sipUA   
*
*   description:
*       implement sipUA, which is descripted in rfc3261.
*   author: YangDong
*   date:   2018-05-02
*   email:  heibao111728@126.com
*/

#include <Windows.h>
#include <process.h>
#include <assert.h>
#include "SipStack\include\eXosip2\eXosip.h"

typedef struct _SipSvrInfo
{
    char svrid[30] = { 0 };//sipSvrID
    char svrip[20] = { 0 };//sipSvrIP
    char authPwd[50] = { 0 };//�ͻ�����֤����
    int svrPort; //sipSvr �����˿�
} ST_SipSvrInfo;

class CSipUA
{
public:
    CSipUA();
    ~CSipUA();

    /**
    *   ���ܣ�
    *       ��ʼ��osipЭ��ջ����ʼ����������ͻ��˻�����Ϣ
    *       ����ֵ��0���ɹ���-1��ʧ��
    *   function:
    *       initiation osip stack and init sipsvr and sipua info 
    *       return:0, success; -1, failure
    */
    int Init(char* SipUaId, char* SipUaIp, int SipUaPort,
        char* SipSvrId, char* SipSvrIp, char* authPwd, int sipSvrPort);
    void Clean(void);


    //Main Worker Thread function, handles sip message
    static void ThreadProc(void* pParam);   //�̺߳���
    int StartProc();
    void StopProc();
    HANDLE m_threadHandle;  //�߳̾��
    bool m_bThreadRuning;   //�߳�����״̬

    //eXosip2 events' handle
    inline int ProceXsipEvt(eXosip_event_t* pSipEvt);

    //rfc3261 define six function : INVITE  ACK  CANCEL  BYE  INFO  OPTIONS
    /**
    *   ���ܣ�
    *        ע�ᵽsip������
    *   ����:
    *       nExpire��0��ע��������0����ʱʱ��
    *   ����ֵ:
    *       0���ɹ�����0��ʧ�ܡ�
    *   function:
    *       register sipUA to sipSvr
    *   parameter:
    *       nExpire:0,unregister; lower than 0, means overtime
    *   return:
    *       0, success, otherwise failure
    */
    int doRegister(int nExpire);

    int doInvite(char* dstDeviceid, char* sdp);
    int doAck();
    int doCancel();

    /**
    *   ���ܣ�
    *       ��������
    *   function:
    *       terminate a call
    */
    int doBye();
    int doInfo();
    int doOptions();

    //INVITE  ACK  CANCEL  BYE  INFO  OPTIONS
    int onInvite();
    int onMessage();

    int paraseMsg(char msg, eXosip_event_t* oSipEvent);

private:
    eXosip_t* m_poSipContent;
    int m_lsnPort;                  //sip��Ϣ�����˿�
    char m_pSipUAID[30];    //sipUa_id
    int m_callid;                   //callid
    int m_dialogid;         //dialogid
    char m_pSipUAIP[20];   //sipua_ip
    int m_ExpiresTime;              //ע�ᳬʱʱ��

    bool m_bInit;
};

#endif // !__SIPUA_H__

