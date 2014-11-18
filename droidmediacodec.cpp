#include <media/stagefright/MediaCodecList.h>

extern "C" {

ssize_t droid_media_codec_find_by_type(const char *type, bool encoder)
{
    return android::MediaCodecList::getInstance()->findCodecByType(type, encoder);
}

ssize_t droid_media_codec_find_by_name(const char *name)
{
    return android::MediaCodecList::getInstance()->findCodecByName(name);
}

size_t droid_media_codec_count()
{
    return android::MediaCodecList::getInstance()->countCodecs();
}

const char *droid_media_codec_get_name(size_t index)
{
    return android::MediaCodecList::getInstance()->getCodecName(index);
}

bool droid_media_codec_is_encoder(size_t index)
{
    return android::MediaCodecList::getInstance()->isEncoder(index);
}

bool droid_media_codec_has_quirk(size_t index, const char *quirkName)
{
    return android::MediaCodecList::getInstance()->codecHasQuirk(index, quirkName);
}

char **droid_media_codec_get_supported_types(size_t index, ssize_t *size)
{
    android::Vector<android::AString> types;
    if (android::MediaCodecList::getInstance()->getSupportedTypes(index, &types) != android::OK) {
        return NULL;
    }

    char **out = (char **)malloc(types.size() + 1);
    for (size_t x = 0; x < types.size(); x++) {
        android::AString str = types[x];
        out[x] = (char *)malloc(str.size() + 1);
        memcpy(out[x], str.c_str(), str.size());
        out[x][str.size()] = '\0';
    }

    out[types.size()] = NULL;

    *size = types.size();

    return out;
}

bool droid_media_codec_get_capabilities(size_t index, const char *type,
                                        uint32_t *profiles, uint32_t *levels, ssize_t *profiles_levels_size,
                                        uint32_t *color_formats, ssize_t *color_formats_size)
{
    android::Vector<android::MediaCodecList::ProfileLevel> profileLevels;
    android::Vector<uint32_t> colorFormats;
    if (android::MediaCodecList::getInstance()->getCodecCapabilities(index, type,
                                                                     &profileLevels,
                                                                     &colorFormats) != android::OK) {
        return false;
    }

    *profiles_levels_size = profileLevels.size();
    profiles = (uint32_t *)malloc(*profiles_levels_size * sizeof(uint32_t));
    levels = (uint32_t *)malloc(*profiles_levels_size * sizeof(uint32_t));
    for (ssize_t x = 0; x < *profiles_levels_size; x++) {
        profiles[x] = profileLevels[x].mProfile;
        levels[x] = profileLevels[x].mLevel;
    }

    *color_formats_size = colorFormats.size();
    color_formats = (uint32_t *)malloc(*color_formats_size * sizeof(uint32_t));
    for (ssize_t x = 0; x < *color_formats_size; x++) {
        color_formats[x] = colorFormats[x];
    }

    return true;
}

};
