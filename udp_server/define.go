/*
auth:上海宁卫信息技术有限公司
License: GPL
Description: this is a module of FreeSWITCH，and it send any udp stream to other udp server
*/
package main

import (
	"fmt"
	"os"
	"sync"
	"time"
)

type Rst_session struct {
	Caller            string
	Callee            string
	Uuid              string
	Caller_port       uint16
	Callee_port       uint16
	Callin_time       time.Time
	Last_recv_package time.Time //最后收到的包的时间，如果长时间没有收到包，那么就要主动断开
	Hangup_time       time.Time
	Caller_file       *os.File
	Callee_file       *os.File
	Caller_file_name  string
	Callee_file_name  string
}

type SessMgr struct {
	sync.RWMutex
	Sessions map[string]Rst_session
}

func (s *SessMgr) Get(key string) (Rst_session, bool) {
	s.RLock()
	defer s.RUnlock()
	session, ok := s.Sessions[key]
	if (!ok){
		fmt.Println("not found session :%s",key)
	}
	
	return session, ok
}
func (s *SessMgr) Set(key string, val Rst_session) {
	s.Lock()
	defer s.Unlock()
	
	s.Sessions[key] = val
	fmt.Println("insert a session for:%s, caller:%s, callee:%s",key,val.Caller , val.Callee)
}
func (s *SessMgr) Del(key string) {
	s.Lock()
	defer s.Unlock()
	session, ok := s.Sessions[key]
	if ok {
		if session.Caller_file != nil {
			session.Caller_file.Close()
		}
		if session.Callee_file != nil {
			session.Callee_file.Close()
		}
	}
	tmp := fmt.Sprintf("caller:%s,callee:%s,R:%s,W:%s,callin time:%s,hangup time:%s\r\n", session.Caller, session.Callee,
		session.Caller_file_name, session.Callee_file_name, session.Callin_time.Format(TIMEFORMAT), time.Now().Format(TIMEFORMAT))
	if Cdr_file != nil {
		Cdr_file.WriteString(tmp)
	}
	delete(s.Sessions, key)
}

//理论上，不需要回应包，但收到包后回一个

//ACK :478525a8-8263-4550-b18c-d027d11c9865

//如果是数据则格式如下：

//DATA:UUID:FLAG:PAYLOAD:LENGTH:xxx

//如  DATA:478525a8-8263-4550-b18c-d027d11c9865:R:00:160:xxxxx

//uuid为FS的session_id

//FALG为FS在channel中的read/write,用R或W

//payload为编码，如00,pcmu,08 PCMA,18 G729 ,04 G723, 10 PCM

//每次收到包后，需要更新下时间，如果某条通道不论read write长时间没有包过来，则我们需要自行BYE

//如果是开始需要送udp包，则发：

//INV :UUID:CALLER:CALLEE

//如：

// INV :478525a8-8263-4550-b18c-d027d11c9865:18621575908:02131570530

//如果是挂机则发：

//BYE :UUID

//如

// BYE :478525a8-8263-4550-b18c-d027d11c9865

const (
	INVITE     = "INV "
	BYE        = "BYE "
	DATA       = "DATA"
	ACK        = "ACK "
	TIMEFORMAT = "2006-01-02 15:04:05"
)
