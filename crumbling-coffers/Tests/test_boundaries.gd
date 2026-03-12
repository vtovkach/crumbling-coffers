extends GutTest

# Path to the player scene
var PlayerScene = load("res://Scenes/Player/player.tscn")

# Test verifies player exists physically i.e. has collision shape
func test_player_has_collision():
	# Instantiate player into the test runner's tree
	var player = PlayerScene.instantiate()
	add_child_autofree(player)
	
	# Search for collision shape or polygon
	var collider = player.find_child("*Collision*", true, false)
	
	# Verify that a collider was found & ensure active
	assert_not_null(collider, "Player must have a CollisionShape2D to interact with boundaries.")
	assert_true(collider.disabled == false, "Player collision shape should not be disabled.")

# Now that player verified, simulate WorldBoundary @ x=0 & verify player cannot cross
func test_boundary_stops_player():
	# MAKE BOUNDARY
	# Create StaticBody2D with WorldBoundaryShape2D @ x=0
	var wall = StaticBody2D.new()
	var collision = CollisionShape2D.new()
	var boundary = WorldBoundaryShape2D.new()
	
	# Set  normal to RIGHT (1, 0) makes area to left solid
	# Player remains on right of x=0 line
	boundary.normal = Vector2.RIGHT
	collision.shape = boundary
	wall.add_child(collision)
	
	# Add wall to test tree and set global position to origin
	add_child_autofree(wall)
	wall.global_position = Vector2(0, 0)
	
	# SET UP PLAYER
	var player = PlayerScene.instantiate()
	add_child_autofree(player)
	
	# Set player position @ x=50
	player.global_position = Vector2(50, 0)
	
	# SIMULATE MOVEMENT
	# Press the move key and wait for physics engine to calculate collision
	Input.action_press("left")
	await wait_frames(25)
	Input.action_release("left")
	
	# CONCLUSION
	# Player should be blocked @ x=0
	assert_gt(player.global_position.x, -1.0, "Player should be stopped by the WorldBoundary @ x=0.")
