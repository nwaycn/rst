package proto_define

import (
	"time"
)

// 用于传输前鉴权，暂保留
type RstAuth struct{}

// 用于Rst的数据交互
type Rst struct{}

type RstInviteRequest struct {
	Uuid         string
	PortA        string
	PortB        string
	UserData     string    //随路数据
	Ip           string    //来自于哪个IP，防止被恶意扔数据，使用了这个ip的对应端口的过来的恶意数据就由他了
	InviteTime   time.Time //收到的时间
	LastDataTime time.Time //最后一个包的收到的时间，当收到包后需要更新这里

}
type RstInviteResponse struct {
	Uuid      string
	RstPortA  string
	RstPortB  string
	OtherData string
}

func (r *Rst) Invite(req RstInviteRequest, rsp *RstInviteResponse) error {
	//通过查找本地的端口来告诉请求方，你的PortA送给我的RstPortA,
	//PortB送给我的RstPortB
	//同时开启rstport a,b的两个端口监听，祼流，不使用协议
	return nil
}

type RstByeRequest struct {
	Uuid string
}
type RstByeResponse struct {
	Code string
}

func (r *Rst) Bye(req RstByeRequest, rsp *RstByeResponse) error {
	//关闭这个会话，以及关闭对应的rst port a,b
	return nil
}

type RstHealthRequest struct {
	Uuid string
}
type RstHealthResponse struct {
	Code string //200为正常，非200全为不正常，如果回个503，那么切记要挂机

}

func (r *Rst) Health(req RstByeRequest, rsp *RstByeResponse) error {

	rsp.Code = "200"
	return nil
}
