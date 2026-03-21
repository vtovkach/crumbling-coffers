extends Object

var tcp_segment_size: int
var server_tcp: StreamPeerTCP

var buffer: PackedByteArray
var queue: Array[PackedByteArray]

func _init(tcp: StreamPeerTCP, segment_size: int) -> void:
	server_tcp = tcp
	tcp_segment_size = segment_size
	buffer = PackedByteArray()
	queue = []

func process() -> void:
	if not _is_connected():
		return

	var data := _read_from_tcp()
	if data.is_empty():
		return

	_append_to_buffer(data)
	_extract_frames()

func _is_connected() -> bool:
	if not server_tcp:
		return false

	server_tcp.poll()
	return server_tcp.get_status() == StreamPeerTCP.STATUS_CONNECTED

func _read_from_tcp() -> PackedByteArray:
	var available := server_tcp.get_available_bytes()
	if available <= 0:
		return PackedByteArray()

	var result := server_tcp.get_data(available)
	var err: int = result[0]
	var data: PackedByteArray = result[1]

	if err != OK:
		return PackedByteArray()

	return data

func _append_to_buffer(data: PackedByteArray) -> void:
	buffer.append_array(data)

func _extract_frames() -> void:
	while buffer.size() >= tcp_segment_size:
		var frame := buffer.slice(0, tcp_segment_size)
		queue.append(frame)

		buffer = buffer.slice(tcp_segment_size)

func has_frame() -> bool:
	return not queue.is_empty()

func get_frame() -> PackedByteArray:
	if queue.is_empty():
		return PackedByteArray()

	var frame := queue[0]
	queue.remove_at(0)
	return frame
	
func send_server_tcp(packet: PackedByteArray) -> bool:
	if not server_tcp:
		return false

	if packet.size() != tcp_segment_size:
		push_error("Invalid packet size: %d (expected %d)" % [packet.size(), tcp_segment_size])
		return false

	var err := server_tcp.put_data(packet)
	return err == OK
