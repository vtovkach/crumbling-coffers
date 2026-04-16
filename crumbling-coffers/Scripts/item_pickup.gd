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

func apply_to_player(player: Player) -> void:
	if points != 0:
		player.add_score(points)

	match pickup_kind:
		PickupKind.COLLECTIBLE:
			if inventory_item != null:
				player.collect(inventory_item)
		PickupKind.HOTBAR:
			if hotbar_item != null:
				player.consumable_collect(hotbar_item)

func on_collected(body: Node) -> void:
	if not body.is_in_group("player"):
		return

	body.receive_pickup(self)
