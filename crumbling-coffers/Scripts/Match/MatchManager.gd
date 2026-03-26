extends Node

# Match Manager will handle timer logic for SINGLEPLAYER ONLY (currently). For multiplayer logic, the server will 
# be keeping track of the timer globally. This is the first iteration of the MatchManager autoload.

# IMPLEMENTING STATES FOR MATCH AND IF SINGLEPLAYER OR MULTIPLAYER (remove comment later).
enum MatchState {WAITING, COUNTDOWN, RUNNING, ENDED} # May add more states later on.
enum MatchMode {SINGLEPLAYER, MULTIPLAYER}
# Set the current state to waiting by default.
var current_state: MatchState = MatchState.WAITING
# Set the mode to singleplayer by default.
var mode: MatchMode = MatchMode.SINGLEPLAYER

var time_left: int = 60
@onready var timer: Timer

# Signals that will be called out whenever emitted to notify other functions.
signal time_updated(time: int) 
signal match_ended() 
signal state_changed(new_state: MatchState)

func _ready() -> void:
	# Initializing MatchManager's timer when ran.
	timer = Timer.new()
	timer.wait_time = 1.0
	timer.one_shot = false # Set to false in order to have timer reset and be used again.
	timer.autostart = false # Manually start the timer.
	add_child(timer)
	
	timer.timeout.connect(_on_game_timer_timeout)

# Set the state to change.
# will set current_state to the state passed into set_state and signal to state_changed
func set_state(new_state: MatchState) -> void:
	current_state = new_state
	state_changed.emit(current_state)

# When called, will assign the new time and emmit signal to have HUD update UI.
func set_time(new_time: int) -> void:
	time_left = new_time
	time_updated.emit(time_left)
	
# Will set the state to COUNTDOWN when called.
func start_countdown() -> void:
	set_state(MatchState.COUNTDOWN)

# Responsibility will not call to start the timer, will only call to set the time and new state.
# UNLESS MODE IS SINGLEPLAYER, the timer is not started by start match since handled by server.
func start_match(match_duration: int) -> void:
	set_time(match_duration)
	set_state(MatchState.RUNNING)

# check mode to see if need to start timer or not when countdown ends.

# New end_match function that will change the match state to ENDED wehen the timer runs out.
func end_match() -> void:
	timer.stop()
	set_state(MatchState.ENDED)
	match_ended.emit()


# Decrementing timer functionality. Will need to change so calls set_time instead of decrementing timer itself.
func _on_game_timer_timeout() -> void:
	# Now will just check if the match state is anything else other than running.
	if current_state != MatchState.RUNNING:
		return
	# decrement time by 1 second.
	set_time(time_left - 1)
	
