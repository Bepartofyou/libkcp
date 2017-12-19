#include "sess.h"
#include <iostream>
#include "util.h"
#include <cstring>

UDPSession *
UDPSession::Listen(const char *ip, uint16_t port) {
	UDPSession::netinit();

	struct sockaddr_in laddr;
	memset(&laddr, 0, sizeof(laddr));
	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(port);
	int ret = inet_pton(AF_INET, ip, &(laddr.sin_addr));

	if (ret == 1) { // do nothing
	}
	else if (ret == 0) { // try ipv6
		return UDPSession::listenIPv6(ip, port);
	}
	else if (ret == -1) {
		return nullptr;
	}

	int sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1) {
		return nullptr;
	}

	if (bind(sockfd, (struct sockaddr *) &laddr, sizeof(struct sockaddr)) < 0) {
		close(sockfd);
		UDPSession::netuninit();
		return nullptr;
	}

	return UDPSession::createSession(sockfd);
}

UDPSession *
UDPSession::ListenWithOptions(const char *ip, uint16_t port, size_t dataShards, size_t parityShards) {
	auto sess = UDPSession::Listen(ip, port);
	if (sess == nullptr) {
		return nullptr;
	}
	return sess;
};


UDPSession *
UDPSession::listenIPv6(const char *ip, uint16_t port) {
	UDPSession::netinit();

	struct sockaddr_in6 laddr;
	memset(&laddr, 0, sizeof(laddr));
	laddr.sin6_family = AF_INET6;
	laddr.sin6_port = htons(port);
	if (inet_pton(AF_INET6, ip, &(laddr.sin6_addr)) != 1) {
		return nullptr;
	}

	int sockfd = socket(PF_INET6, SOCK_DGRAM, 0);
	if (sockfd == -1) {
		return nullptr;
	}

	if (bind(sockfd, (struct sockaddr *) &laddr, sizeof(struct sockaddr)) < 0) {
		close(sockfd);
		UDPSession::netuninit();
		return nullptr;
	}

	return UDPSession::createSession(sockfd);
}

UDPSession *
UDPSession::Dial(const char *ip, uint16_t port) {
	UDPSession::netinit();

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    int ret = inet_pton(AF_INET, ip, &(saddr.sin_addr));

    if (ret == 1) { // do nothing
    } else if (ret == 0) { // try ipv6
        return UDPSession::dialIPv6(ip, port);
    } else if (ret == -1) {
        return nullptr;
    }

    int sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        return nullptr;
    }
    if (connect(sockfd, (struct sockaddr *) &saddr, sizeof(struct sockaddr)) < 0) {
        close(sockfd);
		UDPSession::netuninit();
        return nullptr;
    }

    return UDPSession::createSession(sockfd);
}

UDPSession *
UDPSession::DialWithOptions(const char *ip, uint16_t port, size_t dataShards, size_t parityShards) {
    auto sess = UDPSession::Dial(ip, port);
    if (sess == nullptr) {
        return nullptr;
    }

    return sess;
};


UDPSession *
UDPSession::dialIPv6(const char *ip, uint16_t port) {
	UDPSession::netinit();

    struct sockaddr_in6 saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin6_family = AF_INET6;
    saddr.sin6_port = htons(port);
    if (inet_pton(AF_INET6, ip, &(saddr.sin6_addr)) != 1) {
        return nullptr;
    }

    int sockfd = socket(PF_INET6, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        return nullptr;
    }
    if (connect(sockfd, (struct sockaddr *) &saddr, sizeof(struct sockaddr_in6)) < 0) {
        close(sockfd);
		UDPSession::netuninit();
        return nullptr;
    }

    return UDPSession::createSession(sockfd);
}

UDPSession *
UDPSession::createSession(int sockfd) {

#ifdef __unix
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        return nullptr;
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        return nullptr;
    }
#else
	unsigned long  ul = 1;
	if (ioctlsocket(sockfd, FIONBIO, &ul) < 0) {
		return nullptr;
	}	
#endif

    UDPSession *sess = new(UDPSession);
    sess->m_sockfd = sockfd;
    //sess->m_kcp = ikcp_create(IUINT32(rand()), sess);
	sess->m_kcp = ikcp_create(IUINT32(1234), sess);
    sess->m_kcp->output = sess->out_wrapper;
    return sess;
}


