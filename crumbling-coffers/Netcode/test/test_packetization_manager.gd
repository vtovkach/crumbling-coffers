extends GutTest

const PacketizationManager = preload("res://Netcode/PacketizationManager.gd")

var pm: PacketizationManager

func before_each() -> void:
	pm = PacketizationManager.new()

# ========================= form_tcp_packet =========================

func test_packet_is_correct_size() -> void:
	var packet := pm.form_tcp_packet(PacketizationManager.TYPE_SEARCH_GAME, 0)
	assert_eq(packet.size(), PacketizationManager.PACKET_SIZE)

func test_search_game_request_type_encoded() -> void:
	var packet := pm.form_tcp_packet(PacketizationManager.TYPE_SEARCH_GAME, 0)
	var type_id := packet[0] | (packet[1] << 8) | (packet[2] << 16) | (packet[3] << 24)
	assert_eq(type_id, PacketizationManager.TYPE_SEARCH_GAME)

func test_stop_search_request_type_encoded() -> void:
	var packet := pm.form_tcp_packet(PacketizationManager.TYPE_STOP_SEARCH, 0)
	var type_id := packet[0] | (packet[1] << 8) | (packet[2] << 16) | (packet[3] << 24)
	assert_eq(type_id, PacketizationManager.TYPE_STOP_SEARCH)

func test_map_id_written_at_byte_4() -> void:
	var packet := pm.form_tcp_packet(PacketizationManager.TYPE_SEARCH_GAME, 7)
	assert_eq(packet[4], 7)

func test_map_id_zero_written_at_byte_4() -> void:
	var packet := pm.form_tcp_packet(PacketizationManager.TYPE_SEARCH_GAME, 0)
	assert_eq(packet[4], 0)

func test_map_id_truncated_to_byte() -> void:
	var packet := pm.form_tcp_packet(PacketizationManager.TYPE_SEARCH_GAME, 0x1FF)
	assert_eq(packet[4], 0xFF)

func test_padding_bytes_are_zero() -> void:
	var packet := pm.form_tcp_packet(PacketizationManager.TYPE_SEARCH_GAME, 0)
	for i in range(5, PacketizationManager.PACKET_SIZE):
		assert_eq(packet[i], 0, "byte %d should be zero" % i)

func test_request_type_little_endian_byte_order() -> void:
	# TYPE_STOP_SEARCH = 1, so bytes should be [0x01, 0x00, 0x00, 0x00]
	var packet := pm.form_tcp_packet(PacketizationManager.TYPE_STOP_SEARCH, 0)
	assert_eq(packet[0], 0x01)
	assert_eq(packet[1], 0x00)
	assert_eq(packet[2], 0x00)
	assert_eq(packet[3], 0x00)

func test_large_request_type_little_endian_byte_order() -> void:
	# 0x01020304 in LE → [0x04, 0x03, 0x02, 0x01]
	var packet := pm.form_tcp_packet(0x01020304, 0)
	assert_eq(packet[0], 0x04)
	assert_eq(packet[1], 0x03)
	assert_eq(packet[2], 0x02)
	assert_eq(packet[3], 0x01)

# ========================= interpret_tcp_packet: GAME_FOUND =========================

func _make_raw_packet() -> PackedByteArray:
	var raw := PackedByteArray()
	raw.resize(PacketizationManager.PACKET_SIZE)
	raw.fill(0)
	return raw

func _write_u32_le(buf: PackedByteArray, offset: int, value: int) -> void:
	buf[offset    ] =  value        & 0xFF
	buf[offset + 1] = (value >>  8) & 0xFF
	buf[offset + 2] = (value >> 16) & 0xFF
	buf[offset + 3] = (value >> 24) & 0xFF

func _write_u16_le(buf: PackedByteArray, offset: int, value: int) -> void:
	buf[offset    ] =  value       & 0xFF
	buf[offset + 1] = (value >> 8) & 0xFF

func _write_hex_bytes(buf: PackedByteArray, offset: int, hex: String) -> void:
	for i in range(hex.length() / 2):
		buf[offset + i] = hex.substr(i * 2, 2).hex_to_int()

func _make_game_found_packet(
	game_id_hex: String,
	player_id_hex: String,
	ip_u32: int,
	port: int
) -> PackedByteArray:
	var raw := _make_raw_packet()
	_write_u32_le(raw, 0, PacketizationManager.TYPE_GAME_FOUND)
	_write_hex_bytes(raw, 4, game_id_hex)
	_write_hex_bytes(raw, 20, player_id_hex)
	_write_u32_le(raw, 36, ip_u32)
	_write_u16_le(raw, 40, port)
	return raw

