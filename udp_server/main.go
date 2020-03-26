package main

import (
	"fmt"
	"math/rand"
	"net"

	"time"
)

func random(min, max int) int {
	return rand.Intn(max-min) + min
}

func main() {

	PORT := ":9527"

	s, err := net.ResolveUDPAddr("udp4", PORT)
	if err != nil {
		fmt.Println(err)
		return
	}

	connection, err := net.ListenUDP("udp4", s)
	if err != nil {
		fmt.Println(err)
		return
	}

	defer connection.Close()
	buffer := make([]byte, 1024)
	rand.Seed(time.Now().Unix())

	for {
		n, addr, err := connection.ReadFromUDP(buffer)
		//fmt.Print("-> ", string(buffer[0:n-1]))
		if err != nil {
			fmt.Println("error:  ", err)
		} else {
			fmt.Println("addr:  ", addr, " n:", n)
		}

	}
}
