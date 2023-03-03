/*
auth:上海宁卫信息技术有限公司
License: GPL
Description: this is a module of FreeSWITCH，and it send any udp stream to other udp server
*/
package main

import (
	"errors"
	"fmt"
	"io/fs"
	"math/rand"
	"net"
	"os"
	"path/filepath"
	"runtime"
	"strconv"
	"strings"
	"syscall"
	"time"
	"unicode"

	_ "github.com/zenwerk/go-wave"
)

var Cdr_file *os.File

// var Sessions map[string]Rst_session
var SessMgr_ SessMgr
var Save_dir string
var Cdr_dir string
var Save_rule string
var Ext_name string
var Use_caller bool
var Use_callee bool
var Use_uuid bool

// 起始协议包为 INV :caller:callee:uuid:caller_port:callee_port
// 结束协议包为 BYE :uuid
// 数据协议包为 DATA:uuid:FALGS:payload:len:data   //DATA:%s:W:%s:%d
func IsDir(path string) bool {
	s, err := os.Stat(path)
	if err != nil {
		return false
	}
	return s.IsDir()
}
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
func InsertHexFile(mydata []byte, mydata_len int, file *os.File) error {
	var mydatahex string
	if file == nil {
		fmt.Println("file not opened")
		return errors.New("file not opened")
	}
	if len(mydata) < mydata_len {
		fmt.Println("data real len less than ilen ")
		return errors.New("data real len less than ilen")
	}
	for i := 0; i < mydata_len; i++ {
		//mydatahex += " 0x"
		myassicc := int(mydata[i])
		mydatahex += fmt.Sprintf(" 0x%x", myassicc)
	}
	_, err := file.WriteString(mydatahex)
	return err
}
func GetValidString(s string) string {

	bytes := []byte(s)
	for i, ch := range bytes {

		switch {
		case ch > '~':
			bytes[i] = ' '
		case ch == '\r':
		case ch == '\n':
		case ch == '\t':
		case ch < ' ':
			bytes[i] = ' '
		}
	}
	return strings.Trim(string(bytes), " ")
}

// INV :4d977664-038b-41c9-809a-95efc5c72ba6:1000:1011
func ParseInv(data []byte, l int) (error, string, string, string) {
	var err error = nil
	var cmd string
	var uuid string
	var caller string
	var callee string
	var i int = 0
	var myorder int = 0
	for i = 0; i < l; i++ {
		if data[i] > 34 && data[i] < 123 && myorder < 5 {
			if data[i] != ':' {
				switch myorder {
				case 0:
					cmd += string(data[i])
					break
				case 1:
					uuid += string(data[i])
					break
				case 2:
					caller += string(data[i])
					break
				case 3:
					callee += string(data[i])
				}
			} else {
				myorder = myorder + 1
			}
		}
	}
	if myorder < 3 {
		err = errors.New("params less")
	}
	fmt.Println(cmd, uuid, caller, callee)
	return err, uuid, caller, callee
}

