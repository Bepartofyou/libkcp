//#include <unistd.h>
//#include <sys/time.h>
#include <cstring>
#include <cstdio>
#include "sess.h"

static IUINT32 gStart = 0;
static IUINT32 gTime = 0;
static IUINT64 gCount = 0;
static IUINT64 gCount_last = 0;

static char *buf_w = NULL;
static char *buf_r = NULL;

static bool bstop = false;

//#define ___unix

#ifdef ___unix
#include <pthread.h>
#include <signal.h>
pthread_mutex_t mutex;

static void sig_handler(int signo){
	printf("thread stop sginal!\n");
	bstop = true;
}

void *send_thread(void* data) {
	UDPSession *sess = (UDPSession *)data;
	while (!bstop) {
		pthread_mutex_lock(&mutex);
		auto sz = strlen(buf_w);
		int retval = sess->Write(buf_w, sz);
		sess->Update(iclock());
		gCount += retval;
		pthread_mutex_unlock(&mutex);
		
//		if(retval == 0)
//		    isleep(1);
	}
}
#endif //__unix

#define  MAX_LEN 20 * 1024

int main() {
    //struct timeval time;
    //gettimeofday(&time, NULL);
    //srand((time.tv_sec * 1000) + (time.tv_usec / 1000));
	srand(iclock());

    UDPSession *sess = UDPSession::DialWithOptions("192.168.56.128", 9999, 0,0);
    sess->NoDelay(1, 10, 2, 1);
	//sess->NoDelay(1, 20, 2, 1);
    //sess->WndSize(128, 128);
	sess->WndSize(1024, 1024);
    sess->SetMtu(1400);
    //sess->SetStreamMode(true);
	sess->SetStreamMode(false);
    sess->SetDSCP(46);
	sess->SetHost(false);


    assert(sess != nullptr);
    ssize_t nsent = 0;
    ssize_t nrecv = 0;
    //char *buf = (char *) malloc(128);
	buf_w = (char *)malloc(MAX_LEN);
	buf_r = (char *)malloc(MAX_LEN*100);
	for (size_t i = 0; i < MAX_LEN; i += 10)
	{
		sprintf(buf_w + i, "tick-%05d", i);
	}

#ifdef ___unix
	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		printf("\ncan't catch SIGINT\n");
	}

	pthread_mutex_init(&mutex, NULL);
	pthread_t ptid;
	pthread_create(&ptid, NULL, send_thread, (void*)sess);
#endif // __unix


    //for (int i = 0; i < 10; i++) {
	for (;;) {
		if(bstop)
			break;

		if (gTime==0){
			gTime = iclock();
			gStart = gTime;
		}else {
			if (iclock() - gTime > 1000) {
#ifdef ___unix
				pthread_mutex_lock(&mutex);
#endif
				ikcpcb* sKcp = sess->GetKcp();
				printf("[kcpdata]  %llu kBps [realdata] %llu kBps [kcptotal] %llu kBps [realtotal] %llu kBps\n",
						(gCount - gCount_last) / 1000, (sess->m_count - sess->m_count_l) / 1000,
						gCount/1000/((iclock()-gStart)/1000),sess->m_count/1000/((iclock()-gStart)/1000),sess->m_count/1000);
				gCount_last = gCount;
				sess->m_count_l = sess->m_count;
				printf("[kcpinfo] rmt_wnd:%-4d,cwnd:%-4d,nsnd_buf:%-8d,nsnd_que:%-8d,nrcv_buf:%-8d,nrcv_que:%-8d,rx_rttval:%-2d,rx_srtt:%-2d,rx_rto:%-2d,rx_minrto:%-2d\n", 
					sKcp->rmt_wnd, sKcp->cwnd,sKcp->nsnd_buf,sKcp->nsnd_que,sKcp->nrcv_buf,sKcp->nrcv_que,
					sKcp->rx_rttval, sKcp->rx_srtt, sKcp->rx_rto, sKcp->rx_minrto);
#ifdef ___unix
				pthread_mutex_unlock(&mutex);
#endif
				gTime = iclock();
			}
		}

#ifndef ___unix
        auto sz = strlen(buf_w);
        int retval = sess->Write(buf_w, sz);
        sess->Update(iclock());
		gCount += retval;
#endif
        memset(buf_r, 0, MAX_LEN);
        ssize_t n = 0;
        do {
#ifdef ___unix
			pthread_mutex_lock(&mutex);
#endif
            n = sess->Read(buf_r, MAX_LEN * 100);
            //if (n > 0) { printf("%d\n", strlen(buf_r)); }
			//isleep(3);
            sess->Update(iclock());
#ifdef ___unix
			pthread_mutex_unlock(&mutex);
#endif
        } while(n!=0);
		//isleep(1);
    }

#ifdef ___unix
	pthread_join(ptid, NULL);
	pthread_mutex_destroy(&mutex);
#endif // __unix

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
