@abstract
class_name State
extends Node

# Enter state, update state, exit state cycle
@abstract func enter()
@abstract func exit()
@abstract func update(delta: float)
@abstract func update_physics(delta: float)
	
