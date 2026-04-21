extends Player
class_name RemotePlayer

class PlayerPacket:
	var x:         float
	var y:         float
	var vx:        float
	var vy:        float
	var score:     int
	var timestamp: float  # timestamp is obtained by Time.get_ticks_msec() when packet was received in Game

	func _init(px: float, py: float, pvx: float, pvy: float, pscore: int, ts: float) -> void:
		x         = px
		y         = py
		vx        = pvx
		vy        = pvy
		score     = pscore
		timestamp = ts

var score: int = 0
var player_id: String
var packet_queue: Array[PlayerPacket] = []

func _ready() -> void:
	add_to_group("remote")

func init(id: String, init_x: float, init_y: float) -> void:
	player_id = id
	position.x = init_x
	position.y = init_y 
	velocity.x = 0
	velocity.y = 0
	score = 0

# Called in Game's _process() to push new player data
# Note for Nicholas — it is a rough draft, feel free to change (Array to heap/queue if needed)
# I know you might not like class PlayerPacket inside this script but feel free to change it as you want, but don't break other code
func push_data_packet(packet: PlayerPacket) -> void:
	packet_queue.push_back(packet)

# Called inside Game's _process(), gets render time
# Do interpolation/extrapolation using packet_queue and all other player's updates like score animations etc... 
func render_remote_player(render_time: float) -> void:
	while packet_queue.size() >= 2 and packet_queue[1].timestamp <= render_time:
		packet_queue.pop_front()

	if packet_queue.size() >= 2:
		var from: PlayerPacket = packet_queue[0]
		var to:   PlayerPacket = packet_queue[1]
		var alpha: float = clamp((render_time - from.timestamp) / (to.timestamp - from.timestamp), 0.0, 1.0)
		position = lerp(Vector2(from.x, from.y), Vector2(to.x, to.y), alpha)
		score = from.score
	elif packet_queue.size() == 1:
		position = Vector2(packet_queue[0].x, packet_queue[0].y)
		score = packet_queue[0].score