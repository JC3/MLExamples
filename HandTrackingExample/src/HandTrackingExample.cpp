// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// The author of this example is not affiliated with Magic Leap, and 
// this is not an official example. It is provided entirely as-is, and
// might catch your headset on fire, etc.
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%

#include <runtime/external/glm/glm.hpp>
#include <runtime/external/glm/gtx/quaternion.hpp>
#include <runtime/external/glm/gtx/transform.hpp>
#include <runtime/external/glm/gtc/type_ptr.hpp>

#include <ml_hand_tracking.h>
#include <ml_graphics.h>
#include <ml_input.h>
#include <ml_lifecycle.h>
#include <ml_logging.h>
#include <ml_perception.h>
#include <ml_privileges.h>

#include "SystemGraphicsContext.h"


const char APP_TAG[] = "mlx.handtracking";

const MLPrivilegeID REQUIRED_PRIVILEGES[] = {
    MLPrivilegeID_WorldReconstruction,
    MLPrivilegeID_LowLatencyLightwear,
    MLPrivilegeID_GesturesConfig,
    MLPrivilegeID_GesturesSubscribe,
    MLPrivilegeID_Invalid
};

/** Quick and dirty error checking macro to minimize distractions in the example
 *  code below. Best practice would be to unwind and shutdown any modules that
 *  have been initialized when a failure occurs. This example does not do that. */
#define CHECK(c) do { \
    auto result = (c); \
    if (result != MLResult_Ok) { \
        ML_LOG_TAG(Error, APP_TAG, "%s failed (%d).", #c, (int)result); \
        return -1; \
    } \
} while (0)

/** Converts an MLTransform to a GLM matrix. */
static glm::mat4 mlToGL(const MLTransform &ml) {

    glm::quat q;
    q.w = ml.rotation.w;
    q.x = ml.rotation.x;
    q.y = ml.rotation.y;
    q.z = ml.rotation.z;

    return glm::translate(glm::mat4(), glm::vec3(ml.position.x, ml.position.y, ml.position.z)) * glm::toMat4(q);

}

#include <list>
#include <map>

using std::list;
using std::map;

struct Hand {
    void updateFrame (MLSnapshot *snapshot, const MLKeyPointState *states, const MLHandTrackingHandState &hstate);
    list<MLVec3f> frame () const;
    map<int,MLVec3f> framemap;
    MLVec3f center;
};

void Hand::updateFrame (MLSnapshot *snapshot, const MLKeyPointState *states, const MLHandTrackingHandState &hstate) {

    for (int k = 0; k < MLHandTrackingStaticData_MaxKeyPoints; ++ k) {
        MLTransform transform;
        if (hstate.keypoints_mask[k] && MLSnapshotGetTransform(snapshot, &(states[k].frame_id), &transform) == MLResult_Ok)
            framemap[k] = transform.position;
    }

    center = hstate.hand_center_normalized;

    ML_LOG_TAG(Info, APP_TAG, "%d %f %d", (int)framemap.size(), hstate.hand_confidence, hstate.keypose);

}

list<MLVec3f> Hand::frame () const {

    list<MLVec3f> result;
    for (auto i = framemap.cbegin(); i != framemap.cend(); ++i)
        result.push_back(i->second);
    return result;

}


void drawHand (const Hand &h) {

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_LIGHTING);

    glPointSize(3);
    glBegin(GL_POINTS);
    glColor3f(1, 1, 1);
    list<MLVec3f> frame = h.frame();
    for (auto v = frame.begin(); v != frame.end(); ++v)
        glVertex3f(v->x, v->y, v->z);
    glColor3f(1, 1, 0);
    glVertex3f(h.center.x, h.center.y, h.center.z);
    glEnd();

    glLineWidth(2);
    glBegin(GL_LINES);
    glColor3f(1, 0, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(0.1, 0, 0);
    glColor3f(0, 1, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0.1, 0);
    glColor3f(0, 0, 1);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, 0.1);
    glEnd();

    glLineWidth(1);
    glBegin(GL_LINES);
    glColor3f(0, 1, 1);
    glVertex3f(0, 0, 0);
    glVertex3f(h.center.x, h.center.y, h.center.z);
    glEnd();

}


//-----------------------------------------------------------------------------

