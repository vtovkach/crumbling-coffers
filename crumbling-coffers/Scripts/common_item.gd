extends Area2D

var value := 5

func _on_area_2d_body_entered(body: Node2D) -> void:
	print("Common Item Collected.")
	#update score +5 points
	queue_free()

	
