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

# ========================= form_tcp_packet: extreme =========================

func test_map_id_boundary_0xFF() -> void:
	var packet := pm.form_tcp_packet(PacketizationManager.TYPE_SEARCH_GAME, 0xFF)
	assert_eq(packet[4], 0xFF)

func test_search_game_type_zero_bytes_explicit() -> void:
	# TYPE_SEARCH_GAME = 0, so all four type bytes must be 0x00 — not just "unset"
	var packet := pm.form_tcp_packet(PacketizationManager.TYPE_SEARCH_GAME, 0)
	assert_eq(packet[0], 0x00)
	assert_eq(packet[1], 0x00)
	assert_eq(packet[2], 0x00)
	assert_eq(packet[3], 0x00)

func test_no_state_leak_between_calls() -> void:
	var p1 := pm.form_tcp_packet(PacketizationManager.TYPE_SEARCH_GAME, 1)
	var p2 := pm.form_tcp_packet(PacketizationManager.TYPE_STOP_SEARCH, 2)
	var type1 := p1[0] | (p1[1] << 8) | (p1[2] << 16) | (p1[3] << 24)
	var type2 := p2[0] | (p2[1] << 8) | (p2[2] << 16) | (p2[3] << 24)
	assert_eq(type1, PacketizationManager.TYPE_SEARCH_GAME)
	assert_eq(p1[4], 1)
	assert_eq(type2, PacketizationManager.TYPE_STOP_SEARCH)
	assert_eq(p2[4], 2)

# ========================= interpret_tcp_packet: extreme =========================

func test_game_found_max_ip() -> void:
	# 0xFFFFFFFF → "255.255.255.255"
	var raw := _make_game_found_packet(
		"AABBCCDDEEFF00112233445566778899",
		"00112233445566778899AABBCCDDEEFF",
		0xFFFFFFFF,
		80
	)
	var response := pm.interpret_tcp_packet(raw)
	assert_eq(response.server_ip, "255.255.255.255")

func test_game_found_small_ip() -> void:
	# 0x00000001 → "0.0.0.1"
	var raw := _make_game_found_packet(
		"AABBCCDDEEFF00112233445566778899",
		"00112233445566778899AABBCCDDEEFF",
		0x00000001,
		80
	)
	var response := pm.interpret_tcp_packet(raw)
	assert_eq(response.server_ip, "0.0.0.1")

func test_game_found_port_min_nonzero() -> void:
	var raw := _make_game_found_packet(
		"AABBCCDDEEFF00112233445566778899",
		"00112233445566778899AABBCCDDEEFF",
		0x81924D97,
		1
	)
	var response := pm.interpret_tcp_packet(raw)
	assert_eq(response.port, 1)

func test_game_found_all_ff_ids() -> void:
	var raw := _make_game_found_packet(
		"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",
		"FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",
		0x81924D97,
		7777
	)
	var response := pm.interpret_tcp_packet(raw)
	assert_eq(response.game_id,   "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF")
	assert_eq(response.player_id, "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF")

func test_all_zero_packet_unknown_type() -> void:
	# type_id = 0, not a known response type
	var raw := _make_raw_packet()
	var response := pm.interpret_tcp_packet(raw)
	assert_eq(response.response_type, 0)
	assert_eq(response.game_id,   "")
	assert_eq(response.player_id, "")
	assert_eq(response.server_ip, "")
	assert_eq(response.port, 0)

# ========================= UDP helpers =========================

func _write_udp_header(
	buf:          PackedByteArray,
	game_id_hex:  String,
	player_id_hex: String,
	ctrl:         int,
	payload_size: int,
	seq_num:      int
) -> void:
	_write_hex_bytes(buf, 0,  game_id_hex)
	_write_hex_bytes(buf, 16, player_id_hex)
	_write_u16_le(buf, 32, ctrl)
	_write_u16_le(buf, 34, payload_size)
	_write_u32_le(buf, 36, seq_num)

func _write_i32_le(buf: PackedByteArray, offset: int, value: int) -> void:
	var bits := value & 0xFFFFFFFF
	buf[offset    ] =  bits        & 0xFF
	buf[offset + 1] = (bits >>  8) & 0xFF
	buf[offset + 2] = (bits >> 16) & 0xFF
	buf[offset + 3] = (bits >> 24) & 0xFF

const GAME_ID   := "AABBCCDDEEFF00112233445566778899"
const PLAYER_ID := "00112233445566778899AABBCCDDEEFF"

# ========================= form_udp_init_packet =========================

func test_udp_init_packet_size() -> void:
	var udp_pkt = pm.form_udp_init_packet(GAME_ID, PLAYER_ID, 9000)
	assert_eq(udp_pkt.payload.size(), PacketizationManager.PACKET_SIZE)