void
UDPSession::Update(uint32_t current) noexcept {
    for (;;) {
		ssize_t n;
		if (!m_bserver) {
#ifdef __unix
			n = recv(m_sockfd, m_buf, sizeof(m_buf), 0);
#else
			n = recv(m_sockfd, (char*)m_buf, sizeof(m_buf), 0);
#endif
		}
		else {
			struct sockaddr_in raddr;
			socklen_t len;
#ifdef __unix
			n = recvfrom(m_sockfd, m_buf, sizeof(m_buf), 0, (struct sockaddr *) &raddr, &len);
#else
			n = recvfrom(m_sockfd, (char*)m_buf, sizeof(m_buf), 0, (struct sockaddr *) &raddr, &len);
#endif
			if (n > 0)
				m_raddr = raddr;
		}
		
        if (n > 0) {         
                ikcp_input(m_kcp, (char *) (m_buf), n);
        } else {
            break;
        }
    }
    m_kcp->current = current;
    ikcp_flush(m_kcp);
}

void
UDPSession::Destroy(UDPSession *sess) {
    if (nullptr == sess) return;
	if (0 != sess->m_sockfd) {
		close(sess->m_sockfd); 		
		UDPSession::netuninit();
	}
    if (nullptr != sess->m_kcp) { ikcp_release(sess->m_kcp); }
    delete sess;
}

ssize_t
UDPSession::Read(char *buf, size_t sz) noexcept {
    if (m_streambufsiz > 0) {
        size_t n = m_streambufsiz;
        if (n > sz) {
            n = sz;
        }
        memcpy(buf, m_streambuf, n);

        m_streambufsiz -= n;
        if (m_streambufsiz != 0) {
            memmove(m_streambuf, m_streambuf + n, m_streambufsiz);
        }
        return n;
    }

#if 0
	int psz = 0;
	while (true)
	{
		int sizet = ikcp_peeksize(m_kcp);
		if (sizet > 0)
			psz += sizet;
		else{
			psz = sizet;
			break;
		}
	}
	printf("peeksize: %d\n", psz);
#else
    int psz = ikcp_peeksize(m_kcp);
	//printf("peeksize: %d\n", psz);
#endif

    if (psz <= 0) {
        return 0;
    }

    if (psz <= sz) {
        return (ssize_t) ikcp_recv(m_kcp, buf, int(sz));
    } else {
        ikcp_recv(m_kcp, (char *) m_streambuf, sizeof(m_streambuf));
        memcpy(buf, m_streambuf, sz);
        m_streambufsiz = psz - sz;
        memmove(m_streambuf, m_streambuf + sz, psz - sz);
        return sz;
    }
}

ssize_t
UDPSession::Write(const char *buf, size_t sz) noexcept {
	if (ikcp_waitsnd(m_kcp) > 800)
		return 0;

    int n = ikcp_send(m_kcp, const_cast<char *>(buf), int(sz));
    if (n == 0) {
        return sz;
    } else return n;
}

int
UDPSession::SetDSCP(int iptos) noexcept {
    iptos = (iptos << 2) & 0xFF;
#ifdef __unix
    return setsockopt(this->m_sockfd, IPPROTO_IP, IP_TOS, &iptos, sizeof(iptos));
#else
	return setsockopt(this->m_sockfd, IPPROTO_IP, IP_TOS, (const char*)&iptos, sizeof(iptos));
#endif
}

void
UDPSession::SetStreamMode(bool enable) noexcept {
    if (enable) {
        this->m_kcp->stream = 1;
    } else {
        this->m_kcp->stream = 0;
    }
}

int
UDPSession::out_wrapper(const char *buf, int len, struct IKCPCB *, void *user) {
    assert(user != nullptr);
    UDPSession *sess = static_cast<UDPSession *>(user);

    sess->output(buf, static_cast<size_t>(len));
    return 0;
}

ssize_t
UDPSession::output(const void *buffer, size_t length) {
	m_count += length;

	ssize_t n;
	if (!m_bserver) {
#ifdef __unix
		n = send(m_sockfd, buffer, length, 0);
#else
		n = send(m_sockfd, (const char *)buffer, length, 0);
#endif
	}
	else {
#ifdef __unix
		n = sendto(m_sockfd, buffer, length, 0, (struct sockaddr *) &m_raddr, sizeof(m_raddr));
#else
		n = sendto(m_sockfd, (const char *)buffer, length, 0, (struct sockaddr *) &m_raddr, sizeof(m_raddr));
#endif
	}

    return n;
}

void
UDPSession::netinit() {
#ifndef __unix
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

void
UDPSession::netuninit() {
#ifndef __unix
	WSACleanup();
#endif
}
