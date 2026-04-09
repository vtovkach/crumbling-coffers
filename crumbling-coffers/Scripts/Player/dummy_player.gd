extends ControllablePlayer
class_name DummyPlayer

var jump_chance: float = 0.005 # chance per FRAME
var max_jump_hold_duration: float = 0.5
var jump_hold_duration: float = 0

func _ready() -> void:
	add_to_group("freezable")
	add_to_group("disorientable")
	add_to_group("slowable")
	
func _physics_process(delta: float) -> void:
	jump_tapped = false
	
	# Get inputs
	if not is_frozen:
		set_direction_collision()
		do_fun_jumps(delta)
				
	super(delta)

func set_direction_collision() -> void:
	if is_on_wall():
		var collision = get_last_slide_collision()
		if collision:
			var normal = collision.get_normal()
			input_direction = normal.x
	elif input_direction == 0:
		input_direction = 2*(int)(2*randf()) - 1  #just randomly get -1 or 1 quickly

func do_fun_jumps(delta: float) -> void:
	var random = randf()
	if(!jump_pressed and jump_chance > random):
		jump_tapped = true
		jump_pressed = true
	if jump_pressed:
		jump_hold_duration += delta
		if jump_hold_duration >= max_jump_hold_duration:
			jump_pressed = false
			randomize_jump_duration()

# simply put jump holding on a range from 0.5 to 1.5 seconds
func randomize_jump_duration() -> void:
	max_jump_hold_duration = randf() + 0.5
