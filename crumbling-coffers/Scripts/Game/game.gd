extends Node2D
class_name Game

# ============================================================
# ENUMS
# ============================================================

enum GameStatus { PREMATCH, RUNNING, FINISHED }

# ============================================================
# CONSTANTS
# ============================================================

const UserPlayerScene:  PackedScene = preload("res://Scenes/Player/user_player.tscn")
const RemotePlayerScene: PackedScene = preload("res://Scenes/Player/remote_player.tscn")

const SEND_INTERVAL: float = 0.0167  # ~60 Hz

# ============================================================
# STATE
# ============================================================

var game_id:     String
var game_port:   int
var start_tick:  int = 0
var stop_tick:   int = 0
var game_status: GameStatus

var _send_accumulator: float = 0.0

# ============================================================
# PLAYERS
# ============================================================

var local_player:   UserPlayer
var remote_players: Dictionary = {}  # player_id (String) -> RemotePlayer

# ============================================================
# LIFECYCLE
# ============================================================

func _ready() -> void:
	pass

func _process(delta: float) -> void:
	_process_network(delta)

	if game_status == GameStatus.RUNNING:
		_send_accumulator += delta
		if _send_accumulator >= SEND_INTERVAL:
			_send_accumulator -= SEND_INTERVAL
			_send_local_player_data()

# ============================================================
# INIT
# ============================================================

func init(local_player_id: String, p_game_id: String, port: int, udp_response: PacketizationManager.UDP_Response) -> void:
	game_id     = p_game_id
	game_port   = port
	game_status = GameStatus.PREMATCH
	start_tick  = udp_response.start_tick
	stop_tick   = udp_response.stop_tick
	MatchManager.mode = MatchManager.MatchMode.MULTIPLAYER

	for player_id in udp_response.player_init_positions:
		var pos: PacketizationManager.PlayerInitPos = udp_response.player_init_positions[player_id]
		if player_id == local_player_id:
			local_player            = UserPlayerScene.instantiate()
			local_player.player_id  = local_player_id
			local_player.position   = Vector2(pos.x, pos.y)
			add_child(local_player)
			var hud = $Map/HUD
			hud.bind_to_player(local_player)
			hud.set_player_to_indicators(local_player)
			local_player.set_physics_process(false)
		else:
			var remote: RemotePlayer = RemotePlayerScene.instantiate()
			remote.init(player_id, pos.x, pos.y)
			add_child(remote)
			remote_players[player_id] = remote


# ============================================================
# NETWORK
# ============================================================

func _process_network(_delta: float) -> void:
	var raw := NetworkManager.receive_udp()
	while raw.size() > 0:
		var response := PacketizationManager.interpret_udp_packet(raw)

		if response.status == PacketizationManager.UDPStatus.ERROR:
			raw = NetworkManager.receive_udp()
			continue

		if response.packet_type == PacketizationManager.UDPPacketType.SERVER_INIT:
			raw = NetworkManager.receive_udp()
			continue

		if game_status == GameStatus.PREMATCH and response.server_cur_tick >= start_tick:
			game_status = GameStatus.RUNNING
			MatchManager.set_state(MatchManager.MatchState.RUNNING)
			local_player.set_physics_process(true)

		if game_status == GameStatus.RUNNING and response.server_cur_tick >= stop_tick:
			game_status = GameStatus.FINISHED
			local_player.set_physics_process(false)
			_on_end_match()

		if response.packet_type == PacketizationManager.UDPPacketType.SERVER_AUTH:
			var now := Time.get_ticks_msec()
			for player_id in response.players:
				if player_id in remote_players:
					var info: PacketizationManager.PlayerInfo = response.players[player_id]
					remote_players[player_id].push_packet(
						RemotePlayer.PlayerPacket.new(
							info.pos_x, info.pos_y,
							info.vel_x, info.vel_y,
							info.score, now
						)
					)

		raw = NetworkManager.receive_udp()

func _send_local_player_data() -> void:
	NetworkManager.send_udp(
		PacketizationManager.form_udp_reg_packet(
			game_id,
			local_player.player_id,
			game_port,
			local_player.position,
			local_player.velocity,
			local_player.score
		)
	)

# ============================================================
# MATCH EVENTS
# ============================================================

func _on_end_match() -> void:
	MatchManager.end_match()
