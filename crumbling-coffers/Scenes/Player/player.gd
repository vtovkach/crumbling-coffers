# 	Does not have multiplayer support yet;
#	Need to differentiate between client player	
#	and other connected players in same room 

extends CharacterBody2D

# Most of these values are placeholder.
const MAX_SPEED = 1000.0
const MAX_RUNSPEED = 300.0

const decel = 10;
const accel = 20;

func _physics_process(delta: float) -> void:

	# Update velocity
	move(direction)
		
	# Update position
	move_and_slide()


# Update velocity according to direction of movement
# direction: a float in [-1, 1]
# [-1, 0): 	To left.
# 0: 		Stop 
# (0, 1]:	To right.
func move(direction: float) -> void:
	if direction:
		velocity.x = move_toward(velocity.x, direction * MAX_RUNSPEED, accel)
	else:
		velocity.x = move_toward(velocity.x, 0, decel)
		
		
# Push this character in a direction, may exceed their max runspeed (but not their max speed)
# Adds to initial velocity
# Consider for external force pushing character
func push(direction: float, pushStrength: float) -> void:
	velocity.x = move_toward(velocity.x, direction * MAX_SPEED, pushStrength);
