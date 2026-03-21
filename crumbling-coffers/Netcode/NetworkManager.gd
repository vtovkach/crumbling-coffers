extends Node

# Constants 
var TCP_SEGMENT_SIZE: int = 200

# Indicates the state of the 
var main_server_connect = false; 
var game_server_connect = false; 

# Orchestrator Server 
var server_tcp: StreamPeerTCP
var game_server_udp: PacketPeerUDP

# Main Server Info
var server_ip: String = "127.0.0.1"
var server_port: int = 10000

const FramerTCP = preload("res://Netcode/tcp_framer.gd")
var tcp_framer: FramerTCP

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	pass 

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass

func connect_server_tcp() -> bool:
	# Disconnect existing connection
	if server_tcp and server_tcp.get_status() != StreamPeerTCP.STATUS_NONE:
		server_tcp.disconnect_from_host()
		main_server_connect = false
		tcp_framer = null  

	server_tcp = StreamPeerTCP.new()

	var err = server_tcp.connect_to_host(server_ip, server_port)
	if err != OK:
		push_error("Failed to start TCP connection: %s" % err)
		return false

	var elapsed := 0.0
	var timeout := 5.0

	while elapsed < timeout:
		server_tcp.poll()
		var status = server_tcp.get_status()

		if status == StreamPeerTCP.STATUS_CONNECTED:
			print("TCP Connection established with the server. IP:%s Port:%d." % [server_ip, server_port])
			main_server_connect = true
			# Create framer here
			tcp_framer = FramerTCP.new(server_tcp, TCP_SEGMENT_SIZE)
			return true

		elif status == StreamPeerTCP.STATUS_ERROR:
			main_server_connect = false
			tcp_framer = null
			return false

		await get_tree().process_frame
		elapsed += get_process_delta_time()

	# timeout
	main_server_connect = false
	tcp_framer = null
	return false

func disconnect_server_tcp() -> void:
	if server_tcp:
		server_tcp.disconnect_from_host()

	main_server_connect = false
	
	server_tcp = null
	tcp_framer = null
	
func send_server_tcp(packet: PackedByteArray) -> bool:
	if not tcp_framer:
		push_error("send_server_tcp(): TCP framer is null")
		return false

	if not is_server_tcp_connected():
		push_error("send_server_tcp(): TCP connection is not established")
		main_server_connect = false
		return false

	return tcp_framer.send_server_tcp(packet)
	
func has_server_tcp_frame() -> bool:
	if not tcp_framer:
		return false

	return tcp_framer.has_frame()
	
func get_server_tcp_frame() -> PackedByteArray:
	if not tcp_framer:
		push_error("get_server_tcp_frame(): TCP framer is null")
		return PackedByteArray()

	return tcp_framer.get_frame()

func is_server_tcp_connected() -> bool:
	if not server_tcp:
		return false

	server_tcp.poll()
	return server_tcp.get_status() == StreamPeerTCP.STATUS_CONNECTED

# =================== UDP API =====================
func init_udp(ip: String, port: int) -> bool:
	if game_server_udp:
		game_server_udp.close()

	game_server_udp = PacketPeerUDP.new()

	var err: int = game_server_udp.bind(0)
	if err != OK:
		push_error("init_udp(): failed to bind UDP socket: %s" % err)
		return false

	game_server_udp.set_dest_address(ip, port)
	return true

func udp_send(packet: PackedByteArray) -> bool:
	if not game_server_udp:
		push_error("udp_send(): UDP instance is null")
		return false

	if packet.is_empty():
		push_error("udp_send(): packet is empty")
		return false

	var err: int = game_server_udp.put_packet(packet)
	if err != OK:
		push_error("udp_send(): failed to send packet: %s" % err)
		return false

	return true

func udp_receive(expected_size: int) -> PackedByteArray:
	if not game_server_udp:
		push_error("udp_receive(): UDP instance is null")
		return PackedByteArray()

	if expected_size <= 0:
		push_error("udp_receive(): expected_size must be greater than 0")
		return PackedByteArray()

	if game_server_udp.get_available_packet_count() <= 0:
		return PackedByteArray()

	var packet: PackedByteArray = game_server_udp.get_packet()
	var err: int = game_server_udp.get_packet_error()

	if err != OK:
		push_error("udp_receive(): failed to receive packet: %s" % err)
		return PackedByteArray()

	if packet.size() != expected_size:
		return PackedByteArray()

	return packet
