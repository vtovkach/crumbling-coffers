extends Node2D
class_name Game

enum GameStatus { PREMATCH, RUNNING, FINISHED }

const UserPlayerScene: PackedScene = preload("res://Scenes/Player/user_player.tscn")
const RemotePlayerScene: PackedScene = preload("res://Scenes/Player/remote_player.tscn")

var game_id: String
var local_player: UserPlayer
var remote_players: Array[RemotePlayer] = []

var start_tick: int = 0
var stop_tick: int = 0 
var game_status: GameStatus

func _ready() -> void:
	pass

func init(local_player_id: String, p_game_id: String, udp_response: PacketizationManager.UDP_Response) -> void:
	game_id = p_game_id
	game_status = GameStatus.PREMATCH
	start_tick = udp_response.start_tick
	stop_tick = udp_response.stop_tick

	for player_id in udp_response.player_init_positions:
		var pos: PacketizationManager.PlayerInitPos = udp_response.player_init_positions[player_id]
		if player_id == local_player_id:
			local_player = UserPlayerScene.instantiate()
			local_player.player_id = local_player_id
			local_player.position = Vector2(pos.x, pos.y)
			add_child(local_player)
			var hud = $Map/HUD
			hud.bind_to_player(local_player)
			hud.set_player_to_indicators(local_player)
		else:
			var remote: RemotePlayer = RemotePlayerScene.instantiate()
			remote.init(player_id, pos.x, pos.y)
			add_child(remote)
			remote_players.append(remote)

func _process(delta: float) -> void:
	
	_drain_network(delta)

# Drains NetworkManager, routes each packet to the correct RemotePlayer,
# then advances all remote positions for this frame.
func _drain_network(delta: float) -> void:
	pass

func _send_local_player_data():
	pass 

func _on_end_match() -> void:
	pass
