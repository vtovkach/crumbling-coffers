extends Player
class_name RemotePlayer

class PlayerPacket:
	var x:         int
	var y:         int
	var vx:        int
	var vy:        int
	var score:     int
	var timestamp: int  # timestamp is obtained by Time.get_ticks_msec() when packet was received in Game 

	func _init(px: int, py: int, pvx: int, pvy: int, pscore: int, ts: int) -> void:
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

func init(id: String, init_x: int, init_y: int) -> void:
	player_id = id
	server_update(init_x, init_y, 0, 0, 0)

func push_packet(packet: PlayerPacket) -> void:
	packet_queue.push_back(packet)

# Populate with fields from PlayerInfo
func server_update(x: int, y: int, vx: int, vy: int, points: int) -> void:
	position.x = x
	position.y = y
	velocity.x = vx
	velocity.y = vy
	score = points
	_internal_update()

# update based on latest server values
# may include interpolation / extrapolation (Future Task)
# or animation (Future Task)
func _internal_update() -> void:
	direction = sign(velocity.x)
