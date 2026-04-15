extends Node

const PACKET_SIZE: int = 200

# Outbound request type identifiers (bytes [0:3] of client→server TCP packets)
const TYPE_SEARCH_GAME: int = 0
const TYPE_STOP_SEARCH: int = 1

# Inbound response type identifiers (bytes [0:3] of server→client TCP packets)
const TYPE_GAME_FOUND:     int = 2
const TYPE_GAME_NOT_FOUND: int = 3

# UDP control bitfield values (bytes [32:33] of every UDP packet)
const UDP_CTRL_REGULAR:     int = 0x0000  # client regular packet  — all bits clear
const UDP_CTRL_INIT:        int = 0x0003  # client INIT packet     — bits 0 and 1 set
const UDP_CTRL_SERVER_INIT: int = 0x0002  # server SERVER_INIT     — bit 1 only
const UDP_CTRL_SERVER_AUTH: int = 0x0008  # server SERVER_AUTH     — bit 3 only
const UDP_CTRL_ACK:         int = 0x0004  # ACK packet             — bit 2 only

# UDP header layout (offsets in bytes)
const UDP_HDR_GAME_ID_OFFSET:      int = 0   # 16 bytes
const UDP_HDR_PLAYER_ID_OFFSET:    int = 16  # 16 bytes
const UDP_HDR_CTRL_OFFSET:         int = 32  # 2 bytes (uint16)
const UDP_HDR_PAYLOAD_SIZE_OFFSET: int = 34  # 2 bytes (uint16)
const UDP_HDR_SEQ_NUM_OFFSET:      int = 36  # 4 bytes (uint32)
const UDP_HDR_SIZE:                int = 40

# UDP payload sizes
const UDP_INIT_PAYLOAD_SIZE: int = 0
const UDP_REG_PAYLOAD_SIZE:  int = 20

# UDP response enums — top-level so clients can reference them directly via
# PacketizationManager.UDPStatus.NORMAL / PacketizationManager.UDPPacketType.SERVER_INIT
enum UDPStatus     { NORMAL = 0, ERROR  = 1 }
enum UDPPacketType { SERVER_INIT = 0, SERVER_AUTH = 1 }

var seq_num: int = 0

# ========================= Inner Classes =========================

class TCP_Response:
	var response_type: int  # Wire value: 2 = GAME_FOUND, 3 = GAME_NOT_FOUND
	var game_id:    String  # 16-byte identifier encoded as 32-char uppercase hex
	var player_id:  String  # 16-byte identifier encoded as 32-char uppercase hex
	var server_ip:  String  # IPv4 dotted-decimal string, e.g. "129.146.77.151"
	var port:       int

class PlayerInitPos:
	var x: int
	var y: int

	func _init(ix: int, iy: int) -> void:
		x = ix
		y = iy

class PlayerInfo:
	var pos_x: int
	var pos_y: int
	var vel_x: int
	var vel_y: int
	var score: int

	func _init(px: int, py: int, vx: int, vy: int, sc: int) -> void:
		pos_x = px
		pos_y = py
		vel_x = vx
		vel_y = vy
		score = sc

class UDP_Response:
	var status:      int  # PacketizationManager.UDPStatus value
	var packet_type: int  # PacketizationManager.UDPPacketType value

	var num_players:     int
	var server_cur_tick: int

	# Populated for SERVER_INIT and SERVER_AUTH
	var start_tick: int
	var stop_tick:  int

	# Populated for SERVER_INIT only
	var player_init_positions: Dictionary  # hex String → PacketizationManager.PlayerInitPos

	# Populated for SERVER_AUTH only
	var players: Dictionary  # hex String → PacketizationManager.PlayerInfo

	func _init() -> void:
		status      = 0  # UDPStatus.NORMAL
		packet_type = 0  # UDPPacketType.SERVER_INIT
		num_players     = 0
		server_cur_tick = 0
		start_tick = 0
		stop_tick  = 0
		player_init_positions = {}
		players               = {}

# ========================= Public API =========================

## Builds a PACKET_SIZE-byte TCP packet ready to pass to NetworkManager.send_tcp().
##   request_type – 0 = SEARCH_GAME_REQUEST, 1 = STOP_SEARCH_REQUEST
##   map_id       – 1-byte map identifier (currently unused by the server; pass 0)
func form_tcp_packet(request_type: int, map_id: int) -> PackedByteArray:
	var packet := PackedByteArray()
	packet.resize(PACKET_SIZE)
	packet.fill(0)
	_encode_u32_le(request_type, packet, 0)
	packet[4] = map_id & 0xFF
	return packet

