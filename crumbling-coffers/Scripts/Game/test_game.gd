extends Game

const LOCAL_ID: String = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"

func _ready() -> void:
	var response := PacketizationManager.UDP_Response.new()
	response.start_tick = 0
	response.stop_tick  = 999999
	response.player_init_positions[LOCAL_ID] = PacketizationManager.PlayerInitPos.new(-425, -200)

	init(LOCAL_ID, "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", 34567, response)
	local_player.set_physics_process(true)