func test_udp_init_packet_port() -> void:
	var udp_pkt = pm.form_udp_init_packet(GAME_ID, PLAYER_ID, 9000)
	assert_eq(udp_pkt.port, 9000)

func test_udp_init_game_id_encoded() -> void:
	var udp_pkt = pm.form_udp_init_packet(GAME_ID, PLAYER_ID, 9000)
	var result  := ""
	for i in range(16):
		result += "%02X" % udp_pkt.payload[i]
	assert_eq(result, GAME_ID)

func test_udp_init_player_id_encoded() -> void:
	var udp_pkt = pm.form_udp_init_packet(GAME_ID, PLAYER_ID, 9000)
	var result  := ""
	for i in range(16):
		result += "%02X" % udp_pkt.payload[16 + i]
	assert_eq(result, PLAYER_ID)

func test_udp_init_ctrl_bits() -> void:
	var udp_pkt = pm.form_udp_init_packet(GAME_ID, PLAYER_ID, 9000)
	var ctrl    := udp_pkt.payload[32] | (udp_pkt.payload[33] << 8)
	assert_eq(ctrl, PacketizationManager.UDP_CTRL_INIT)

func test_udp_init_payload_size_field_is_zero() -> void:
	var udp_pkt      = pm.form_udp_init_packet(GAME_ID, PLAYER_ID, 9000)
	var payload_size := udp_pkt.payload[34] | (udp_pkt.payload[35] << 8)
	assert_eq(payload_size, 0)

func test_udp_init_seq_num_is_zero() -> void:
	var udp_pkt = pm.form_udp_init_packet(GAME_ID, PLAYER_ID, 9000)
	var seq     := udp_pkt.payload[36] | (udp_pkt.payload[37] << 8) | (udp_pkt.payload[38] << 16) | (udp_pkt.payload[39] << 24)
	assert_eq(seq, 0)

func test_udp_init_payload_all_zero() -> void:
	var udp_pkt = pm.form_udp_init_packet(GAME_ID, PLAYER_ID, 9000)
	for i in range(40, PacketizationManager.PACKET_SIZE):
		assert_eq(udp_pkt.payload[i], 0, "payload byte %d should be zero" % i)

# ========================= form_udp_reg_packet =========================

func test_udp_reg_packet_size() -> void:
	var udp_pkt = pm.form_udp_reg_packet(GAME_ID, PLAYER_ID, 9001, 42, Vector2(10, 20), Vector2(3, -4), 100)
	assert_eq(udp_pkt.payload.size(), PacketizationManager.PACKET_SIZE)

func test_udp_reg_packet_port() -> void:
	var udp_pkt = pm.form_udp_reg_packet(GAME_ID, PLAYER_ID, 9001, 42, Vector2(10, 20), Vector2(3, -4), 100)
	assert_eq(udp_pkt.port, 9001)

func test_udp_reg_ctrl_bits_cleared() -> void:
	var udp_pkt = pm.form_udp_reg_packet(GAME_ID, PLAYER_ID, 9001, 42, Vector2(10, 20), Vector2(3, -4), 100)
	var ctrl    := udp_pkt.payload[32] | (udp_pkt.payload[33] << 8)
	assert_eq(ctrl, PacketizationManager.UDP_CTRL_REGULAR)

func test_udp_reg_payload_size_field() -> void:
	var udp_pkt      = pm.form_udp_reg_packet(GAME_ID, PLAYER_ID, 9001, 42, Vector2(10, 20), Vector2(3, -4), 100)
	var payload_size := udp_pkt.payload[34] | (udp_pkt.payload[35] << 8)
	assert_eq(payload_size, PacketizationManager.UDP_REG_PAYLOAD_SIZE)

func test_udp_reg_seq_num() -> void:
	var udp_pkt = pm.form_udp_reg_packet(GAME_ID, PLAYER_ID, 9001, 42, Vector2(10, 20), Vector2(3, -4), 100)
	var seq     := udp_pkt.payload[36] | (udp_pkt.payload[37] << 8) | (udp_pkt.payload[38] << 16) | (udp_pkt.payload[39] << 24)
	assert_eq(seq, 42)

func test_udp_reg_pos_x() -> void:
	var udp_pkt = pm.form_udp_reg_packet(GAME_ID, PLAYER_ID, 9001, 0, Vector2(300, 0), Vector2(0, 0), 0)
	var val     := udp_pkt.payload[40] | (udp_pkt.payload[41] << 8) | (udp_pkt.payload[42] << 16) | (udp_pkt.payload[43] << 24)
	assert_eq(val, 300)

