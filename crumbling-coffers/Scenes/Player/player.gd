# 	Does not have multiplayer support yet;
#	Need to differentiate between client player	
#	and other connected players in same room 

extends CharacterBody2D

# Most of these values are placeholder.
const MAX_SPEED = 1000.0
const MAX_RUNSPEED = 600.0

const decel = 500
const accel = 1000
const braking_decel = 2000

func _physics_process(delta: float) -> void:

	# Update velocity
#	move(direction)		Direction given by player input
		
	# Update position
	move_and_slide()


# Update velocity according to direction of movement
# direction: a float in [-1, 1]
# [-1, 0): 	To left.
# 0: 		Stop 
# (0, 1]:	To right.
func move(direction: float, delta: float) -> void:
	if direction:
		if (velocity.x > 0 and direction < 0) or (velocity.x < 0 and direction > 0):
			brake(delta)
		else:
			velocity.x = move_toward(velocity.x, direction * MAX_RUNSPEED, accel * delta)
	else:
		velocity.x = move_toward(velocity.x, 0, decel * delta)
	
# When acceleration direction is against current velocity
# May prefer a state to handle this, especially if custom animation
func brake(delta: float) -> void:
	velocity.x = move_toward(velocity.x, 0, braking_decel * delta)
	
		
# Push this character in a direction, may exceed their max runspeed (but not their max speed)
# Adds to initial velocity
# Consider for external force pushing character
func push(direction: float, pushStrength: float) -> void:
	velocity.x = move_toward(velocity.x, direction * MAX_SPEED, pushStrength);
