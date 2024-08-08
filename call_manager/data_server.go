package call_manager

import "net"

//查找空闲的端口启用udp server用于接收裸数据
// 检查端口是否可用
func CheckUDPPort(port int) bool {
    addr := net.UDPAddr{
        Port: port,
        IP:   net.ParseIP("0.0.0.0"),
    }
    conn, err := net.ListenUDP("udp", &addr)
    if err != nil {
        return false
    }
    conn.Close()
    return true
}
// 检查TCP端口是否可用
func CheckTCPPort(port int) bool {
    addr := net.TCPAddr{
        Port: port,
        IP:   net.ParseIP("0.0.0.0"),
    }
    conn, err := net.ListenTCP("tcp", &addr)
    if err != nil {
        return false
    }
    conn.Close()
    return true
}
