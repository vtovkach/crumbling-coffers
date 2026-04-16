extends Player
class_name RemotePlayer

var score: int = 0
var player_id: String	# This value should be populated by another class/function (Game class?) at this player's creation in online play

func _ready() -> void:
	add_to_group("remote")

func init(id: String, init_x: int, init_y: int) -> void:
	player_id = id
	server_update(init_x, init_y, 0, 0, 0)

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
#	state = _infer_state()

# dont mind this, when animation is added this will be possible to complete
#func _infer_state() -> AnimationState:
	#if velocity.y > 0:
		#state = jump
	#elif velocity.y < 0:
		#state = fall
	#elif velocity.x == 0:
		#state = idle
	#else:
		#state = run
