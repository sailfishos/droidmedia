#ifndef DROID_MEDIA_CODEC_H
#define DROID_MEDIA_CODEC_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

ssize_t droid_media_codec_find_by_type(const char *type, bool encoder);
ssize_t droid_media_codec_find_by_name(const char *name);
size_t droid_media_codec_count();
const char *droid_media_codec_get_name(size_t index);
bool droid_media_codec_is_encoder(size_t index);
bool droid_media_codec_has_quirk(size_t index, const char *quirkName);
char **droid_media_codec_get_supported_types(size_t index, ssize_t *size);
bool droid_media_codec_get_capabilities(size_t index, const char *type,
                                        uint32_t *profiles, uint32_t *levels, ssize_t *profiles_levels_size,
                                        uint32_t *color_formats, ssize_t *color_formats_size);

#ifdef __cplusplus
};
#endif

#endif /* DROID_MEDIA_CODEC_H */