## Parses a raw PACKET_SIZE-byte TCP server response into a TCP_Response.
## Callers can inspect response_type first, then read the populated fields.
func interpret_tcp_packet(raw: PackedByteArray) -> TCP_Response:
	var response := TCP_Response.new()
	var type_id  := _decode_u32_le(raw, 0)

	response.response_type = type_id
	if type_id == TYPE_GAME_FOUND:
		response.game_id   = _bytes_to_hex(raw, 4,  16)
		response.player_id = _bytes_to_hex(raw, 20, 16)
		response.server_ip = _u32_to_ipv4(_decode_u32_le(raw, 36))
		response.port      = _decode_u16_le(raw, 40)

	return response

## Builds a PACKET_SIZE-byte UDP INIT packet and returns a UDPPacket ready for
## NetworkManager.send_udp(). Used to register with the game server on match start.
##   game_id   – 32-char uppercase hex string representing a 16-byte game identifier
##   player_id – 32-char uppercase hex string representing a 16-byte player identifier
##   port      – destination UDP port
func form_udp_init_packet(game_id: String, player_id: String, port: int) -> Object:
	var packet := PackedByteArray()
	packet.resize(PACKET_SIZE)
	packet.fill(0)
	_hex_to_bytes(game_id,   packet, UDP_HDR_GAME_ID_OFFSET)
	_hex_to_bytes(player_id, packet, UDP_HDR_PLAYER_ID_OFFSET)
	_encode_u16_le(UDP_CTRL_INIT,        packet, UDP_HDR_CTRL_OFFSET)
	_encode_u16_le(UDP_INIT_PAYLOAD_SIZE, packet, UDP_HDR_PAYLOAD_SIZE_OFFSET)
	_encode_u32_le(0,                    packet, UDP_HDR_SEQ_NUM_OFFSET)
	return NetworkManager.UDPPacket.new(packet, port)

## Builds a PACKET_SIZE-byte UDP regular packet and returns a UDPPacket ready for
## NetworkManager.send_udp(). Sent every game tick to report player state.
##   game_id   – 32-char uppercase hex string representing a 16-byte game identifier
##   player_id – 32-char uppercase hex string representing a 16-byte player identifier
##   port      – destination UDP port
##   position  – player position (x, y encoded as signed 32-bit integers)
##   velocity  – player velocity (x, y encoded as signed 32-bit integers)
##   score     – player score (unsigned 32-bit integer)
func form_udp_reg_packet(
	game_id:   String,
	player_id: String,
	port:      int,
	position:  Vector2,
	velocity:  Vector2,
	score:     int
) -> Object:
	var packet := PackedByteArray()
	packet.resize(PACKET_SIZE)
	packet.fill(0)
	_hex_to_bytes(game_id,   packet, UDP_HDR_GAME_ID_OFFSET)
	_hex_to_bytes(player_id, packet, UDP_HDR_PLAYER_ID_OFFSET)
	_encode_u16_le(UDP_CTRL_REGULAR,    packet, UDP_HDR_CTRL_OFFSET)
	_encode_u16_le(UDP_REG_PAYLOAD_SIZE, packet, UDP_HDR_PAYLOAD_SIZE_OFFSET)
	_encode_u32_le(0,                   packet, UDP_HDR_SEQ_NUM_OFFSET)
	_encode_i32_le(int(position.x), packet, UDP_HDR_SIZE)
	_encode_i32_le(int(position.y), packet, UDP_HDR_SIZE + 4)
	_encode_i32_le(int(velocity.x), packet, UDP_HDR_SIZE + 8)
	_encode_i32_le(int(velocity.y), packet, UDP_HDR_SIZE + 12)
	_encode_u32_le(score,           packet, UDP_HDR_SIZE + 16)
	return NetworkManager.UDPPacket.new(packet, port)

