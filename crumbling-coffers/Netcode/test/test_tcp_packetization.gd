extends GutTest


# ── form_tcp_packet ──────────────────────────────────────────────────────────

func test_tcp_packet_is_correct_size() -> void:
	var pkt := PacketizationManager.form_tcp_packet(PacketizationManager.TYPE_SEARCH_GAME, 0)
	assert_eq(pkt.size(), PacketizationManager.PACKET_SIZE,
		"TCP packet must be exactly PACKET_SIZE bytes")

func test_tcp_packet_search_game_type_field() -> void:
	var pkt := PacketizationManager.form_tcp_packet(PacketizationManager.TYPE_SEARCH_GAME, 0)
	var type_id := pkt[0] | (pkt[1] << 8) | (pkt[2] << 16) | (pkt[3] << 24)
	assert_eq(type_id, PacketizationManager.TYPE_SEARCH_GAME,
		"Bytes [0:4] must encode TYPE_SEARCH_GAME as little-endian uint32")

func test_tcp_packet_stop_search_type_field() -> void:
	var pkt := PacketizationManager.form_tcp_packet(PacketizationManager.TYPE_STOP_SEARCH, 0)
	var type_id := pkt[0] | (pkt[1] << 8) | (pkt[2] << 16) | (pkt[3] << 24)
	assert_eq(type_id, PacketizationManager.TYPE_STOP_SEARCH,
		"Bytes [0:4] must encode TYPE_STOP_SEARCH as little-endian uint32")

func test_tcp_packet_map_id_byte() -> void:
	var pkt := PacketizationManager.form_tcp_packet(PacketizationManager.TYPE_SEARCH_GAME, 7)
	assert_eq(pkt[4], 7, "Byte [4] must carry the map_id")

func test_tcp_packet_map_id_masked_to_byte() -> void:
	var pkt := PacketizationManager.form_tcp_packet(PacketizationManager.TYPE_SEARCH_GAME, 0x1FF)
	assert_eq(pkt[4], 0xFF, "map_id must be masked to a single byte (& 0xFF)")

func test_tcp_packet_remaining_bytes_are_zero() -> void:
	var pkt := PacketizationManager.form_tcp_packet(PacketizationManager.TYPE_SEARCH_GAME, 0)
	for i in range(5, PacketizationManager.PACKET_SIZE):
		if pkt[i] != 0:
			fail_test("Byte %d should be 0, got %d" % [i, pkt[i]])
			return
	pass_test("All padding bytes are zero")


# ── interpret_tcp_packet — GAME_FOUND ────────────────────────────────────────

func _make_game_found_packet(game_id_hex: String, player_id_hex: String,
		ip_u32_be: int, port: int) -> PackedByteArray:
	var raw := PackedByteArray()
	raw.resize(PacketizationManager.PACKET_SIZE)
	raw.fill(0)
	raw[0] = PacketizationManager.TYPE_GAME_FOUND & 0xFF
	raw[1] = (PacketizationManager.TYPE_GAME_FOUND >> 8) & 0xFF
	raw[2] = (PacketizationManager.TYPE_GAME_FOUND >> 16) & 0xFF
	raw[3] = (PacketizationManager.TYPE_GAME_FOUND >> 24) & 0xFF
	for i in range(16):
		raw[4 + i] = game_id_hex.substr(i * 2, 2).hex_to_int()
	for i in range(16):
		raw[20 + i] = player_id_hex.substr(i * 2, 2).hex_to_int()
	raw[36] = (ip_u32_be >> 24) & 0xFF
	raw[37] = (ip_u32_be >> 16) & 0xFF
	raw[38] = (ip_u32_be >>  8) & 0xFF
	raw[39] =  ip_u32_be        & 0xFF
	raw[40] =  port       & 0xFF
	raw[41] = (port >> 8) & 0xFF
	return raw

func test_interpret_game_found_response_type() -> void:
	var raw := _make_game_found_packet(
		"AABBCCDDEEFF00112233445566778899",
		"00112233445566778899AABBCCDDEEFF",
		0x81924D97, 7777)
	var resp := PacketizationManager.interpret_tcp_packet(raw)
	assert_eq(resp.response_type, PacketizationManager.TYPE_GAME_FOUND,
		"response_type must be TYPE_GAME_FOUND")

func test_interpret_game_found_game_id() -> void:
	var raw := _make_game_found_packet(
		"AABBCCDDEEFF00112233445566778899",
		"00112233445566778899AABBCCDDEEFF",
		0x81924D97, 7777)
	var resp := PacketizationManager.interpret_tcp_packet(raw)
	assert_eq(resp.game_id, "AABBCCDDEEFF00112233445566778899",
		"game_id must round-trip through packet encoding")

func test_interpret_game_found_player_id() -> void:
	var raw := _make_game_found_packet(
		"AABBCCDDEEFF00112233445566778899",
		"00112233445566778899AABBCCDDEEFF",
		0x81924D97, 7777)
	var resp := PacketizationManager.interpret_tcp_packet(raw)
	assert_eq(resp.player_id, "00112233445566778899AABBCCDDEEFF",
		"player_id must round-trip through packet encoding")

func test_interpret_game_found_server_ip() -> void:
	var raw := PackedByteArray()
	raw.resize(PacketizationManager.PACKET_SIZE)
	raw.fill(0)
	raw[0] = PacketizationManager.TYPE_GAME_FOUND & 0xFF
	# LE-encode so _decode_u32_le produces 0x81924D97 → _u32_to_ipv4 → "129.146.77.151"
	raw[36] = 151
	raw[37] = 77
	raw[38] = 146
	raw[39] = 129
	var resp := PacketizationManager.interpret_tcp_packet(raw)
	assert_eq(resp.server_ip, "129.146.77.151", "server_ip must decode to dotted IPv4")

func test_interpret_game_found_port() -> void:
	var raw := _make_game_found_packet(
		"AABBCCDDEEFF00112233445566778899",
		"00112233445566778899AABBCCDDEEFF",
		0x81924D97, 7777)
	var resp := PacketizationManager.interpret_tcp_packet(raw)
	assert_eq(resp.port, 7777, "port must round-trip through LE u16 encoding")


# ── interpret_tcp_packet — GAME_NOT_FOUND ────────────────────────────────────

func test_interpret_game_not_found_response_type() -> void:
	var raw := PackedByteArray()
	raw.resize(PacketizationManager.PACKET_SIZE)
	raw.fill(0)
	raw[0] = PacketizationManager.TYPE_GAME_NOT_FOUND & 0xFF
	raw[1] = (PacketizationManager.TYPE_GAME_NOT_FOUND >> 8) & 0xFF
	raw[2] = (PacketizationManager.TYPE_GAME_NOT_FOUND >> 16) & 0xFF
	raw[3] = (PacketizationManager.TYPE_GAME_NOT_FOUND >> 24) & 0xFF
	var resp := PacketizationManager.interpret_tcp_packet(raw)
	assert_eq(resp.response_type, PacketizationManager.TYPE_GAME_NOT_FOUND,
		"response_type must be TYPE_GAME_NOT_FOUND")

func test_interpret_game_not_found_leaves_game_id_empty() -> void:
	var raw := PackedByteArray()
	raw.resize(PacketizationManager.PACKET_SIZE)
	raw.fill(0)
	raw[0] = PacketizationManager.TYPE_GAME_NOT_FOUND & 0xFF
	var resp := PacketizationManager.interpret_tcp_packet(raw)
	assert_eq(resp.game_id, "", "game_id must not be populated for GAME_NOT_FOUND")
