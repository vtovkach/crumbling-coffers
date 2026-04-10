extends Node

class UDPPacket:
	var payload: PackedByteArray
	var port: int

	func _init(p: PackedByteArray, prt: int) -> void:
		payload = p
		port = prt

const FramerTCP = preload("res://Netcode/tcp_framer.gd")

const PACKET_SIZE: int = 200
const MAX_RECONNECT_ATTEMPTS: int = 3
const RECONNECT_DELAY_MS: int = 2000
const TCP_CONNECT_TIMEOUT_MS: int = 5000
const THREAD_TICK_MS: int = 1

var server_ip: String = "129.146.77.151"
var server_tcp_port: int = 10000

var _server_tcp: StreamPeerTCP
var _udp: PacketPeerUDP
var _tcp_framer: FramerTCP

# Incoming queues (populated by thread, consumed by game)
var _in_tcp: Array[PackedByteArray] = []
var _in_udp: Array[PackedByteArray] = []

# Outgoing queues (populated by game, consumed by thread)
var _out_tcp: Array[PackedByteArray] = []
var _out_udp: Array[UDPPacket] = []

var _mutex: Mutex
var _thread: Thread
var _running: bool = false

func _ready() -> void:
	startup()

func startup() -> void:
	_mutex = Mutex.new()
	_in_tcp.clear()
	_in_udp.clear()
	_out_tcp.clear()
	_out_udp.clear()
	_running = true
	_thread = Thread.new()
	_thread.start(_thread_main)

func _exit_tree() -> void:
	_running = false
	if _thread and _thread.is_started():
		_thread.wait_to_finish()
	if _udp:
		_udp.close()
	if _server_tcp:
		_server_tcp.disconnect_from_host()

# ========================= Thread =========================

func _thread_main() -> void:
	_udp = PacketPeerUDP.new()
	var udp_err := _udp.bind(0)
	if udp_err != OK:
		push_error("NetworkManager: Failed to bind UDP socket: %s" % udp_err)
		call_deferred("_show_disconnect_error")
		return

	if not _connect_with_retries():
		call_deferred("_show_disconnect_error")
		return

	while _running:
		_server_tcp.poll()
		if _server_tcp.get_status() != StreamPeerTCP.STATUS_CONNECTED:
			if not _connect_with_retries():
				call_deferred("_show_disconnect_error")
				break

		# Read incoming TCP frames into _in_tcp
		_tcp_framer.process()
		while _tcp_framer.has_frame():
			var frame := _tcp_framer.get_frame()
			# tcp_framer already guarantees PACKET_SIZE frames, but double-check
			if frame.size() != PACKET_SIZE:
				push_error("NetworkManager: Dropping malformed TCP frame of size %d" % frame.size())
				continue
			_mutex.lock()
			_in_tcp.append(frame)
			_mutex.unlock()

		# Read incoming UDP packets into _in_udp
		while _udp.get_available_packet_count() > 0:
			var packet := _udp.get_packet()
			if _udp.get_packet_error() != OK:
				continue
			if packet.size() != PACKET_SIZE:
				push_error("NetworkManager: Dropping UDP packet with wrong size: %d" % packet.size())
				continue
			_mutex.lock()
			_in_udp.append(packet)
			_mutex.unlock()

		# Drain outgoing TCP queue
		_mutex.lock()
		var tcp_out := _out_tcp.duplicate()
		_out_tcp.clear()
		_mutex.unlock()
		for pkt in tcp_out:
			_tcp_framer.send_server_tcp(pkt)

		# Drain outgoing UDP queue
		_mutex.lock()
		var udp_out := _out_udp.duplicate()
		_out_udp.clear()
		_mutex.unlock()
		for udp_pkt: UDPPacket in udp_out:
			_udp.set_dest_address(server_ip, udp_pkt.port)
			_udp.put_packet(udp_pkt.payload)

		OS.delay_msec(THREAD_TICK_MS)

func _connect_with_retries() -> bool:
	for attempt in range(MAX_RECONNECT_ATTEMPTS):
		print("NetworkManager: TCP connect attempt %d/%d..." % [attempt + 1, MAX_RECONNECT_ATTEMPTS])
		if _try_connect_tcp():
			print("NetworkManager: TCP connected.")
			return true
		if attempt < MAX_RECONNECT_ATTEMPTS - 1:
			OS.delay_msec(RECONNECT_DELAY_MS)
	push_error("NetworkManager: All reconnection attempts failed.")
	return false

func _try_connect_tcp() -> bool:
	if _server_tcp and _server_tcp.get_status() != StreamPeerTCP.STATUS_NONE:
		_server_tcp.disconnect_from_host()

	_server_tcp = StreamPeerTCP.new()
	var err := _server_tcp.connect_to_host(server_ip, server_tcp_port)
	if err != OK:
		return false

	var elapsed := 0
	while elapsed < TCP_CONNECT_TIMEOUT_MS:
		_server_tcp.poll()
		var status := _server_tcp.get_status()
		if status == StreamPeerTCP.STATUS_CONNECTED:
			_tcp_framer = FramerTCP.new(_server_tcp, PACKET_SIZE)
			return true
		elif status == StreamPeerTCP.STATUS_ERROR:
			return false
		OS.delay_msec(10)
		elapsed += 10

	return false

# ========================= Error UI (main thread) =========================

func _show_disconnect_error() -> void:
	var dialog := AcceptDialog.new()
	dialog.title = "Connection Failed"
	dialog.dialog_text = "Could not connect to the server after %d attempts.\nThe application will now close." % MAX_RECONNECT_ATTEMPTS
	dialog.get_ok_button().text = "Quit"
	dialog.confirmed.connect(func() -> void: get_tree().quit())
	dialog.canceled.connect(func() -> void: get_tree().quit())
	get_tree().root.add_child(dialog)
	dialog.popup_centered()

# ========================= Public API =========================

## Queues a 200-byte TCP packet to be sent to the server.
## Returns -1 if the packet is not exactly PACKET_SIZE bytes.
func send_tcp(packet: PackedByteArray) -> int:
	if packet.size() != PACKET_SIZE:
		push_error("send_tcp(): Packet size mismatch: %d (expected %d)" % [packet.size(), PACKET_SIZE])
		return -1
	_mutex.lock()
	_out_tcp.append(packet)
	_mutex.unlock()
	return 0

## Pops the next received TCP packet. Returns empty PackedByteArray if none available.
func receive_tcp() -> PackedByteArray:
	_mutex.lock()
	if _in_tcp.is_empty():
		_mutex.unlock()
		return PackedByteArray()
	var pkt := _in_tcp[0]
	_in_tcp.remove_at(0)
	_mutex.unlock()
	return pkt

## Queues a UDPPacket (payload + port) to be sent to server_ip:port.
## Returns -1 if the payload is not exactly PACKET_SIZE bytes.
func send_udp(udp_packet: UDPPacket) -> int:
	if udp_packet.payload.size() != PACKET_SIZE:
		push_error("send_udp(): Payload size mismatch: %d (expected %d)" % [udp_packet.payload.size(), PACKET_SIZE])
		return -1
	_mutex.lock()
	_out_udp.append(udp_packet)
	_mutex.unlock()
	return 0

## Pops the next received UDP packet. Returns empty PackedByteArray if none available.
func receive_udp() -> PackedByteArray:
	_mutex.lock()
	if _in_udp.is_empty():
		_mutex.unlock()
		return PackedByteArray()
	var pkt := _in_udp[0]
	_in_udp.remove_at(0)
	_mutex.unlock()
	return pkt
