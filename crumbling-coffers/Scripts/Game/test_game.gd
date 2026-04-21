extends Game

const LOCAL_ID: String = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"

var _tick_accumulator: float = 0.0

func _ready() -> void:
	var response: PacketizationManager.UDP_Response = PacketizationManager.UDP_Response.new()
	response.start_tick = 450
	response.stop_tick  = 1000
	response.player_init_positions[LOCAL_ID] = PacketizationManager.PlayerInitPos.new(-425, -200)

	init(LOCAL_ID, "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB", 34567, response)

func _process(delta: float) -> void:
	_tick_accumulator += delta
	if _tick_accumulator >= (1.0 / SERVER_TICK_RATE):
		_tick_accumulator -= (1.0 / SERVER_TICK_RATE)
		server_tick += 1

	super(delta)
