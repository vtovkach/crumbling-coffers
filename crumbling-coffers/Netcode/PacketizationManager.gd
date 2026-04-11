extends RefCounted

const PACKET_SIZE: int = 200

# Outbound request type identifiers (bytes [0:3] of client→server TCP packets)
const TYPE_SEARCH_GAME: int = 0
const TYPE_STOP_SEARCH: int = 1

# Inbound response type identifiers (bytes [0:3] of server→client TCP packets)
const TYPE_GAME_FOUND:     int = 2
const TYPE_GAME_NOT_FOUND: int = 3

# ========================= Inner Classes =========================

class TCP_Response:
	var response_type: int  # Wire value: 2 = GAME_FOUND, 3 = GAME_NOT_FOUND
	var game_id:    String  # 16-byte identifier encoded as 32-char uppercase hex
	var player_id:  String  # 16-byte identifier encoded as 32-char uppercase hex
	var server_ip:  String  # IPv4 dotted-decimal string, e.g. "129.146.77.151"
	var port:       int


# TODO: Define fields once UDP gameplay packets are specced out.
class UDP_Response:
	pass

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

## TODO: Build a UDPPacket (payload + destination port) for the given game state.
func form_udp_packet() -> Object:
	push_error("PacketizationManager: form_udp_packet() is not yet implemented (TODO)")
	return null

## TODO: Parse a raw PACKET_SIZE-byte UDP server response into a UDP_Response.
func interpret_udp_packet(_raw: PackedByteArray) -> UDP_Response:
	push_error("PacketizationManager: interpret_udp_packet() is not yet implemented (TODO)")
	return null

# ========================= Private Helpers =========================

func _encode_u32_le(value: int, packet: PackedByteArray, offset: int) -> void:
	packet[offset    ] =  value        & 0xFF
	packet[offset + 1] = (value >>  8) & 0xFF
	packet[offset + 2] = (value >> 16) & 0xFF
	packet[offset + 3] = (value >> 24) & 0xFF

func _decode_u32_le(bytes: PackedByteArray, offset: int) -> int:
	return bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)

func _decode_u16_le(bytes: PackedByteArray, offset: int) -> int:
	return bytes[offset] | (bytes[offset + 1] << 8)

## Reads 'length' bytes from 'offset' and returns them as an uppercase hex string.
func _bytes_to_hex(bytes: PackedByteArray, offset: int, length: int) -> String:
	var result := ""
	for i in range(length):
		result += "%02X" % bytes[offset + i]
	return result

## Converts a little-endian uint32 IP address to dotted IPv4 notation.
## Example: 0x81924D97 (LE bytes [0x97, 0x4D, 0x92, 0x81]) → "129.146.77.151"
func _u32_to_ipv4(ip: int) -> String:
	return "%d.%d.%d.%d" % [
		(ip >> 24) & 0xFF,
		(ip >> 16) & 0xFF,
		(ip >>  8) & 0xFF,
		 ip        & 0xFF,
	]
