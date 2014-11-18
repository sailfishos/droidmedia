#include "droidmedia.h"
#include "droidmediacodec.h"
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    droid_media_init();

    size_t codecs = droid_media_codec_count();

    printf("Codecs: %d\n", codecs);

    for (size_t x = 0; x < codecs; x++) {
        const char *name = droid_media_codec_get_name(x);

        printf("Codec %d is %s (%s)\n", x, name, droid_media_codec_is_encoder(x) ? "encoder" : "decoder");

        ssize_t size = 0;
        char **types = droid_media_codec_get_supported_types (x, &size);
        if (!types) {
            printf("  no supported types\n");
        } else {
            printf("  supported types (%li):\n", size);
            ssize_t i;
            for (i = 0; i < size; i++) {
                printf("    %li. %s\n", i, types[i]);
                free(types[i]);
            }
            free(types);
        }
    }

    
    return 0;
}
