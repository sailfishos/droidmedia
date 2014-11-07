#include "droidmediacamera.h"
#include <stdio.h>
#include <unistd.h>

int main(int args, char *argv[]) {
    droid_media_camera_init();

    int cameras = droid_media_camera_get_number_of_cameras();
    printf("%d cameras\n", cameras);

    for (int x = 0; x < cameras; x++) {
        DroidMediaCameraInfo info;

        if (!droid_media_camera_get_info(&info, x)) {
            printf("Failed to get camera %d info\n", x);
        }

        printf("Camera %d: Orientation: %d, facing: %d\n", x, info.orientation, info.facing);

        printf("Initializing camera %d\n", x);
        DroidMediaCamera *cam = droid_media_camera_connect(x);
        if (!cam) {
            printf("Failed\n");
            continue;
        }

        printf("Starting\n");

        if (!droid_media_camera_lock(cam)) {
            printf("Failed to lock camera\n");
            continue;
        }

        if (!droid_media_camera_start_preview(cam)) {
            printf("Failed\n");
            droid_media_camera_disconnect(cam);
            continue;
        }

        printf("Started\n");
        sleep(2);

        printf("Stopping\n");
        droid_media_camera_stop_preview(cam);
        droid_media_camera_unlock(cam);
        printf("Stopped\n");
        droid_media_camera_disconnect(cam);
    }

    droid_media_camera_deinit();
}
