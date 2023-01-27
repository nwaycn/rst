package main

import (
	"fmt"
	"math/rand"
	"net"
	"os"
	"path/filepath"
	"runtime"
	"strconv"
	"strings"
	"time"

	"github.com/astaxie/beego/config"
	_ "github.com/zenwerk/go-wave"
)

var Sessions map[string]Rst_session
var Files map[uint16]*os.File
var Save_dir string
var Save_rule string
var Ext_name string
var Use_caller bool
var Use_callee bool
var Use_uuid bool

//起始协议包为 INV :caller:callee:uuid:caller_port:callee_port
//结束协议包为 BYE :uuid
//数据协议包为 DATA:port:payload:data
func GetCurrentDirectory() string {
	dir, err := filepath.Abs(filepath.Dir(os.Args[0]))
	if err != nil {
		println(err)
	}
	if runtime.GOOS == "linux" {
		dir += "/"
	} else {
		dir += "\\"
	}
	return dir

}
func ByteString(p []byte) string {
	for i := 0; i < len(p); i++ {
		if p[i] == 0 {
			return string(p[0:i])
		}
	}
	return string(p)
}

func StringByte(s string) []byte {
	var b []byte
	for _, v := range s {
		b = append(b, byte(v))
	}
	return b
}
func random(min, max int) int {
	return rand.Intn(max-min) + min
}

func main() {
	conf_file := GetCurrentDirectory() + "udp_server.conf"
	iniconf, err := config.NewConfig("ini", conf_file)
	if err != nil {
		fmt.Println("load config file failed")
		return
	}
	PORT := iniconf.String("udp_port")
	//os.Cre
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

	Save_dir = iniconf.String("save_dir")
	if len(Save_dir) < 1 {
		Save_dir = "/opt/nway/"
	}
	Save_rule = iniconf.String("save_rule")
	if len(Save_rule) < 1 {
		Save_rule = "caller_callee_uuid.wav"
	}

	//解析rule
	s1 := strings.Split(Save_rule, "_")
	if len(s1) < 2 {
		Use_callee = false
		Use_caller = false
		Use_uuid = true
	} else {
		for i := 0; i <= len(s1); i++ {
			if s1[i] == "caller" {
				Use_caller = true
			} else if s1[i] == "callee" {
				Use_callee = true
			} else if s1[i] == "uuid" {
				Use_uuid = true
			}
		}
		if Use_callee == false && Use_caller == false && Use_uuid == false {
			Use_uuid = true
		}
	}
	s2 := strings.Split(Save_rule, ".")
	if len(s2) < 1 {
		Ext_name = ".wav"
	} else {
		Ext_name = "." + s2[len(s2)-1]
	}
	//end 解析rule
	buffer := make([]byte, 1024)
	rand.Seed(time.Now().Unix())
	Sessions = make(map[string]Rst_session, 0)
	Files = make(map[uint16]*os.File, 0)
	var itmp int
	//var err error
	for {
		n, addr, err := connection.ReadFromUDP(buffer)
		//fmt.Print("-> ", string(buffer[0:n-1]))

		if err != nil {
			fmt.Println("error:  ", err)
		} else {
			fmt.Println("addr:  ", addr, " n:", n)
			cmd := string(buffer[:10])

			if strings.Contains(cmd, INVITE) {
				//new call
				params := strings.Split(cmd, ":")
				if len(params) < 5 {
					fmt.Println("this call length not enough!")
				} else {
					var session Rst_session
					session.Caller = params[1]
					session.Callee = params[2]
					session.Uuid = params[3]
					tmp := params[4]
					if len(tmp) > 0 {
						itmp, err = strconv.Atoi(tmp)
						session.Caller_port = uint16(itmp)
						if err != nil {
							fmt.Println("port convert faile:", tmp)
						} else {
							if _, ok := Files[session.Caller_port]; ok {

							} else {
								fmt.Sprintf(tmp, "./%s_%s.wav", session.Caller, session.Uuid)
								Files[session.Caller_port], err = os.Create(tmp)
								if err != nil {
									fmt.Println("created file failed")
									Files[session.Caller_port].Close()
								}
							}

						}
					}
					tmp = params[5]
					if len(tmp) > 0 {

						itmp, err = strconv.Atoi(tmp)
						session.Callee_port = uint16(itmp)
						if err != nil {
							fmt.Println("port convert faile:", tmp)
						} else {
							if _, ok := Files[session.Caller_port]; ok {

							} else {
								fmt.Sprintf(tmp, "./%s_%s.wav", session.Caller, session.Uuid)
								Files[session.Caller_port], err = os.Create(tmp)
								if err != nil {
									fmt.Println("created file failed")
									Files[session.Caller_port].Close()
								}
							}

						}
					}
					Sessions[session.Uuid] = session

				}

			} else if strings.Contains(cmd, BYE) {
				params := strings.Split(cmd, ":")
				if _s, ok := Sessions[params[1]]; ok {
					//find session
					if _v1, ok1 := Files[_s.Caller_port]; ok1 {
						_v1.Close()
						delete(Files, _s.Caller_port)
					}
					if _v2, ok2 := Files[_s.Callee_port]; ok2 {
						_v2.Close()
						delete(Files, _s.Callee_port)
					}
					delete(Sessions, params[1])
				}
				//destory a call
			} else if strings.Contains(cmd, DATA) {
				//write data into file
				//get port and payload
				cmd_data := string(buffer[:20])
				params := strings.Split(cmd_data, ":")
				port_s := params[1]
				//payload_s := params[2]

				var port uint16
				if len(port_s) > 0 {
					itmp, err = strconv.Atoi(port_s)
					port = uint16(itmp)
					if err == nil {
						if _v, ok := Files[port]; ok {
							//write to file
							index := strings.LastIndex(cmd_data, ":")
							data := buffer[index:]
							_v.Write(data)
						}
					}

				}

			}

		}

	}
}
