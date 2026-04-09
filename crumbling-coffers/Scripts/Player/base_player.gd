# Player's base clase. Will only contain information that is essential to being a displayable player in the game

extends CharacterBody2D
class_name Player

# Position, velocity already given by GODOT parent classes. Position is especially important

# Player should also own some variables for rendering, i.e. direction and state.
# Technically this is better as interface, but ALL players will be renderable. So this is appropriate.
@export var camera: Camera2D	# All players have a camera, making it exist in code allows selecting specific camera (feature enabling) 
@export var direction: float = 0
var state: PlayerState
#var animationState Needs to be implemented and added in Animation task

# Player's state variable is not adjusted within player. This method will be used to adjust it from external source
# Note that PlayerState logic is NOT applied to base_player class. 
func setState(s: PlayerState) -> void:
	state = s
