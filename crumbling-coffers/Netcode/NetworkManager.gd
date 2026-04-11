extends Node

class UDPPacket:
	var payload: PackedByteArray
	var port: int

	func _init(p: PackedByteArray, prt: int) -> void:
		payload = p
		port = prt

const FramerTCP = preload("res://Netcode/tcp_framer.gd")

const PACKET_SIZE: int = 200
const TCP_CONNECT_TIMEOUT_MS: int = 5000
const THREAD_TICK_MS: int = 1
const RECONNECT_INTERVAL_MS: int = 400

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

var _mutex_in_tcp: Mutex
var _mutex_in_udp: Mutex
var _mutex_out_tcp: Mutex
var _mutex_out_udp: Mutex
var _thread: Thread
var _running: bool = false

var _disconnect_indicator: TextureRect

func _ready() -> void:
	_init_disconnect_indicator()
	startup()

func startup() -> void:
	_mutex_in_tcp = Mutex.new()
	_mutex_in_udp = Mutex.new()
	_mutex_out_tcp = Mutex.new()
	_mutex_out_udp = Mutex.new()
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
		return

	call_deferred("_show_disconnect_indicator")
	if not _connect_until_running():
		return
	call_deferred("_hide_disconnect_indicator")

	while _running:
		_server_tcp.poll()
		if _server_tcp.get_status() != StreamPeerTCP.STATUS_CONNECTED:
			call_deferred("_show_disconnect_indicator")
			if not _connect_until_running():
				return
			call_deferred("_hide_disconnect_indicator")

		# Read incoming TCP frames into _in_tcp
		_tcp_framer.process()
		while _tcp_framer.has_frame():
			var frame := _tcp_framer.get_frame()
			# tcp_framer already guarantees PACKET_SIZE frames, but double-check
			if frame.size() != PACKET_SIZE:
				push_error("NetworkManager: Dropping malformed TCP frame of size %d" % frame.size())
				continue
			_mutex_in_tcp.lock()
			_in_tcp.append(frame)
			_mutex_in_tcp.unlock()

		# Read incoming UDP packets into _in_udp
		while _udp.get_available_packet_count() > 0:
			var packet := _udp.get_packet()
			if _udp.get_packet_error() != OK:
				continue
			if packet.size() != PACKET_SIZE:
				push_error("NetworkManager: Dropping UDP packet with wrong size: %d" % packet.size())
				continue
			_mutex_in_udp.lock()
			_in_udp.append(packet)
			_mutex_in_udp.unlock()

		# Drain outgoing TCP queue
		_mutex_out_tcp.lock()
		var tcp_out := _out_tcp.duplicate()
		_out_tcp.clear()
		_mutex_out_tcp.unlock()
		for pkt in tcp_out:
			_tcp_framer.send_server_tcp(pkt)

		# Drain outgoing UDP queue
		_mutex_out_udp.lock()
		var udp_out := _out_udp.duplicate()
		_out_udp.clear()
		_mutex_out_udp.unlock()
		for udp_pkt: UDPPacket in udp_out:
			_udp.set_dest_address(server_ip, udp_pkt.port)
			_udp.put_packet(udp_pkt.payload)

		OS.delay_msec(THREAD_TICK_MS)

# Retries TCP connection every RECONNECT_INTERVAL_MS until connected or shutdown.
# Returns true on success, false if _running was set to false.
func _connect_until_running() -> bool:
	while _running:
		print("NetworkManager: Attempting TCP connection to %s:%d..." % [server_ip, server_tcp_port])
		if _try_connect_tcp():
			print("NetworkManager: TCP connection established.")
			return true
		OS.delay_msec(RECONNECT_INTERVAL_MS)
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

# ========================= Disconnect Indicator =========================

func _init_disconnect_indicator() -> void:
	var canvas := CanvasLayer.new()
	canvas.layer = 100
	add_child(canvas)

	var root := Control.new()
	root.set_anchors_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	canvas.add_child(root)

	_disconnect_indicator = TextureRect.new()
	_disconnect_indicator.texture = load("res://Assets/Icons/no-inet-icon.png")
	_disconnect_indicator.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	_disconnect_indicator.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	_disconnect_indicator.anchor_left = 1.0
	_disconnect_indicator.anchor_right = 1.0
	_disconnect_indicator.anchor_top = 0.0
	_disconnect_indicator.anchor_bottom = 0.0
	_disconnect_indicator.offset_left = -36.0
	_disconnect_indicator.offset_right = -4.0
	_disconnect_indicator.offset_top = 4.0
	_disconnect_indicator.offset_bottom = 36.0
	_disconnect_indicator.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_disconnect_indicator.visible = false
	root.add_child(_disconnect_indicator)

func _show_disconnect_indicator() -> void:
	_disconnect_indicator.visible = true

func _hide_disconnect_indicator() -> void:
	_disconnect_indicator.visible = false

# ========================= Public API =========================

## Queues a 200-byte TCP packet to be sent to the server.
## Returns -1 if the packet is not exactly PACKET_SIZE bytes.
func send_tcp(packet: PackedByteArray) -> int:
	if packet.size() != PACKET_SIZE:
		push_error("send_tcp(): Packet size mismatch: %d (expected %d)" % [packet.size(), PACKET_SIZE])
		return -1
	_mutex_out_tcp.lock()
	_out_tcp.append(packet)
	_mutex_out_tcp.unlock()
	return 0

## Pops the next received TCP packet. Returns empty PackedByteArray if none available.
func receive_tcp() -> PackedByteArray:
	_mutex_in_tcp.lock()
	if _in_tcp.is_empty():
		_mutex_in_tcp.unlock()
		return PackedByteArray()
	var pkt := _in_tcp[0]
	_in_tcp.remove_at(0)
	_mutex_in_tcp.unlock()
	return pkt

## Queues a UDPPacket (payload + port) to be sent to server_ip:port.
## Returns -1 if the payload is not exactly PACKET_SIZE bytes.
func send_udp(udp_packet: UDPPacket) -> int:
	if udp_packet.payload.size() != PACKET_SIZE:
		push_error("send_udp(): Payload size mismatch: %d (expected %d)" % [udp_packet.payload.size(), PACKET_SIZE])
		return -1
	_mutex_out_udp.lock()
	_out_udp.append(udp_packet)
	_mutex_out_udp.unlock()
	return 0

## Pops the next received UDP packet. Returns empty PackedByteArray if none available.
func receive_udp() -> PackedByteArray:
	_mutex_in_udp.lock()
	if _in_udp.is_empty():
		_mutex_in_udp.unlock()
		return PackedByteArray()
	var pkt := _in_udp[0]
	_in_udp.remove_at(0)
	_mutex_in_udp.unlock()
	return pkt
