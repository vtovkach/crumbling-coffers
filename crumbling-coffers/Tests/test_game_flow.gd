extends GutTest

# Must call biome scene to test specific signal handlers and pause logic
var MossyBiome = load("res://Scenes/Maps/mossy_biome.tscn")

func test_timer_accuracy_start():
	var test_duration = 60
	MatchManager.start_match(test_duration)
	
	# Verify match starts with correct time
	# Confirms match is live and variable was initialized correctly
	assert_eq(MatchManager.time_left, test_duration, "time_left should match start_match.")
	assert_true(MatchManager.time_left > 0, "Timer should be positive.")

func test_timer_decrement():
	# Start at 10 seconds remaining
	MatchManager.start_match(10)
	var initial_time = MatchManager.time_left
	
	# Manually trigger timeout func that decrements time to avoid waiting on test
	MatchManager._on_game_timer_timeout()
	
	# Assert correct math
	assert_eq(MatchManager.time_left, initial_time-1, "remaining_time should decrease by exactly 1.")

func test_timer_boundary():
	# Start at 1 second remaining
	MatchManager.set_time(1)
	
	# Trigger timeout func that decrements time to zero
	MatchManager._on_game_timer_timeout()
	
	# Asserts to ensure stops timer
	assert_eq(MatchManager.time_left, 0, "Timer should stop at exactly 0.")
	assert_eq(MatchManager.current_state, MatchManager.MatchState.ENDED, "Match state should transition to ended @ zero.")

func test_unpause_after_transition():
	# Set scene & add to tree so _ready() runs and $Player/$HUD found
	var biome = MossyBiome.instantiate()
	add_child_autofree(biome)
	
	# Simulate pause in last 2 seconds disabling mm button in score page
	get_tree().paused = true
	assert_true(get_tree().paused, "Game should start in a paused state for test.")
	
	# Trigger match end ie MatchManager emitting signal
	biome._on_match_ended()
	
	# Await might cause tree to remain paused when it should be
	# Await until after
	await wait_seconds(2.1)
	
	# Verify on score page
	var current_scene = get_tree().current_scene
	assert_eq(current_scene.name, "ScorePage", "Should have transitioned scenes.")
	
	# Fix should make pause false before transition to next scene
	assert_false(get_tree().paused, "The SceneTree should be unpaused by _on_match_ended to allow interaction.")
	
	# Unpause at end of test
	get_tree().paused = false
	
