#include <trx/game/inject.h>
#include <trx/game/objects.h>

static void M_ObjectTypeEdits(
    const INJECTION_CONTEXT *const ctx, const INJECTION *const injection,
    const int32_t data_count)
{
    for (int32_t i = 0; i < data_count; i++) {
        const INJECTION_OBJECT_INFO base_obj_info =
            Inject_ReadObjectPtr(injection);
        const INJECTION_OBJECT_INFO target_obj_info =
            Inject_ReadObjectPtr(injection);

        OBJECT *const base_obj = Object_TryGet(base_obj_info.id);
        const OBJECT *const target_obj = Object_TryGet(target_obj_info.id);
        if (base_obj == nullptr || target_obj == nullptr) {
            continue;
        }
        base_obj->setup_func = target_obj->setup_func;
    }
}

static void M_ObjectLinkEdits(
    const INJECTION_CONTEXT *const ctx, const INJECTION *const injection,
    const int32_t data_count)
{
    for (int32_t i = 0; i < data_count; i++) {
        const INJECTION_OBJECT_INFO base_obj_info =
            Inject_ReadObjectPtr(injection);
        const INJECTION_OBJECT_INFO source_obj_info =
            Inject_ReadObjectPtr(injection);

        OBJECT *const base_obj = Object_TryGet(base_obj_info.id);
        const OBJECT *const source_obj = Object_TryGet(source_obj_info.id);
        if (base_obj == nullptr || source_obj == nullptr
            || !source_obj->loaded) {
            continue;
        }

        base_obj->frame_base = source_obj->frame_base;
        base_obj->frame_ofs = source_obj->frame_ofs;
        base_obj->anim_idx = source_obj->anim_idx;
        base_obj->anim_count = source_obj->anim_count;
        base_obj->bone_idx = source_obj->bone_idx;
        base_obj->mesh_idx = source_obj->mesh_idx;
        base_obj->mesh_count = source_obj->mesh_count;
        base_obj->loaded = true;
    }
}

REGISTER_INJECT_EDITOR(IDT_OBJ_TYPE_EDITS, M_ObjectTypeEdits)
REGISTER_INJECT_EDITOR(IDT_OBJ_LINK_EDITS, M_ObjectLinkEdits)
