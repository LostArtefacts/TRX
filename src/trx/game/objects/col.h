#include <trx/game/collision.h>

void Object_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
void Object_Collision_Trap(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
void Object_Collision_BoxTrap(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll, int32_t damage);
void Object_Collision_SphereTrap(
    int16_t item_num, ITEM *lara_item, COLL_INFO *coll, int32_t damage,
    int32_t deadly_mesh_bits, bool ignore_first_sphere);
