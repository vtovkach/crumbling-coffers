extends Panel

# will add future textures that will make use of the variable "bgSprite". its main use would be to switch textures 
# within the specific sprite frame.
#@onready var bgSprite: Sprite2D = $background
@onready var itemVisual: Sprite2D = $CenterContainer/Panel/item_display


func update(slot: InventorySlot):
	if !slot.item:
		itemVisual.visible = false
	else:
		itemVisual.visible = true
		itemVisual.texture = slot.item.texture
