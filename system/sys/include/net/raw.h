#ifndef __F_ATHEOS_NET_RAW_H__
#define __F_ATHEOS_NET_RAW_H__

struct _RawEndPoint {
    RawEndPoint_s *re_psNext;
    RawPort_s *re_psPort;

    ipaddr_t re_anRemoteAddr;
    ipaddr_t re_anLocalAddr;

    bool re_bNonBlocking;
    bool re_bRoute;
    int re_nCreateHeader;
    NetQueue_s re_sPackets;
    int re_nProtocol;
    SelectRequest_s *re_psFirstReadSelReq;
    SelectRequest_s *re_psFirstWriteSelReq;
    SelectRequest_s *re_psFirstExceptSelReq;
};


struct _RawPort {
    RawPort_s *rp_psNext;
    RawEndPoint_s *rp_psFirstEndPoint;
    int rp_nProtocol;
};

int raw_open(Socket_s * psSocket);

int init_raw(void);

#endif 	/* __F_ATHEOS_NET_RAW_H__ */

