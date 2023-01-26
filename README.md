# rst

A real time stream trans to other application or server for FreeSWITCH

## mod_rst

get media bug data send it to an udp server

## udp_server

receive udp packet and write into file

# 中文说明

实时将FreeSWITCH通话中的语音媒体流导出到其它程序或机器中
支持双路音频实时数据通过udp 传输给其它的udp server，从而进行识别或旁路录音等

## mod_rst
实时对media bug 数据转发的模块

## udp server
接收udp包并写进文件中

## 协议
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

## License:GPL

