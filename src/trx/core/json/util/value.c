#include <trx/core/json/util/value.h>

#include <trx/core/enum_map.h>
#include <trx/core/strings.h>

#include <stdio.h>

void JSONValue_Write(
    JSON_OBJECT *const obj, const char *const key, const TRX_VALUE_TYPE type,
    const void *const param, const TRX_VALUE *const value)
{
    switch (type) {
    case TVT_BOOL:
        JSON_ObjectAppendBool(obj, key, value->as_bool);
        break;

    case TVT_S8:
    case TVT_U8:
    case TVT_S16:
    case TVT_U16:
    case TVT_S32:
    case TVT_U32:
        JSON_ObjectAppendInt64(obj, key, value->as_int);
        break;

    case TVT_FLOAT:
    case TVT_DOUBLE:
        JSON_ObjectAppendDouble(obj, key, value->as_num);
        break;

    case TVT_XYZ_16:
    case TVT_XYZ_32: {
        JSON_OBJECT *const vec = JSON_ObjectNew();
        JSON_ObjectAppendInt64(vec, "x", value->as_xyz.x);
        JSON_ObjectAppendInt64(vec, "y", value->as_xyz.y);
        JSON_ObjectAppendInt64(vec, "z", value->as_xyz.z);
        JSON_ObjectAppendObject(obj, key, vec);
        break;
    }

    case TVT_RGB_888: {
        char text[8];
        sprintf(
            text, "#%02X%02X%02X", value->as_rgb.r, value->as_rgb.g,
            value->as_rgb.b);
        JSON_ObjectAppendString(obj, key, text);
        break;
    }

    case TVT_ENUM:
        JSON_ObjectAppendString(
            obj, key, EnumMap_ToString(param, value->as_int));
        break;

    case TVT_STRING:
    case TVT_DYNAMIC_ENUM:
        JSON_ObjectAppendString(
            obj, key, value->as_str != nullptr ? value->as_str : "");
        break;
    }
}

bool JSONValue_ReadFrom(
    const JSON_VALUE *const value, const TRX_VALUE_TYPE type,
    const void *const param, TRX_VALUE *const out)
{
    out->type = type;
    if (value == nullptr) {
        return false;
    }

    switch (type) {
    case TVT_BOOL:
        out->as_bool = JSON_ValueGetBool(value, false);
        return true;

    case TVT_S8:
    case TVT_U8:
    case TVT_S16:
    case TVT_U16:
    case TVT_S32:
    case TVT_U32:
        // A number reads straight; a string is parsed, as a hand-edited config
        // may spell an integer either way.
        if (value->type == JSON_TYPE_NUMBER) {
            out->as_int = JSON_ValueGetInt64(value, 0);
            return true;
        }
        if (value->type == JSON_TYPE_STRING) {
            int32_t parsed;
            if (!String_ParseInteger(JSON_ValueGetString(value, ""), &parsed)) {
                return false;
            }
            out->as_int = parsed;
            return true;
        }
        return false;

    case TVT_FLOAT:
    case TVT_DOUBLE:
        out->as_num = JSON_ValueGetDouble(value, 0.0);
        return true;

    case TVT_XYZ_16:
    case TVT_XYZ_32: {
        const JSON_OBJECT *const vec = JSON_ValueAsObject(value);
        if (vec == nullptr) {
            return false;
        }
        out->as_xyz = (XYZ_32) {
            .x = JSON_ObjectGetInt(vec, "x", 0),
            .y = JSON_ObjectGetInt(vec, "y", 0),
            .z = JSON_ObjectGetInt(vec, "z", 0),
        };
        return true;
    }

    case TVT_RGB_888:
        // A number packs the channels; a string spells them, either the way the
        // dump writes it or as a hand-edited hex value.
        if (value->type == JSON_TYPE_NUMBER) {
            const uint32_t packed = JSON_ValueGetInt(value, 0);
            out->as_rgb = (RGB_888) {
                .r = (packed >> 0) & 0xFF,
                .g = (packed >> 8) & 0xFF,
                .b = (packed >> 16) & 0xFF,
            };
            return true;
        }
        const char *const rgb_text = JSON_ValueGetString(value, nullptr);
        return rgb_text != nullptr
            && String_ParseRGB888(rgb_text, &out->as_rgb);

    case TVT_ENUM: {
        // A name the map does not know leaves the caller to apply its own
        // default; Value_Parse reports that as a miss.
        const char *const text = JSON_ValueGetString(value, nullptr);
        return text != nullptr && Value_Parse(TVT_ENUM, param, text, out);
    }

    case TVT_STRING:
    case TVT_DYNAMIC_ENUM:
        out->as_str = JSON_ValueGetString(value, nullptr);
        return true;
    }
    return false;
}

bool JSONValue_Read(
    const JSON_OBJECT *const obj, const char *const key,
    const TRX_VALUE_TYPE type, const void *const param, TRX_VALUE *const out)
{
    return JSONValue_ReadFrom(JSON_ObjectGetValue(obj, key), type, param, out);
}
