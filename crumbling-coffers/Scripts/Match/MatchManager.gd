extends Node

# Match Manager will handle timer logic for SINGLEPLAYER ONLY (currently). For multiplayer logic, the server will 
# be keeping track of the timer globally. This is the first iteration of the MatchManager autoload.

enum MatchState {WAITING, COUNTDOWN, RUNNING, ENDED} # May add more states later on.
enum MatchMode {SINGLEPLAYER, MULTIPLAYER}
# Set the current state to waiting by default.
var current_state: MatchState = MatchState.WAITING
# Set the mode to singleplayer by default.
var mode: MatchMode = MatchMode.SINGLEPLAYER

# Variables for score page to hold results of last match
var final_score: int = 0

var time_left: int = 60
@onready var timer: Timer

# Signals that will be called out whenever emitted to notify other functions.
signal time_updated(time: int) 
signal match_ended() 
signal state_changed(new_state: MatchState)
signal match_ready

# Broadcasts end of match to all clients
@rpc("authority", "call_local", "reliable")
func end_match_rpc() -> void:
	end_match()

func _ready() -> void:
	# Connect handshake to start function
	# Later can be updated to wait for mulpitle match_ready signals before starting match
	match_ready.connect(func(): start_match(60))
	# Initializing MatchManager's timer when ran.
	timer = Timer.new()
	timer.wait_time = 1.0
	timer.one_shot = false # Set to false in order to have timer reset and be used again.
	timer.autostart = false # Manually start the timer.
	add_child(timer)
	
	timer.timeout.connect(_on_game_timer_timeout)
	#print(current_state) # To check if current_state = 0 (which is WAITING)


# set_state: Will assign/set the current state that is passed to the function and emit corresponding signal.
func set_state(new_state: MatchState) -> void:
	current_state = new_state
	state_changed.emit(current_state)
# Debugging code below to check if singleplayer mode is in proper state.
#	print("I am now this current state:")
#	print(current_state)

# set_time: When called, will assign the new time and emit signal to have HUD update UI.
func set_time(new_time: int) -> void:
	time_left = new_time
	time_updated.emit(time_left)
	
	# Check if the time_left is 0, then switch match state to ENDED by calling end_match.
	if time_left <= 0 and current_state != MatchState.ENDED:
		if multiplayer.is_server():
			end_match_rpc.rpc() # Server tells everyone to end
		else:
			end_match() # For singleplayer

func start_countdown() -> void:
	set_state(MatchState.COUNTDOWN)

func start_match(match_duration: int) -> void:
	set_time(match_duration)
	set_state(MatchState.RUNNING)
	
	# check mode to see if need to start timer or not when countdown ends.
	if mode == MatchMode.SINGLEPLAYER:
		timer.start()

# Note: Multiplayer mode distinction will be defined in a later task. For right now, only handling Singleplayer. 

# New end_match function that will change the match state to ENDED wehen the timer runs out.
func end_match() -> void:
	timer.stop()
	set_state(MatchState.ENDED)
	match_ended.emit()


# _on_game_timer_timeout: Checks if the current state is not running. If false, then set timer to decrement current time (time_left).
func _on_game_timer_timeout() -> void:
	# Now will just check if the match state is anything else other than running.
	if current_state != MatchState.RUNNING:
		return
	
	# In multiplayer make only the server calculate time to prevent drift
	# Make clients ignore local clocks
	if mode == MatchMode.MULTIPLAYER and not multiplayer.is_server():
		return
		
	# decrement time by 1 second.
	set_time(time_left - 1)
	
