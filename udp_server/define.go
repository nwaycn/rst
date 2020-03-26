package main

import (
	"time"
)

type Rst_session struct {
	Caller      string
	Callee      string
	Uuid        string
	Caller_port uint16
	Callee_port uint16
	Callin_time time.Time
	Hangup_time time.Time
	//Caller_File *os.File
	//Callee_File *os.File
	//waveFile, err := os.Create(audioFileName)
}

const (
	INVITE = "INV"
	BYE    = "BYE"
	DATA   = "DATA"
)
