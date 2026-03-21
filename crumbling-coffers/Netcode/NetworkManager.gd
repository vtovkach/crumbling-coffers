extends Node

# Indicates the state of the 
var main_server_connect = false; 
var game_server_connect = false; 

# Orchestrator Server 
var server_tcp: StreamPeerTCP = StreamPeerTCP.new()
var server_ip: String = "127.0.0.1"
var server_port: int = 10000

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	pass # Replace with function body.

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass

func connect_server_tcp() -> bool:
	if server_tcp and server_tcp.get_status() != StreamPeerTCP.STATUS_NONE:
		server_tcp.disconnect_from_host()
		main_server_connect = false 
		
	server_tcp = StreamPeerTCP.new()
	
	var err = server_tcp.connect_to_host(server_ip, server_port)
	if err != OK:
		push_error("Failed to start TCP connection: %s" % err)
		return false 	
	
	var elapsed = 0.0 
	var timeout = 5.0
	
	while elapsed < timeout:
		server_tcp.poll()
		var status = server_tcp.get_status()
		
		if status == StreamPeerTCP.STATUS_CONNECTED:
			print("TCP Connection established with the server. IP:%s Port:%d." % [server_ip, server_port])
			main_server_connect = true
			return true 
		elif status == StreamPeerTCP.STATUS_ERROR:
			main_server_connect = false 
			return false
		
		await get_tree().process_frame 
		elapsed += get_process_delta_time()
		
	return false  

func disconnect_server_tcp() -> void:
	if server_tcp:
		server_tcp.disconnect_from_host()
	main_server_connect = false
	server_tcp = StreamPeerTCP.new()
	return 
	
func recv_server_tcp() -> PackedByteArray:
	if not server_tcp:
		push_error("recv_server_tcp(): TCP instance is null")
		return PackedByteArray()

	server_tcp.poll()

	if server_tcp.get_status() != StreamPeerTCP.STATUS_CONNECTED:
		push_error("recv_server_tcp(): TCP connection is not established")
		main_server_connect = false
		return PackedByteArray()

	var available: int = server_tcp.get_available_bytes()
	if available <= 0:
		return PackedByteArray()

	var result: Array = server_tcp.get_data(available)
	var err: int = result[0]
	var data: PackedByteArray = result[1]

	if err != OK:
		push_error("recv_server_tcp(): failed to receive data: %s" % err)
		return PackedByteArray()

	return data
	
func send_server_tcp(packet: PackedByteArray) -> bool:
	if not server_tcp:
		push_error("send_server_tcp(): TCP instance is null")
		return false

	server_tcp.poll()

	if server_tcp.get_status() != StreamPeerTCP.STATUS_CONNECTED:
		push_error("send_server_tcp(): TCP connection is not established")
		main_server_connect = false
		return false

	if packet.is_empty():
		push_error("send_server_tcp(): packet is empty")
		return false

	var err: int = server_tcp.put_data(packet)
	if err != OK:
		push_error("send_server_tcp(): failed to send data: %s" % err)
		return false

	return true
	
