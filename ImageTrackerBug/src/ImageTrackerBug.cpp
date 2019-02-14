// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%

#include <time.h>
#include <ml_head_tracking.h>
#include <ml_image_tracking.h>
#include <ml_lifecycle.h>
#include <ml_logging.h>
#include <ml_perception.h>
#include <ml_privileges.h>

static const char APP_TAG[] = "ImageTrackerBug";

#define CHECK(c) do { \
    ML_LOG_TAG(Info, APP_TAG, "%s: Calling...", #c); \
    auto result = (c); \
    if (result != MLResult_Ok) { \
        ML_LOG_TAG(Error, APP_TAG, "    => Failed, returned %d.", (int)result); \
        return -1; \
    } else { \
        ML_LOG_TAG(Info, APP_TAG, "    => Success."); \
    }\
} while (0)

int main() {

    // ==== INITIALIZE

    CHECK(MLLifecycleInit(NULL, NULL));

    CHECK(MLPrivilegesStartup());
    if (MLPrivilegesRequestPrivilege(MLPrivilegeID_CameraCapture) != MLPrivilegesResult_Granted) {
        ML_LOG_TAG(Error, APP_TAG, "CameraCapture privilege is required.");
        return -1;
    }

    MLPerceptionSettings psettings;
    CHECK(MLPerceptionInitSettings(&psettings));
    CHECK(MLPerceptionStartup(&psettings));

    MLHandle head;
    CHECK(MLHeadTrackingCreate(&head));

    MLImageTrackerSettings tsettings;
    CHECK(MLImageTrackerInitSettings(&tsettings));
    tsettings.enable_image_tracking = true;
    tsettings.max_simultaneous_targets = 1;

    MLHandle tracking;
    CHECK(MLImageTrackerCreate(&tsettings, &tracking));

    CHECK(MLLifecycleSetReadyIndication());

    // ==== SHUTDOWN

    // Uncomment this to add a delay, which should eliminate the issue.
    //timespec t = { 3, 0 };
    //nanosleep(&t, NULL);

    CHECK(MLImageTrackerDestroy(tracking));
    CHECK(MLHeadTrackingDestroy(head));
    CHECK(MLPerceptionShutdown());
    ML_LOG_TAG(Info, APP_TAG, "Finished.");

}