// DATA:uuid:FALGS:payload:len:data
func ParseData(data []byte, l int) (error, string, string, string, int, []byte) {
	var err error
	var cmd string
	var uuid string
	var flag string
	var payload string

	var strlen string
	var rdata []byte
	var ilen int = 0
	var i int = 0
	var myorder int = 0

	rdata = make([]byte, 1)
	for i = 0; i < l; i++ {
		if data[i] > 31 && data[i] < 127 && myorder < 5 {
			if data[i] != ':' {
				switch myorder {
				case 0:
					cmd += string(data[i])
					break
				case 1:
					uuid += string(data[i])
					break
				case 2:
					flag += string(data[i])
					break
				case 3:
					payload += string(data[i])
					break
				case 4:
					strlen += string(data[i])
				}
			} else {
				myorder = myorder + 1
			}
		} else if myorder > 4 {
			rdata = append(rdata, data[i])
		}
	}
	//fmt.Println(cmd, uuid, flag, payload, strlen, "  ", strlen)
	if len(strlen) > 0 {
		ilen, err = strconv.Atoi(strlen)
		if err != nil {
			fmt.Println(err)
		}
	}
	fmt.Printf("len of proto:%d , real data length:%d\n", ilen, len(rdata))
	return nil, uuid, flag, payload, ilen, rdata
}
func process_package(n int, addr *net.UDPAddr, buffer []byte) {
	//fmt.Println("addr:  ", addr, " n:", n)
	cmd := string(buffer[:4])
	var err error
	if cmd == INVITE {
		var session Rst_session
		//所有包内容为INVITE命令
		err1, uuid, caller, callee := ParseInv(buffer, n)
		if err1 != nil {
			fmt.Println(err1)
			return
		}
		session.Uuid = uuid
		session.Caller = caller
		session.Callee = callee
		session.Caller_file = nil
		session.Callee_file = nil
		if Use_caller {

			session.Caller_file_name = caller + "-"
		}
		if Use_callee {

			session.Caller_file_name += callee + "-"
		}
		if Use_uuid {

			session.Caller_file_name += uuid
		}
		session.Callee_file_name = Save_dir + session.Caller_file_name + "-R" + Ext_name
		session.Caller_file_name += "-W" + Ext_name
		session.Caller_file_name = Save_dir + session.Caller_file_name

		session.Caller_file_name = session.Caller_file_name

		session.Callee_file_name = session.Callee_file_name
		//openfile
		fmt.Println(session.Callee_file_name)
		fmt.Println(session.Caller_file_name)
		myfile := session.Caller_file_name
		session.Caller_file, err = os.Create(myfile) //os.OpenFile(myfile, os.O_RDWR|os.O_APPEND|os.O_CREATE, 0666)
		// os.OpenFile(session.Caller_file_name, os.O_CREATE|os.O_RDWR|os.O_APPEND, 0777)//os.Create(session.Caller_file_name)
		if err != nil {
			if errors.Is(err, os.ErrInvalid) {
				fmt.Println("this is an os.ErrInvalid error")
			}

			if errors.Is(err, fs.ErrInvalid) {
				fmt.Println("this is an fs.ErrInvalid error")
			}

			if errors.Is(err, syscall.EINVAL) {
				fmt.Println("this is a syscall.EINVAL error")
			}
			fmt.Println("open file:", session.Caller_file_name, "  error:", err.Error())
			//return
		}
		myfile = session.Callee_file_name
		//myfile = "/opt/nway/1000_1011_7ea6d1a2-b806-49ef-9888-0e7ac158917a_W.pcm"
		session.Callee_file, err = os.Create(myfile)
		//os.OpenFile(myfile, os.O_RDWR|os.O_APPEND|os.O_CREATE, 0666)
		// os.OpenFile(session.Callee_file_name, os.O_CREATE|os.O_RDWR|os.O_APPEND, 0777)
		if err != nil {
			fmt.Println("open file:", session.Callee_file_name, "  error:", err.Error())
			//return
		}
		if WriteHex {
			tmp := session.Callee_file_name + ".hex"
			session.Callee_file_hex, err = os.Create(tmp)
			if err != nil {
				fmt.Println("open hex file error:", err)
				return
			}
			tmp = session.Caller_file_name + ".hex"
			session.Caller_file_hex, err = os.Create(tmp)
			if err != nil {
				fmt.Println("open hex file error:", err)
				return
			}
		}

		session.Callin_time = time.Now()
		//Sessions[session.Uuid] = session
		SessMgr_.Set(session.Uuid, session)

	} else if cmd == DATA {
		//DATA:478525a8-8263-4550-b18c-d027d11c9865:R:00:160:xxxxx   ,先转为字符串后，再把长度后的那部分写入文件,
		//可以从第六个开始，前五个字符不需要再辨别
		err, uuid, flag, _, ilen, mydata := ParseData(buffer, n)
		if err == nil && ilen > 0 {
			session, ok := SessMgr_.Get(uuid) //Sessions[uuid]
			if !ok {
				fmt.Println("not found session for uuid:", uuid)
				return
			}
			if flag == "W" {
				if session.Caller_file != nil {
					_, err = session.Caller_file.Write(mydata)
					if err != nil {
						fmt.Println("write to file error:", err)
					}
					if WriteHex {
						InsertHexFile(mydata, ilen, session.Caller_file_hex)
					}

				}
			} else {
				if session.Callee_file != nil {
					_, err = session.Callee_file.Write(mydata)
					if err != nil {
						fmt.Println("write to file error:", err)
					}
					if WriteHex {
						InsertHexFile(mydata, ilen, session.Callee_file_hex)
					}
				}
			}
		}
		//p := string(buffer)
		// var uuid string
		// var flag string
		// var payload string
		// var mylen string
		// var mydata []byte
		// var mypos int = 0
		// for i := 5; i < len(p); i++ {
		// 	if p[i] == ':' {

		// 		mypos++
		// 		i++

		// 	} else {
		// 		if mypos > 3 {
		// 			if IsDigits(mylen) {
		// 				session, ok := SessMgr_.Get(uuid) //Sessions[uuid]
		// 				if !ok {
		// 					//fmt.Println("not found session for uuid:", uuid)
		// 					return
		// 				}
		// 				var l int
		// 				l, err = strconv.Atoi(mylen)
		// 				if err != nil {
		// 					fmt.Println("convert len failed:", err)
		// 					return
		// 				}
		// 				if l < 1 {
		// 					fmt.Println("逗你玩？")
		// 					return
		// 				}
		// 				mydata = make([]byte, l) //
		// 				//fmt.Println(flag)
		// 				copy(mydata, buffer[i:])
		// 				if flag == "W" {
		// 					if session.Caller_file != nil {
		// 						_, err = session.Caller_file.Write(mydata)
		// 						if err != nil {
		// 							fmt.Println("write to file error:", err)
		// 						}
		// 					}
		// 				} else {
		// 					if session.Callee_file != nil {
		// 						_, err = session.Callee_file.Write(mydata)
		// 						if err != nil {
		// 							fmt.Println("write to file error:", err)
		// 						}
		// 					}
		// 				}

		// 			} else {
		// 				fmt.Println("len in package is failed")
		// 			}

		// 		} else if mypos == 0 {
		// 			uuid += string(p[i])
		// 		} else if mypos == 1 {
		// 			flag += string(p[i])
		// 		} else if mypos == 2 {
		// 			payload += string(p[i])
		// 		} else if mypos == 3 {
		// 			//必须是数字
		// 			mylen += string(p[i])
		// 		}
		// 	}
		// }
	} else if cmd == BYE {
		p := string(buffer)
		s3 := strings.Split(p, ":")
		if len(s3) != 2 {
			fmt.Println("This BYE request not correct,rule number not 2")
			return
		}
		uuid := s3[1]

		SessMgr_.Del(uuid)

		//delete(Sessions, uuid)
	}
}

