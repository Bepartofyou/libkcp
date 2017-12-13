//#include <unistd.h>
//#include <sys/time.h>
#include <cstring>
#include <cstdio>
#include "sess.h"

#define  MAX_LEN 60 * 1024

IUINT32 iclock();

static IUINT32 gTime = 0;

int main() {
    //struct timeval time;
    //gettimeofday(&time, NULL);
    //srand((time.tv_sec * 1000) + (time.tv_usec / 1000));
	srand(iclock());

    UDPSession *sess = UDPSession::DialWithOptions("127.0.0.1", 9999, 2,2);
    sess->NoDelay(1, 10, 2, 1);
	//sess->NoDelay(1, 20, 2, 1);
    //sess->WndSize(128, 128);
	sess->WndSize(8192, 8192);
    sess->SetMtu(1400);
    //sess->SetStreamMode(true);
	sess->SetStreamMode(false);
    sess->SetDSCP(46);

    assert(sess != nullptr);
    ssize_t nsent = 0;
    ssize_t nrecv = 0;
    //char *buf = (char *) malloc(128);
	char *buf_w = (char *)malloc(MAX_LEN);
	char *buf_r = (char *)malloc(MAX_LEN*100);
	for (size_t i = 0; i < MAX_LEN; i += 10)
	{
		sprintf(buf_w + i, "tick-%05d", i);
	}

    //for (int i = 0; i < 10; i++) {
	for (;;) {

		if (gTime==0){
			gTime = iclock();
		}else {
			if (iclock() - gTime > 1000) {
				ikcpcb* sKcp = sess->GetKcp();
				printf("[kcpinfo] rmt_wnd:%-4d,cwnd:%-4d,nsnd_buf:%-8d,nsnd_que:%-8d,nrcv_buf:%-8d,nrcv_que:%-8d,rx_rttval:%-2d,rx_srtt:%-2d,rx_rto:%-2d,rx_minrto:%-2d\n", 
					sKcp->rmt_wnd, sKcp->cwnd,sKcp->nsnd_buf,sKcp->nsnd_que,sKcp->nrcv_buf,sKcp->nrcv_que,
					sKcp->rx_rttval, sKcp->rx_srtt, sKcp->rx_rto, sKcp->rx_minrto);
				gTime = iclock();
			}
		}


        //sprintf(buf, "message:%d", i);
        auto sz = strlen(buf_w);
        sess->Write(buf_w, sz);
        sess->Update(iclock());
        memset(buf_r, 0, MAX_LEN);
        ssize_t n = 0;
        do {
            n = sess->Read(buf_r, MAX_LEN * 100);
            //if (n > 0) { printf("%d\n", strlen(buf_r)); }
            //usleep(33000);
			//isleep(33000/1000);
			//isleep(3);
            sess->Update(iclock());
        } while(n==0);
		isleep(10);
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
}


//void
//itimeofday(long *sec, long *usec) {
//    struct timeval time;
//    gettimeofday(&time, NULL);
//    if (sec) *sec = time.tv_sec;
//    if (usec) *usec = time.tv_usec;
//}
//
//IUINT64 iclock64(void) {
//    long s, u;
//    IUINT64 value;
//    itimeofday(&s, &u);
//    value = ((IUINT64) s) * 1000 + (u / 1000);
//    return value;
//}
//
//IUINT32 iclock() {
//    return (IUINT32) (iclock64() & 0xfffffffful);
//}