int main () {

    // ==== INITIALIZE ========================================================
    
    // ---- Lifecycle

    ML_LOG_TAG(Info, APP_TAG, "Initializing...");
    CHECK(MLLifecycleInit(NULL, NULL));

    // ---- Privileges

    CHECK(MLPrivilegesStartup());
    for (auto p = REQUIRED_PRIVILEGES; *p != MLPrivilegeID_Invalid; ++ p)
        if (MLPrivilegesRequestPrivilege(*p) != MLPrivilegesResult_Granted) {
            ML_LOG_TAG(Error, APP_TAG, "Privilege %d denied.", *p);
            return -1;
        }

    // ---- Perception

    MLPerceptionSettings perception_settings;
    CHECK(MLPerceptionInitSettings(&perception_settings));
    CHECK(MLPerceptionStartup(&perception_settings));

    // ---- Hand Tracking

    MLHandle hand;
    MLHandTrackingStaticData hand_sdata;
    MLHandTrackingConfiguration hand_config;
    CHECK(MLHandTrackingCreate(&hand));
    CHECK(MLHandTrackingGetStaticData(hand, &hand_sdata));
    
    CHECK(MLHandTrackingGetConfiguration(hand, &hand_config));
    hand_config.handtracking_pipeline_enabled = true;
    hand_config.keypoints_filter_level = MLKeypointFilterLevel_2;
    for (int k = 0; k < MLHandTrackingKeyPose_Count - 1; ++k)
        hand_config.keypose_config[k] = true;
    CHECK(MLHandTrackingSetConfiguration(hand, &hand_config));

    // ---- Controller buttons

    MLHandle input;
    CHECK(MLInputCreate(NULL, &input));

    // ---- Graphics

    SystemGraphicsContext graphics_context; // constructor inits a context
    graphics_context.makeCurrent();

    // A GL context *must* be current for this to succeed!     
    if (!gladLoadGLLoader(graphics_context.loader())) {
        ML_LOG_TAG(Error, APP_TAG, "GL loader failed.");
        return -1;
    }

    // We're responsible for creating the render target framebuffer.
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);

    // This call list is just used 'cause we render the scene multiple times.
    GLuint calllist = glGenLists(1); 

    // In the SystemGraphicsContext constructor, I tried to make the requested
    // surface format match the one specified here as closely as possible. I am
    // not sure what the exact connection is between the two formats, though.
    // This is an open question.
    MLGraphicsOptions graphics_options = {};
    graphics_options.color_format = MLSurfaceFormat_RGBA8UNormSRGB;
    graphics_options.depth_format = MLSurfaceFormat_D32Float; // no stencil here
    graphics_options.graphics_flags = MLGraphicsFlags_Default;

    MLHandle graphics = ML_INVALID_HANDLE;
    CHECK(MLGraphicsCreateClientGL(&graphics_options, graphics_context.context(), &graphics));

    // ---- Finalize

    CHECK(MLLifecycleSetReadyIndication());
    ML_LOG_TAG(Info, APP_TAG, "Ready! Press bumper to exit.");

    // ==== MAIN LOOP =========================================================

    Hand lhand, rhand;
    bool quit = false; // loop exits when true

    // This will run until bumper button is pressed on any controller. BUG: If
    // the controller is powered on after this app starts, sometimes it won't
    // respond to button presses and must be forcefully terminated. Will be 
    // fixed when I figure out how to best detect and recover from that.
    do {

        MLSnapshot *snapshot;
        CHECK(MLPerceptionGetSnapshot(&snapshot));

        MLHandTrackingData hand_data;
        CHECK(MLHandTrackingGetData(hand, &hand_data));
        lhand.updateFrame(snapshot, hand_sdata.left_frame, hand_data.left_hand_state);
        rhand.updateFrame(snapshot, hand_sdata.right_frame, hand_data.right_hand_state);

        CHECK(MLPerceptionReleaseSnapshot(snapshot));

        // ---- RENDER SCENE

        MLGraphicsFrameParams frame_params;
        CHECK(MLGraphicsInitFrameParams(&frame_params));
        frame_params.near_clip = 0.1f;
        frame_params.far_clip = 100.0f;
        frame_params.focus_distance = 1.0f;
        frame_params.projection_type = MLGraphicsProjectionType_SignedZ;
        frame_params.protected_surface = false;
        frame_params.surface_scale = 1.0f;

        MLHandle frame;
        MLGraphicsVirtualCameraInfoArray cameras;
        MLResult frame_result = MLGraphicsBeginFrame(graphics, &frame_params, &frame, &cameras);

        if (frame_result == MLResult_Ok) {

            /* How to render things (fixed function pipeline version):
             *
             * The scene must be rendered for each virtual camera (left eye, right eye).
             *
             * The MLGraphicsVirtualCameraInfoArray structure contains all the info
             * needed to set up the viewport, projection, and modelview matrices for
             * each camera, and implicitly encapsulates things like the head position,
             * visual calibration, etc.
             *
             * - Viewport: Common to all cameras and specified in .viewport.
             * - Projection: Directly specified for each camera in .projection.
             * - Modelview: The inverse of the camera transform for each camera.
             *
             * Rendering to each camera is accomplished by rendering to a specific layer
             * in a multi-layered texture created by the API. Those texture names are
             * given to us in .color_id and .depth_id, and the layer number corresponding
             * to each camera is the integer name of the camera (note: this may not be the
             * same as the index in the camera array so always use .virtual_camera_name!).
             */

            for (int k = 0; k < MLGraphicsVirtualCameraName_Count; ++k) {

                // Set render target to the appropriate texture layer for this camera.
                const MLGraphicsVirtualCameraInfo &camera = cameras.virtual_cameras[k];
                glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cameras.color_id, 0, camera.virtual_camera_name);
                glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cameras.depth_id, 0, camera.virtual_camera_name);

                // Viewport is common to all cameras and is given.
                glViewport((int)(cameras.viewport.x + 0.5f), (int)(cameras.viewport.y + 0.5f), 
                           (GLsizei)(cameras.viewport.w + 0.5f), (GLsizei)(cameras.viewport.h + 0.5f));

                // Projection matrix is given directly for each camera.
                glMatrixMode(GL_PROJECTION);
                glPushMatrix();
                glLoadMatrixf(camera.projection.matrix_colmajor);

                // Modelview is inverse of transform for each camera. GLM simplifies this.
                glMatrixMode(GL_MODELVIEW);
                glPushMatrix();
                glLoadMatrixf(glm::value_ptr(glm::inverse(mlToGL(camera.transform))));

                // Now we can draw things in the reality coordinate frame. Also, this example
                // renders once to a call list and uses that for remaining cameras.
                if (k == 0) {
                    glNewList(calllist, GL_COMPILE_AND_EXECUTE);
                    //drawPlanes(query_results, query_nresults);
                    drawHand(lhand);
                    drawHand(rhand);
                    glEndList();
                } else {
                    glCallList(calllist);
                }

                glMatrixMode(GL_MODELVIEW);
                glPopMatrix();
                glMatrixMode(GL_PROJECTION);
                glPopMatrix();
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                // Signal that we're done rendering this camera. MLGraphicsEndFrame will
                // fail if you don't trigger the signal for every camera, so even if your
                // app, say, only runs in the left eye, you still need to signal everything.
                MLGraphicsSignalSyncObjectGL(graphics, cameras.virtual_cameras[k].sync_object);

            }

            // End frame must match begin frame.
            CHECK(MLGraphicsEndFrame(graphics, frame));
            graphics_context.swapBuffers();

        } else if (frame_result != MLResult_Timeout) { // sometimes it fails with timeout when device is busy

            ML_LOG_TAG(Error, APP_TAG, "MLGraphicsBeginFrame failed with %d", frame_result);
            quit = true;

        }

        // ---- CHECK CONTROLLER BUTTONS
        // Poll controllers for bumper button presses. The other way to do this
        // is to use MLInputSetControllerCallbacks for a callback-based 
        // approach. Polling serves our purpose here though.

        MLInputControllerState input_states[MLInput_MaxControllers];
        CHECK(MLInputGetControllerState(input, input_states));

        for (int k = 0; k < MLInput_MaxControllers; ++k) {
            if (input_states[k].button_state[MLInputControllerButton_Bumper]) {
                ML_LOG_TAG(Info, APP_TAG, "Bye!");
                quit = true;
                break;
            }
        }

    } while (!quit);

    // ==== CLEANUP ===========================================================

    // Just the opposite order of initialization.
    glDeleteLists(calllist, 1);
    glDeleteFramebuffers(1, &framebuffer);
    MLGraphicsDestroyClient(&graphics);
    graphics_context.cleanup();
    MLInputDestroy(input);
    MLHandTrackingDestroy(hand);
    MLPerceptionShutdown();

    ML_LOG_TAG(Info, APP_TAG, "Finished.");

}