func test_game_found_response_type() -> void:
	var raw := _make_game_found_packet(
		"AABBCCDDEEFF00112233445566778899",
		"00112233445566778899AABBCCDDEEFF",
		0x8192_4D97,
		7777
	)
	var response := pm.interpret_tcp_packet(raw)
	assert_eq(response.response_type, PacketizationManager.TYPE_GAME_FOUND)

func test_game_found_game_id() -> void:
	var raw := _make_game_found_packet(
		"AABBCCDDEEFF00112233445566778899",
		"00112233445566778899AABBCCDDEEFF",
		0x8192_4D97,
		7777
	)
	var response := pm.interpret_tcp_packet(raw)
	assert_eq(response.game_id, "AABBCCDDEEFF00112233445566778899")

func test_game_found_player_id() -> void:
	var raw := _make_game_found_packet(
		"AABBCCDDEEFF00112233445566778899",
		"00112233445566778899AABBCCDDEEFF",
		0x8192_4D97,
		7777
	)
	var response := pm.interpret_tcp_packet(raw)
	assert_eq(response.player_id, "00112233445566778899AABBCCDDEEFF")

func test_game_found_server_ip() -> void:
	# 129.146.77.151 → big-endian u32 = 0x81924D97
	# stored as LE bytes [0x97, 0x4D, 0x92, 0x81], decoded back to 0x81924D97
	var raw := _make_game_found_packet(
		"AABBCCDDEEFF00112233445566778899",
		"00112233445566778899AABBCCDDEEFF",
		0x81924D97,
		7777
	)
	var response := pm.interpret_tcp_packet(raw)
	assert_eq(response.server_ip, "129.146.77.151")

func test_game_found_port() -> void:
	var raw := _make_game_found_packet(
		"AABBCCDDEEFF00112233445566778899",
		"00112233445566778899AABBCCDDEEFF",
		0x81924D97,
		7777
	)
	var response := pm.interpret_tcp_packet(raw)
	assert_eq(response.port, 7777)

func test_game_found_port_max_u16() -> void:
	var raw := _make_game_found_packet(
		"AABBCCDDEEFF00112233445566778899",
		"00112233445566778899AABBCCDDEEFF",
		0x81924D97,
		65535
	)
	var response := pm.interpret_tcp_packet(raw)
	assert_eq(response.port, 65535)

func test_game_found_all_zero_ids() -> void:
	var raw := _make_game_found_packet(
		"00000000000000000000000000000000",
		"00000000000000000000000000000000",
		0,
		0
	)
	var response := pm.interpret_tcp_packet(raw)
	assert_eq(response.game_id,   "00000000000000000000000000000000")
	assert_eq(response.player_id, "00000000000000000000000000000000")
	assert_eq(response.server_ip, "0.0.0.0")
	assert_eq(response.port, 0)

# ========================= interpret_tcp_packet: GAME_NOT_FOUND =========================

func test_game_not_found_response_type() -> void:
	var raw := _make_raw_packet()
	_write_u32_le(raw, 0, PacketizationManager.TYPE_GAME_NOT_FOUND)
	var response := pm.interpret_tcp_packet(raw)
	assert_eq(response.response_type, PacketizationManager.TYPE_GAME_NOT_FOUND)

func test_game_not_found_fields_not_populated() -> void:
	var raw := _make_raw_packet()
	_write_u32_le(raw, 0, PacketizationManager.TYPE_GAME_NOT_FOUND)
	var response := pm.interpret_tcp_packet(raw)
	assert_eq(response.game_id,   "")
	assert_eq(response.player_id, "")
	assert_eq(response.server_ip, "")
	assert_eq(response.port, 0)

# ========================= interpret_tcp_packet: unknown type =========================

func test_unknown_type_stored_as_response_type() -> void:
	var raw := _make_raw_packet()
	_write_u32_le(raw, 0, 99)
	var response := pm.interpret_tcp_packet(raw)
	assert_eq(response.response_type, 99)

# ========================= round-trip =========================

func test_form_then_interpret_search_game() -> void:
	var sent   := pm.form_tcp_packet(PacketizationManager.TYPE_SEARCH_GAME, 0)
	# Server echoes TYPE_GAME_NOT_FOUND back — just verify our outbound type survived encoding
	var type_id := sent[0] | (sent[1] << 8) | (sent[2] << 16) | (sent[3] << 24)
	assert_eq(type_id, PacketizationManager.TYPE_SEARCH_GAME)

func test_form_then_interpret_stop_search() -> void:
	var sent   := pm.form_tcp_packet(PacketizationManager.TYPE_STOP_SEARCH, 0)
	var type_id := sent[0] | (sent[1] << 8) | (sent[2] << 16) | (sent[3] << 24)
	assert_eq(type_id, PacketizationManager.TYPE_STOP_SEARCH)
