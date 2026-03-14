extends GutTest

var PlayerScene = load("res://Scenes/Player/player.tscn")
# Path to crystal
var CrystalItemScene = load("res://Scenes/items/Common Item.tscn")
# Path to star
var StarItemScene = load("res://Scenes/items/test_item.tscn")

func test_crystal_removed_on_collision():
	# Setup testing subjects
	var player = PlayerScene.instantiate()
	var crystal = CrystalItemScene.instantiate()
	
	add_child_autofree(player)
	add_child_autofree(crystal)
	
	# Overlap player and crystal to force collision
	var collection_point = Vector2(200, 200)
	player.global_position = collection_point
	crystal.global_position = collection_point
	
	# Wait a couple physics frames for body_entered to process
	await wait_physics_frames(2)
	
	# Check if item is gone
	# is_instance_valid(item) returns false if freed
	assert_false(is_instance_valid(crystal), "Crystal item should be freed after player overlap.")
	
	# Check score on player
	assert_true(player.score > 0, "Player score should increase when item is collected.")
	
func test_star_ability_trigger():
	var player = PlayerScene.instantiate()
	var star = StarItemScene.instantiate()
	
	add_child_autofree(player)
	add_child_autofree(star)
	
	# Overlap player and star to force collision
	star.global_position = Vector2(300, 300)
	player.global_position = Vector2(300, 300)
	
	# Wait a couple physics frames for body_entered to process
	await wait_physics_frames(2)
	
	# Verify the star is collected
	assert_false(is_instance_valid(star), "Star item should be freed after player overlap.")
	
	# TO-DO: DEFINE ABILITY AND ADD ASSERTION THAT ABILITY IS ACTIVE
	
