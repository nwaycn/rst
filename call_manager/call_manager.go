package call_manager

type CallManager struct {
	Uuid        string
	RemotePortA string
	RemotePortB string
	LocalPortA  string
	LocalPortB  string
	State       int
	FromIp      string
}

var NwayCalls map[Uuid]CallManager
