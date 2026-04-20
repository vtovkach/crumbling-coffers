extends PickupBase
class_name ItemPickup

enum PickupKind {
	COLLECTIBLE,
	HOTBAR
}

@export var pickup_kind: PickupKind = PickupKind.COLLECTIBLE
@export var inventory_item: InventoryItem
@export var hotbar_item: HotbarItem

func _ready() -> void:
	super()

func apply_to_player(player: Player) -> bool:
	if points != 0 and player.has_method("add_score"):
		player.add_score(points)

	match pickup_kind:
		PickupKind.COLLECTIBLE:
			if inventory_item != null and player.has_method("collect"):
				player.collect(inventory_item)
				return true
		PickupKind.HOTBAR:
			if hotbar_item != null and player.has_method("consumable_collect"):
				player.consumable_collect(hotbar_item)
				return true
	return false

func on_collected(body: Node) -> bool:
	if not body.is_in_group("player"):
		return false
	if not body.has_method("receive_pickup"):
		return false

	return body.receive_pickup(self)