func main() {
	//conf_file := GetCurrentDirectory() + "udp_server.conf"
	//iniconf, err := config.NewConfig("ini", conf_file)
	/*	if err != nil {
			fmt.Println("load config file failed")
			return
		}
		PORT := iniconf.String("udp_port") */
	//os.Cre
	s, err := net.ResolveUDPAddr("udp4", "0.0.0.0:9527")
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

	Save_dir = "" // iniconf.String("save_dir")
	if len(Save_dir) < 1 {
		Save_dir = "/opt/nway/"
	}
	if IsDir(Save_dir) == false {
		os.MkdirAll(Save_dir, 0777)
	}
	Cdr_dir = "" // iniconf.String("cdr_dir")
	if len(Cdr_dir) < 1 {
		Cdr_dir = "/opt/nway/cdr/"
	}
	if IsDir(Cdr_dir) == false {
		os.MkdirAll(Cdr_dir, 0777)
	}
	cdr_filename := fmt.Sprintf("%s%d.log", Cdr_dir, time.Now().Unix())
	Cdr_file, err = os.Create(cdr_filename)
	if err != nil {
		fmt.Println("create cdr file handler failed:", err)
		return
	}
	Save_rule = "" //iniconf.String("save_rule")
	if len(Save_rule) < 1 {
		Save_rule = "caller_callee_uuid.pcm"
	}
	WriteHex = false
	//解析rule
	s1 := strings.Split(Save_rule, "_")
	if len(s1) < 2 {
		Use_callee = false
		Use_caller = false
		Use_uuid = true
	} else {
		fmt.Println(s1)
		for i := 0; i < len(s1); i++ {
			if s1[i] == "caller" {
				Use_caller = true
			} else if s1[i] == "callee" {
				Use_callee = true
			} else if strings.Contains(s1[i], "uuid") {
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
	//Sessions = make(map[string]Rst_session, 0)
	SessMgr_ = SessMgr{Sessions: make(map[string]Rst_session, 1000)}
	for {
		buffer := make([]byte, 1024)
		n, addr, err := connection.ReadFromUDP(buffer)
		//fmt.Print("-> ", string(buffer[0:n-1]))

		if err != nil {
			fmt.Println("error:  ", err)
			break
		} else {
			process_package(n, addr, buffer) //

		}

	}
	if Cdr_file != nil {
		Cdr_file.Close()
	}
}