## Parses a raw PACKET_SIZE-byte UDP server packet into a UDP_Response.
## Check response.status first — if ERROR the remaining fields are undefined.
## Then check response.packet_type to know which fields are populated.
func interpret_udp_packet(raw: PackedByteArray) -> UDP_Response:
	var response := UDP_Response.new()

	if raw.size() != PACKET_SIZE:
		response.status = UDPStatus.ERROR
		return response

	var ctrl    := _decode_u16_le(raw, UDP_HDR_CTRL_OFFSET)
	var seq_num := _decode_u32_le(raw, UDP_HDR_SEQ_NUM_OFFSET)
	response.server_cur_tick = seq_num

	if ctrl & UDP_CTRL_SERVER_AUTH:
		response.packet_type = UDPPacketType.SERVER_AUTH
		response.start_tick  = _decode_u32_le(raw, UDP_HDR_SIZE)
		response.stop_tick   = _decode_u32_le(raw, UDP_HDR_SIZE + 4)
		var num := raw[UDP_HDR_SIZE + 8]
		response.num_players = num
		for i in range(num):
			var base      := UDP_HDR_SIZE + 9 + i * 36
			var pid_hex   := _bytes_to_hex(raw, base, 16)
			var info      := PlayerInfo.new(
				_decode_i32_le(raw, base + 16),
				_decode_i32_le(raw, base + 20),
				_decode_i32_le(raw, base + 24),
				_decode_i32_le(raw, base + 28),
				_decode_u32_le(raw, base + 32)
			)
			response.players[pid_hex] = info

	elif ctrl & UDP_CTRL_SERVER_INIT:
		response.packet_type  = UDPPacketType.SERVER_INIT
		response.start_tick   = _decode_u32_le(raw, UDP_HDR_SIZE)
		response.stop_tick    = _decode_u32_le(raw, UDP_HDR_SIZE + 4)
		var num               := raw[UDP_HDR_SIZE + 8]
		response.num_players  = num
		for i in range(num):
			var base    := UDP_HDR_SIZE + 9 + i * 24
			var pid_hex := _bytes_to_hex(raw, base, 16)
			var pos     := PlayerInitPos.new(
				_decode_i32_le(raw, base + 16),
				_decode_i32_le(raw, base + 20)
			)
			response.player_init_positions[pid_hex] = pos

	else:
		response.status = UDPStatus.ERROR

	return response

# ========================= Private Helpers =========================

func _encode_u16_le(value: int, packet: PackedByteArray, offset: int) -> void:
	packet[offset    ] =  value       & 0xFF
	packet[offset + 1] = (value >> 8) & 0xFF

func _encode_u32_le(value: int, packet: PackedByteArray, offset: int) -> void:
	packet[offset    ] =  value        & 0xFF
	packet[offset + 1] = (value >>  8) & 0xFF
	packet[offset + 2] = (value >> 16) & 0xFF
	packet[offset + 3] = (value >> 24) & 0xFF

func _encode_i32_le(value: int, packet: PackedByteArray, offset: int) -> void:
	var bits := value & 0xFFFFFFFF  # two's complement 32-bit
	packet[offset    ] =  bits        & 0xFF
	packet[offset + 1] = (bits >>  8) & 0xFF
	packet[offset + 2] = (bits >> 16) & 0xFF
	packet[offset + 3] = (bits >> 24) & 0xFF

func _decode_u16_le(bytes: PackedByteArray, offset: int) -> int:
	return bytes[offset] | (bytes[offset + 1] << 8)

func _decode_u32_le(bytes: PackedByteArray, offset: int) -> int:
	return bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)

func _decode_i32_le(bytes: PackedByteArray, offset: int) -> int:
	var u := _decode_u32_le(bytes, offset)
	return u if u < 0x80000000 else u - 0x100000000

## Reads 'length' bytes from 'offset' and returns them as an uppercase hex string.
func _bytes_to_hex(bytes: PackedByteArray, offset: int, length: int) -> String:
	var result := ""
	for i in range(length):
		result += "%02X" % bytes[offset + i]
	return result

## Writes a 32-char uppercase hex string as 16 raw bytes into buf at offset.
func _hex_to_bytes(hex: String, buf: PackedByteArray, offset: int) -> void:
	for i in range(16):
		buf[offset + i] = hex.substr(i * 2, 2).hex_to_int()

## Converts a little-endian uint32 IP address to dotted IPv4 notation.
## Example: 0x81924D97 (LE bytes [0x97, 0x4D, 0x92, 0x81]) → "129.146.77.151"
func _u32_to_ipv4(ip: int) -> String:
	return "%d.%d.%d.%d" % [
		(ip >> 24) & 0xFF,
		(ip >> 16) & 0xFF,
		(ip >>  8) & 0xFF,
		 ip        & 0xFF,
	]
