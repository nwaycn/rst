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
	"unicode"

	"github.com/astaxie/beego/config"
	_ "github.com/zenwerk/go-wave"
)

var Sessions map[string]Rst_session

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
func IsDigits(str string) bool {
	for _, r := range str {
		if unicode.IsDigit(r) == false {
			return false
		}

	}
	return true
}
func process_package(n int, addr *net.UDPAddr, buffer []byte) {
	fmt.Println("addr:  ", addr, " n:", n)
	cmd := string(buffer[:4])
	var err error
	if cmd == INVITE {
		var session Rst_session
		//所有包内容为INVITE命令
		p := string(buffer)
		s3 := strings.Split(p, ":")
		if len(s3) != 4 {
			fmt.Println("This INVTIE request not correct,rule number not 4")
			return
		}
		session.Uuid = s3[1]
		session.Caller = s3[2]
		session.Callee = s3[3]

		if Use_caller {

			session.Caller_file_name = session.Caller + "_"
		}
		if Use_callee {

			session.Caller_file_name += session.Callee + "_"
		}
		if Use_uuid {

			session.Caller_file_name += session.Uuid
		}
		session.Callee_file_name = session.Caller_file_name + "_R" + Ext_name
		session.Caller_file_name += "_W" + Ext_name
		//openfile
		session.Caller_file, err = os.Create(session.Caller_file_name)
		if err != nil {
			fmt.Println("open file error:", err)
			return
		}
		session.Callee_file, err = os.Create(session.Callee_file_name)
		if err != nil {
			fmt.Println("open file error:", err)
			return
		}
		Sessions[session.Uuid] = session

	} else if cmd == DATA {
		//DATA:478525a8-8263-4550-b18c-d027d11c9865:R:00:160:xxxxx   ,先转为字符串后，再把长度后的那部分写入文件,
		//可以从第六个开始，前五个字符不需要再辨别
		p := string(buffer)
		var uuid string
		var flag string
		var payload string
		var mylen string
		var mydata []byte
		var mypos int = 0
		for i := 5; i < len(p); i++ {
			if p[i] == ':' {

				mypos++
				i++

			} else {
				if mypos > 3 {
					if IsDigits(mylen) {
						session, ok := Sessions[uuid]
						if !ok {
							fmt.Println("not found session for uuid:", uuid)
							return
						}
						var l int
						l, err = strconv.Atoi(mylen)
						if err != nil {
							fmt.Println("convert len failed:", err)
							return
						}
						if l < 1 {
							fmt.Println("逗你玩？")
							return
						}
						mydata = make([]byte, l) //
						copy(mydata, buffer[i:])
						if flag == "W" {
							if session.Caller_file != nil {
								_, err = session.Caller_file.Write(mydata)
								if err != nil {
									fmt.Println("write to file error:", err)
								}
							}
						} else {
							if session.Callee_file != nil {
								_, err = session.Callee_file.Write(mydata)
								if err != nil {
									fmt.Println("write to file error:", err)
								}
							}
						}

					} else {
						fmt.Println("len in package is failed")
					}

				} else if mypos == 0 {
					uuid += string(p[i])
				} else if mypos == 1 {
					flag += string(p[i])
				} else if mypos == 2 {
					payload += string(p[i])
				} else if mypos == 3 {
					//必须是数字
					mylen += string(p[i])
				}
			}
		}
	} else if cmd == BYE {
		p := string(buffer)
		s3 := strings.Split(p, ":")
		if len(s3) != 2 {
			fmt.Println("This BYE request not correct,rule number not 2")
			return
		}
		uuid := s3[1]
		//如果有未关闭的文件，需要在些处关闭
		session, ok := Sessions[uuid]
		if ok {
			if session.Caller_file != nil {
				session.Caller_file.Close()
			}
			if session.Callee_file != nil {
				session.Callee_file.Close()
			}
		}
		delete(Sessions, uuid)
	}
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

	rand.Seed(time.Now().Unix())
	Sessions = make(map[string]Rst_session, 0)

	for {
		buffer := make([]byte, 1024)
		n, addr, err := connection.ReadFromUDP(buffer)
		//fmt.Print("-> ", string(buffer[0:n-1]))

		if err != nil {
			fmt.Println("error:  ", err)
		} else {
			go process_package(n, addr, buffer) //

		}

	}
}
