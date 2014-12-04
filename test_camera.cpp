/*
 * Copyright (C) 2014 Jolla Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Authored by: Mohammed Hassan <mohammed.hassan@jolla.com>
 */

#include "droidmediacamera.h"
#include <stdio.h>
#include <unistd.h>

int main(int args, char *argv[]) {
    droid_media_init();

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

    droid_media_deinit();
}