func test_udp_reg_pos_y() -> void:
	var udp_pkt = pm.form_udp_reg_packet(GAME_ID, PLAYER_ID, 9001, 0, Vector2(0, 500), Vector2(0, 0), 0)
	var val     := udp_pkt.payload[44] | (udp_pkt.payload[45] << 8) | (udp_pkt.payload[46] << 16) | (udp_pkt.payload[47] << 24)
	assert_eq(val, 500)

func test_udp_reg_negative_velocity() -> void:
	var udp_pkt = pm.form_udp_reg_packet(GAME_ID, PLAYER_ID, 9001, 0, Vector2(0, 0), Vector2(-7, -3), 0)
	var vx_raw  := udp_pkt.payload[48] | (udp_pkt.payload[49] << 8) | (udp_pkt.payload[50] << 16) | (udp_pkt.payload[51] << 24)
	var vy_raw  := udp_pkt.payload[52] | (udp_pkt.payload[53] << 8) | (udp_pkt.payload[54] << 16) | (udp_pkt.payload[55] << 24)
	# two's complement: -7 = 0xFFFFFFF9, -3 = 0xFFFFFFFD
	assert_eq(vx_raw, -7 & 0xFFFFFFFF)
	assert_eq(vy_raw, -3 & 0xFFFFFFFF)

func test_udp_reg_score() -> void:
	var udp_pkt = pm.form_udp_reg_packet(GAME_ID, PLAYER_ID, 9001, 0, Vector2(0, 0), Vector2(0, 0), 9999)
	var val     := udp_pkt.payload[56] | (udp_pkt.payload[57] << 8) | (udp_pkt.payload[58] << 16) | (udp_pkt.payload[59] << 24)
	assert_eq(val, 9999)

func test_udp_reg_padding_after_payload() -> void:
	var udp_pkt = pm.form_udp_reg_packet(GAME_ID, PLAYER_ID, 9001, 0, Vector2(1, 2), Vector2(3, 4), 5)
	for i in range(60, PacketizationManager.PACKET_SIZE):
		assert_eq(udp_pkt.payload[i], 0, "byte %d should be zero" % i)

# ========================= interpret_udp_packet: SERVER_INIT =========================

func _make_server_init_raw(
	game_id_hex: String,
	seq_num:     int,
	start_tick:  int,
	stop_tick:   int,
	players:     Array  # Array of [pid_hex, x, y]
) -> PackedByteArray:
	var raw := _make_raw_packet()
	_write_udp_header(raw, game_id_hex, "00000000000000000000000000000000",
		PacketizationManager.UDP_CTRL_SERVER_INIT, 0, seq_num)
	_write_u32_le(raw, 40, start_tick)
	_write_u32_le(raw, 44, stop_tick)
	raw[48] = players.size()
	for i in range(players.size()):
		var base := 49 + i * 24
		_write_hex_bytes(raw, base, players[i][0])
		_write_i32_le(raw, base + 16, players[i][1])
		_write_i32_le(raw, base + 20, players[i][2])
	return raw

func test_server_init_status_normal() -> void:
	var raw      := _make_server_init_raw(GAME_ID, 10, 1, 100, [])
	var response := pm.interpret_udp_packet(raw)
	assert_eq(response.status, PacketizationManager.UDPStatus.NORMAL)

func test_server_init_packet_type() -> void:
	var raw      := _make_server_init_raw(GAME_ID, 10, 1, 100, [])
	var response := pm.interpret_udp_packet(raw)
	assert_eq(response.packet_type, PacketizationManager.UDPPacketType.SERVER_INIT)

func test_server_init_server_cur_tick() -> void:
	var raw      := _make_server_init_raw(GAME_ID, 77, 1, 100, [])
	var response := pm.interpret_udp_packet(raw)
	assert_eq(response.server_cur_tick, 77)

func test_server_init_start_and_stop_tick() -> void:
	var raw      := _make_server_init_raw(GAME_ID, 0, 5, 200, [])
	var response := pm.interpret_udp_packet(raw)
	assert_eq(response.start_tick, 5)
	assert_eq(response.stop_tick, 200)

func test_server_init_num_players() -> void:
	var raw := _make_server_init_raw(GAME_ID, 0, 0, 0, [
		[GAME_ID,   10, 20],
		[PLAYER_ID, 30, 40],
	])
	var response := pm.interpret_udp_packet(raw)
	assert_eq(response.num_players, 2)

func test_server_init_player_positions() -> void:
	var raw := _make_server_init_raw(GAME_ID, 0, 0, 0, [
		[GAME_ID,   100, -50],
		[PLAYER_ID, -200, 75],
	])
	var response := pm.interpret_udp_packet(raw)
	assert_true(response.player_init_positions.has(GAME_ID))
	assert_true(response.player_init_positions.has(PLAYER_ID))
	assert_eq(response.player_init_positions[GAME_ID].x,    100)
	assert_eq(response.player_init_positions[GAME_ID].y,    -50)
	assert_eq(response.player_init_positions[PLAYER_ID].x, -200)
	assert_eq(response.player_init_positions[PLAYER_ID].y,   75)

