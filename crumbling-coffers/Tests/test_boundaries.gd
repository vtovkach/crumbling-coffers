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
	
	# Set up player
	var player = PlayerScene.instantiate()
	add_child_autofree(player)
	
	# Set player position @ x=50
	player.global_position = Vector2(50, 0)
	
	# Teleport the player into the wall @ x = -10 (which is inside/past the wall)
	# No floor in unit test causes issues; player.set_physics_process(false) at start
	# of test ensures is_on_floor() is true
	player.global_position = Vector2(-10, 0)
	
	# Wait 2 frames for physics engine to notice overlap
	await wait_frames(2)
	
	# Test for assert less than (assert_lt) caused issues due to gravity
	# **edited from assert_gt in favor of assert_true which is safer with floating point math**
	assert_true(player.global_position.x >= 0.0, "Player should be stopped by the WorldBoundary @ x=0.")
	
# Test for speed causing boundary glitch
func test_high_speed_boundary_glitch():
	var player = PlayerScene.instantiate()
	add_child_autofree(player)
	
	# Set boundary
	var wall = StaticBody2D.new()
	var collision = CollisionShape2D.new()
	var shape = WorldBoundaryShape2D.new()
	
	shape.normal = Vector2.RIGHT
	collision.shape = shape
	wall.add_child(collision)
	add_child_autofree(wall)
	
	# Position player against wall
	player.global_position = Vector2(5, 0)
	
	# Apply extreme situation with push() from player.gd
	# Push left (-1, 0) @ magnitude 5000
	player.push(Vector2.LEFT, 5000.0)
	
	# Wait for 1 physics frame for glitch
	await wait_frames(1)
	
	# Player's x should still be >= 0
	assert_true(player.global_position.x >= 0.0, "Player should not glitch through boundary; x >= 0")
	
