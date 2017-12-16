#include <cstring>
#include <cstdio>
#include "sess.h"

static IUINT32 gStart = 0;
static IUINT32 gTime = 0;
static IUINT64 gCount = 0;
static IUINT64 gCount_last = 0;

static char *buf_w = NULL;
static char *buf_r = NULL;

#define  MAX_LEN 20 * 1024

static bool bstop = false;
#include <signal.h>
static void sig_handler(int signo) {
	printf("thread stop sginal!\n");
	bstop = true;
}

int main(int argc, char** argv) {

	if (argc != 2) {
		printf("Usage:         \n");
		printf("      ./kcp_test filename\n");

		exit(1);
	}
	FILE* fp_input = fopen(argv[1], "wb");

	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		printf("\ncan't catch SIGINT\n");
	}

	srand(iclock());

    UDPSession *sess = UDPSession::DialWithOptions("192.168.56.128", 9999, 0,0);
    sess->NoDelay(1, 10, 2, 1);
	sess->WndSize(4096, 4096);
    sess->SetMtu(1400);
	sess->SetStreamMode(false);
    sess->SetDSCP(46);
	sess->SetHost(false);

    assert(sess != nullptr);
	buf_w = (char *)malloc(MAX_LEN);
	buf_r = (char *)malloc(MAX_LEN);

	int retval = 1;
	int readsize = 0;
	ikcpcb* sKcp;

	for (;;) {
		if (bstop)
			break;

		if (gTime==0){
			gTime = iclock();
			gStart = gTime;
		}else {
			if (iclock() - gTime > 1000) {

				sKcp = sess->GetKcp();
				printf("[kcpdata]  %llu kBps [realdata] %llu kBps [kcptotal] %llu kBps [realtotal] %llu kBps\n",
						(gCount - gCount_last) / 1000, (sess->m_count - sess->m_count_l) / 1000,
						gCount/1000/((iclock()-gStart)/1000),sess->m_count/1000/((iclock()-gStart)/1000),sess->m_count/1000);
				gCount_last = gCount;
				sess->m_count_l = sess->m_count;
				printf("[kcpinfo] rmt_wnd:%-4d,cwnd:%-4d,nsnd_buf:%-8d,nsnd_que:%-8d,nrcv_buf:%-8d,nrcv_que:%-8d,rx_rttval:%-2d,rx_srtt:%-2d,rx_rto:%-2d,rx_minrto:%-2d\n", 
					sKcp->rmt_wnd, sKcp->cwnd,sKcp->nsnd_buf,sKcp->nsnd_que,sKcp->nrcv_buf,sKcp->nrcv_que,
					sKcp->rx_rttval, sKcp->rx_srtt, sKcp->rx_rto, sKcp->rx_minrto);

				gTime = iclock();
			}
		}

		if (retval) {
			memset(buf_w, 0, MAX_LEN);
			readsize = fread(buf_w, 1, MAX_LEN, fp_input);
		}	
        retval = sess->Write(buf_w, readsize);
        sess->Update(iclock());
		gCount += retval;

        memset(buf_r, 0, MAX_LEN);
        ssize_t n = 0;
        do {
            n = sess->Read(buf_r, MAX_LEN);
            sess->Update(iclock());
        } while(n!=0);


		if (readsize <= 0 && sKcp && sKcp->nsnd_buf <= 0 && sKcp->nsnd_que <= 0)  //文件读完,并且kcp发送缓存没有数据的时候,代表已经全部发送成功
			break;
    }

    UDPSession::Destroy(sess);

	if (buf_w){
		free(buf_w);
		buf_w = NULL;
	}
	if (buf_r) {
		free(buf_r);
		buf_r = NULL;
	}

	if (fp_input)
		fclose(fp_input);

	return 0;
}
