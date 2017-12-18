#ifndef KCP_SESS_H
#define KCP_SESS_H

#include "ikcp.h"
#include "fec.h"
#include "util.h"
#include <list>

struct sendpkt {
	int size;
	char* data;
};

class CSockSend
{
public:
	CSockSend(int sock);
	~CSockSend();

public:
	static void* sendthread(void* arg);
	void Start();
	void Stop();
	size_t InputData(char *buffer, size_t length);
private:
	int m_sockfd{ 0 };
	std::list<sendpkt *> m_list;
	bool m_bstop{ false };
	pthread_mutex_t m_mutex;
	pthread_t m_ptid;
};


class UDPSession {
private:
    int m_sockfd{0};
    ikcpcb *m_kcp{nullptr};
    byte m_buf[2048];
    byte m_streambuf[65535];
    size_t m_streambufsiz{0};

    FEC fec;
    uint32_t pkt_idx{0};
    std::vector<row_type> shards;
    size_t dataShards{0};
    size_t parityShards{0};
public:
	// bepartofyou
	bool m_bserver{false};
	// Set session host
	void SetHost(bool bserver) noexcept { m_bserver = bserver; }
	IUINT64 m_count{ 0 };
	IUINT64 m_count_l{ 0 };
	struct sockaddr_in m_raddr;
	ikcpcb * GetKcp() { return m_kcp; }

	bool m_bthead{ false };
	CSockSend* m_socksend;
	void SetThread(bool bthead) noexcept;

	// Listen listen to the local port and returns UDPSession.
	static UDPSession *Listen(const char *ip, uint16_t port);
	// ListenWithOptions listen to the local port "raddr" on the network "udp" with packet encryption
	static UDPSession *ListenWithOptions(const char *ip, uint16_t port, size_t dataShards, size_t parityShards);
	// bepartofyou

    UDPSession(const UDPSession &) = delete;

    UDPSession &operator=(const UDPSession &) = delete;

    // Dial connects to the remote server and returns UDPSession.
    static UDPSession *Dial(const char *ip, uint16_t port);

    // DialWithOptions connects to the remote address "raddr" on the network "udp" with packet encryption
    static UDPSession *DialWithOptions(const char *ip, uint16_t port, size_t dataShards, size_t parityShards);

    // Update will try reading/writing udp packet, pass current unix millisecond
    void Update(uint32_t current) noexcept;

    // Destroy release all resource related.
    static void Destroy(UDPSession *sess);

    // Read reads from kcp with buffer empty sz.
    ssize_t Read(char *buf, size_t sz) noexcept;

    // Write writes into kcp with buffer empty sz.
    ssize_t Write(const char *buf, size_t sz) noexcept;

    // Set DSCP value
    int SetDSCP(int dscp) noexcept;

    // SetStreamMode toggles the stream mode on/off
    void SetStreamMode(bool enable) noexcept;

    // Wrappers for kcp control
    inline int NoDelay(int nodelay, int interval, int resend, int nc) {
        return ikcp_nodelay(m_kcp, nodelay, interval, resend, nc);
    }

    inline int WndSize(int sndwnd, int rcvwnd) { return ikcp_wndsize(m_kcp, sndwnd, rcvwnd); }

    inline int SetMtu(int mtu) { return ikcp_setmtu(m_kcp, mtu); }

	//win32 socket
	static inline void netinit();
	static inline void netuninit();

private:
    UDPSession() = default;

    ~UDPSession() = default;

    // DialIPv6 is the ipv6 version of Dial.
    static UDPSession *dialIPv6(const char *ip, uint16_t port);

	// listenIPv6 is the ipv6 version of listen.
	static UDPSession *listenIPv6(const char *ip, uint16_t port);

    // out_wrapper
    static int out_wrapper(const char *buf, int len, struct IKCPCB *kcp, void *user);

    // output udp packet
    ssize_t output(const void *buffer, size_t length);

    static UDPSession *createSession(int sockfd);
};

inline uint32_t currentMs() {
	return iclock();
}


#endif //KCP_SESS_H