func test_server_init_zero_players() -> void:
	var raw      := _make_server_init_raw(GAME_ID, 0, 0, 0, [])
	var response := pm.interpret_udp_packet(raw)
	assert_eq(response.num_players, 0)
	assert_true(response.player_init_positions.is_empty())

# ========================= interpret_udp_packet: SERVER_AUTH =========================

func _make_server_auth_raw(
	game_id_hex: String,
	seq_num:     int,
	players:     Array  # Array of [pid_hex, pos_x, pos_y, vel_x, vel_y, score]
) -> PackedByteArray:
	var raw := _make_raw_packet()
	_write_udp_header(raw, game_id_hex, "00000000000000000000000000000000",
		PacketizationManager.UDP_CTRL_SERVER_AUTH, 0, seq_num)
	raw[40] = players.size()
	for i in range(players.size()):
		var base := 41 + i * 36
		_write_hex_bytes(raw, base, players[i][0])
		_write_i32_le(raw, base + 16, players[i][1])
		_write_i32_le(raw, base + 20, players[i][2])
		_write_i32_le(raw, base + 24, players[i][3])
		_write_i32_le(raw, base + 28, players[i][4])
		_write_u32_le(raw, base + 32, players[i][5])
	return raw

func test_server_auth_status_normal() -> void:
	var raw      := _make_server_auth_raw(GAME_ID, 5, [])
	var response := pm.interpret_udp_packet(raw)
	assert_eq(response.status, PacketizationManager.UDPStatus.NORMAL)

func test_server_auth_packet_type() -> void:
	var raw      := _make_server_auth_raw(GAME_ID, 5, [])
	var response := pm.interpret_udp_packet(raw)
	assert_eq(response.packet_type, PacketizationManager.UDPPacketType.SERVER_AUTH)

func test_server_auth_server_cur_tick() -> void:
	var raw      := _make_server_auth_raw(GAME_ID, 55, [])
	var response := pm.interpret_udp_packet(raw)
	assert_eq(response.server_cur_tick, 55)

func test_server_auth_num_players() -> void:
	var raw := _make_server_auth_raw(GAME_ID, 0, [
		[GAME_ID,   10, 20, 1, 2, 50],
		[PLAYER_ID, 30, 40, 3, 4, 80],
	])
	var response := pm.interpret_udp_packet(raw)
	assert_eq(response.num_players, 2)

func test_server_auth_player_fields() -> void:
	var raw := _make_server_auth_raw(GAME_ID, 0, [
		[GAME_ID, 100, -50, -7, 3, 999],
	])
	var response := pm.interpret_udp_packet(raw)
	assert_true(response.players.has(GAME_ID))
	var p = response.players[GAME_ID]
	assert_eq(p.pos_x,  100)
	assert_eq(p.pos_y,  -50)
	assert_eq(p.vel_x,  -7)
	assert_eq(p.vel_y,   3)
	assert_eq(p.score,  999)

func test_server_auth_multiple_players() -> void:
	var raw := _make_server_auth_raw(GAME_ID, 0, [
		[GAME_ID,   1, 2, 3, 4, 10],
		[PLAYER_ID, 5, 6, 7, 8, 20],
	])
	var response := pm.interpret_udp_packet(raw)
	assert_true(response.players.has(GAME_ID))
	assert_true(response.players.has(PLAYER_ID))
	assert_eq(response.players[GAME_ID].score,   10)
	assert_eq(response.players[PLAYER_ID].score, 20)

func test_server_auth_zero_players() -> void:
	var raw      := _make_server_auth_raw(GAME_ID, 0, [])
	var response := pm.interpret_udp_packet(raw)
	assert_eq(response.num_players, 0)
	assert_true(response.players.is_empty())

# ========================= interpret_udp_packet: error cases =========================

func test_interpret_udp_wrong_size_returns_error() -> void:
	var raw      := PackedByteArray()
	raw.resize(100)
	raw.fill(0)
	var response := pm.interpret_udp_packet(raw)
	assert_eq(response.status, PacketizationManager.UDPStatus.ERROR)

func test_interpret_udp_unknown_ctrl_returns_error() -> void:
	var raw := _make_raw_packet()
	_write_udp_header(raw, GAME_ID, PLAYER_ID, 0x00F0, 0, 0)
	var response := pm.interpret_udp_packet(raw)
	assert_eq(response.status, PacketizationManager.UDPStatus.ERROR)
