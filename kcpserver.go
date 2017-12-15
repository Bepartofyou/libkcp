package main

import (
	"fmt"

	"github.com/xtaci/kcp-go"
)

const port = ":9999"

func ListenTest() (*kcp.Listener, error) {
	return kcp.ListenWithOptions(port, nil, 0, 0)
}

func server() {
	l, err := ListenTest()
	if err != nil {
		panic(err)
	}
	l.SetDSCP(46)
	for {
		s, err := l.AcceptKCP()
		if err != nil {
			panic(err)
		}

		go handle_client(s)
	}
}
func handle_client(conn *kcp.UDPSession) {
	conn.SetWindowSize(4096, 4096)
	//	conn.SetNoDelay(1, 20, 2, 1)
	conn.SetNoDelay(1, 20, 2, 1)
	//	conn.SetMtu(512)
	conn.SetStreamMode(false)
	fmt.Println("new client", conn.RemoteAddr())
	buf := make([]byte, 65536)
	count := 0
	for {
		n, err := conn.Read(buf)
		if err != nil {
			panic(err)
		}
		count++
		//		fmt.Println("received:", string(buf[:n]))
		//fmt.Println("received: ", n)
		conn.Write(buf[:n])
	}
}

func main() {
	server()
}
