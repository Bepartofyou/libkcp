//#include <unistd.h>
//#include <sys/time.h>
#include <cstring>
#include <cstdio>
#include "sess.h"

#define  MAX_LEN 10 * 1024

IUINT32 iclock();

int main() {
    //struct timeval time;
    //gettimeofday(&time, NULL);
    //srand((time.tv_sec * 1000) + (time.tv_usec / 1000));
	srand(iclock());

    UDPSession *sess = UDPSession::DialWithOptions("127.0.0.1", 9999, 2,2);
    sess->NoDelay(1, 20, 2, 1);
    //sess->WndSize(128, 128);
	sess->WndSize(1024, 1024);
    sess->SetMtu(1400);
    //sess->SetStreamMode(true);
	sess->SetStreamMode(false);
    sess->SetDSCP(46);

    assert(sess != nullptr);
    ssize_t nsent = 0;
    ssize_t nrecv = 0;
    //char *buf = (char *) malloc(128);
	char *buf_w = (char *)malloc(MAX_LEN);
	char *buf_r = (char *)malloc(MAX_LEN);
	for (size_t i = 0; i < MAX_LEN; i += 10)
	{
		sprintf(buf_w + i, "tick-%05d", i);
	}

    //for (int i = 0; i < 10; i++) {
	for (;;) {
        //sprintf(buf, "message:%d", i);
        auto sz = strlen(buf_w);
        sess->Write(buf_w, sz);
        sess->Update(iclock());
        memset(buf_r, 0, MAX_LEN);
        ssize_t n = 0;
        do {
            n = sess->Read(buf_r, MAX_LEN);
            //if (n > 0) { printf("%s\n", buf); }
            //usleep(33000);
			//isleep(33000/1000);
			isleep(3);
            sess->Update(iclock());
        } while(n==0);
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


