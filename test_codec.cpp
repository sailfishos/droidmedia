#include "droidmedia.h"
#include "droidmediacodec.h"
#include <stdio.h>
#include <unistd.h>

void print_codec_info(size_t index, const char *name) {
    printf("Codec %d is %s (%s)\n", index, name, droid_media_codec_is_encoder(index) ? "encoder" : "decoder");

    ssize_t size = 0;
    char **types = droid_media_codec_get_supported_types (index, &size);
    if (!types) {
        printf("  no supported types\n");
        return;
    }

    printf("  supported types (%li):\n", size);
    for (ssize_t i = 0; i < size; i++) {
        printf("    %li. %s\n", i, types[i]);
        free(types[i]);
    }

    free(types);

    uint32_t *profiles, *levels, *color_formats;
    ssize_t profiles_levels_size, color_formats_size;

    if (!droid_media_codec_get_capabilities(index, name, &profiles, &levels, &profiles_levels_size,
                                            &color_formats, &color_formats_size)) {
        printf("  no capabilities\n");
        return;
    }

    if (color_formats_size > 0) {
        printf("  color formats (%li): ", color_formats_size);
        for (ssize_t i = 0; i < color_formats_size; i++) {
            printf("0x%x ", color_formats[i]);
        }

        free(color_formats);
        printf("\n");
    }

    if (profiles_levels_size > 0) {
        printf("  profile\tlevel:\n");
        for (ssize_t i = 0; i < profiles_levels_size; i++) {
            printf("  0x%x\t0x%x\n", profiles[i], levels[i]);
        }

        free(profiles);
        free(levels);
    }
}

int main(int argc, char *argv[]) {
    droid_media_init();

    size_t codecs = droid_media_codec_count();

    printf("Codecs: %d\n", codecs);

    for (size_t x = 0; x < codecs; x++) {
        const char *name = droid_media_codec_get_name(x);
        print_codec_info(x, name);
    }

    return 0;
}
