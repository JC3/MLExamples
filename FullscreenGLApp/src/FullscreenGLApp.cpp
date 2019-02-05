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

#include <chrono>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <glad/glad.h>

#include <runtime/external/glm/glm.hpp>
#include <runtime/external/glm/gtx/quaternion.hpp>
#include <runtime/external/glm/gtx/transform.hpp>
#include <runtime/external/glm/gtc/type_ptr.hpp>

#include "FullscreenGLApp.h"
#include <ml_head_tracking.h>
#include <ml_graphics.h>
#include <ml_input.h>
#include <ml_lifecycle.h>
#include <ml_logging.h>
#include <ml_perception.h>
#include <ml_planes.h>
#include <ml_privileges.h>

static const char APP_NAME[] = "com.mlexamples.fullscreenglapp";

static const MLPrivilegeID REQUIRED_PRIVILEGES[] = {
	MLPrivilegeID_WorldReconstruction,
	MLPrivilegeID_LowLatencyLightwear,
	MLPrivilegeID_Invalid
};

#define CHECK(c) do { \
	auto result = (c); \
	if (result != MLResult_Ok) { \
		ML_LOG(Error, "%s: %s failed (%d).", APP_NAME, #c, (int)result); \
		return -1; \
	} \
} while (0)

// Adaptation of graphics_context_t from SimpleGLApp SDK example.
class SystemGraphicsContext {
private:
	EGLDisplay egl_display;
	EGLContext egl_context;
public:
	SystemGraphicsContext();
	~SystemGraphicsContext() { cleanup(); }
	void cleanup();
	void makeCurrent();
	void swapBuffers();
	void unmakeCurrent();
	MLHandle context() const;
};

SystemGraphicsContext::SystemGraphicsContext() {
	egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	EGLint major = 4;
	EGLint minor = 0;
	eglInitialize(egl_display, &major, &minor);
	eglBindAPI(EGL_OPENGL_API);

	EGLint config_attribs[] = {
	  EGL_RED_SIZE, 8,			// 5/6/5 is another option
	  EGL_GREEN_SIZE, 8,
	  EGL_BLUE_SIZE, 8,
	  EGL_ALPHA_SIZE, 0,
	  EGL_DEPTH_SIZE, 24,
	  EGL_STENCIL_SIZE, 0,		// 8 works for stencil size too if you need it
	  EGL_NONE
	};
	EGLConfig egl_config = nullptr;
	EGLint config_size = 0;
	eglChooseConfig(egl_display, config_attribs, &egl_config, 1, &config_size);

	EGLint context_attribs[] = {
	  EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
	  EGL_CONTEXT_MINOR_VERSION_KHR, 0,
	  EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR, EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR,
	  EGL_NONE
	};
	egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);
}

void SystemGraphicsContext::makeCurrent() {
	eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, egl_context);
}

void SystemGraphicsContext::unmakeCurrent() {
	eglMakeCurrent(NULL, EGL_NO_SURFACE, EGL_NO_SURFACE, NULL);
}

void SystemGraphicsContext::swapBuffers() {
	// buffer swapping is implicit on device (MLGraphicsEndFrame)
}

void SystemGraphicsContext::cleanup() {
	if (egl_display) {
		eglDestroyContext(egl_display, egl_context);
		eglTerminate(egl_display);
		egl_display = NULL;
	}
}

MLHandle SystemGraphicsContext::context() const {
	return (MLHandle)egl_context;
}

glm::mat4 mlToGL(const MLTransform &ml) {

	glm::quat q;
	q.w = ml.rotation.w;
	q.x = ml.rotation.x;
	q.y = ml.rotation.y;
	q.z = ml.rotation.z;

	return glm::translate(glm::mat4(), glm::vec3(ml.position.x, ml.position.y, ml.position.z)) * glm::toMat4(q);

}

void drawPlanes(const MLPlane *p, int np);

int main () {

	// ==== INITIALIZE ========================================================
	
	// ---- Lifecycle
	ML_LOG(Info, "%s: Initializing...", APP_NAME);
	CHECK(MLLifecycleInit(NULL, NULL));

	// ---- Privileges

	CHECK(MLPrivilegesStartup());
	for (auto p = REQUIRED_PRIVILEGES; *p != MLPrivilegeID_Invalid; ++ p)
		if (MLPrivilegesRequestPrivilege(*p) != MLPrivilegesResult_Granted) {
			ML_LOG(Error, "%s: Privilege %d denied.", APP_NAME, *p);
			return -1;
		}

	// ---- Perception

	MLPerceptionSettings perception_settings;
	CHECK(MLPerceptionInitSettings(&perception_settings));
	CHECK(MLPerceptionStartup(&perception_settings));

	// ---- Head Tracking

	MLHandle head_tracking;
	CHECK(MLHeadTrackingCreate(&head_tracking));

	// ---- Plane Tracking

	MLHandle planes;
	CHECK(MLPlanesCreate(&planes));

	// ---- Controller buttons

	MLHandle input;
	CHECK(MLInputCreate(NULL, &input));

	// ---- Graphics

	SystemGraphicsContext graphics_context; // constructor inits a context
	graphics_context.makeCurrent();

	if (!gladLoadGLLoader((GLADloadproc)eglGetProcAddress)) {
		ML_LOG(Error, "%s: GL loader failed.", APP_NAME);
		return -1;
	}

	GLuint framebuffer;
	glGenFramebuffers(1, &framebuffer);

	GLuint calllist = glGenLists(1);

	MLGraphicsOptions graphics_options = {};
	graphics_options.color_format = MLSurfaceFormat_RGBA8UNormSRGB;
	graphics_options.depth_format = MLSurfaceFormat_D32Float; // no stencil here
	graphics_options.graphics_flags = MLGraphicsFlags_Default;

	MLHandle graphics = ML_INVALID_HANDLE;
	CHECK(MLGraphicsCreateClientGL(&graphics_options, graphics_context.context(), &graphics));

	// ---- Finalize

	CHECK(MLLifecycleSetReadyIndication());
	ML_LOG(Info, "%s: Ready! Press bumper to exit.", APP_NAME);

	// ==== MAIN LOOP =========================================================

	MLHandle planes_query = ML_INVALID_HANDLE; // invalid unless query running
	MLPlane query_results[16];
	uint32_t query_nresults = 0;
	bool quit = false; // loop exits when true

	// This will run until bumper button is pressed on any controller.
	do {

		if (planes_query == ML_INVALID_HANDLE) {

			MLSnapshot *snapshot;
			CHECK(MLPerceptionGetSnapshot(&snapshot));

			MLHeadTrackingState ht_state;
			MLHeadTrackingStaticData ht_data;
			MLTransform ht_transform;

			bool ht_valid = (MLHeadTrackingGetState(head_tracking, &ht_state) == MLResult_Ok) &&
				(ht_state.error == MLHeadTrackingError_None) &&
				(ht_state.mode == MLHeadTrackingMode_6DOF) &&
				(ht_state.confidence > 0.9) &&
				(MLHeadTrackingGetStaticData(head_tracking, &ht_data)) &&
				(MLSnapshotGetTransform(snapshot, &ht_data.coord_frame_head, &ht_transform));

			CHECK(MLPerceptionReleaseSnapshot(snapshot));
			
			MLPlanesQuery query = {};
			query.bounds_center = ht_transform.position;
			query.bounds_extents.x = 6;
			query.bounds_extents.y = 6;
			query.bounds_extents.z = 6;
			query.bounds_rotation = ht_transform.rotation;
			query.flags = MLPlanesQueryFlag_AllOrientations | MLPlanesQueryFlag_Semantic_All;
			query.max_results = sizeof(query_results) / sizeof(MLPlane);
			query.min_plane_area = 0.25;

			CHECK(MLPlanesQueryBegin(planes, &query, &planes_query));
			//ML_LOG(Info, "%s: Planes query started.", APP_NAME);

		} 
		
		if (planes_query != ML_INVALID_HANDLE &&
			MLPlanesQueryGetResults(planes, planes_query, query_results, &query_nresults) == MLResult_Ok) 
		{
			planes_query = ML_INVALID_HANDLE;
		}

		MLGraphicsFrameParams frame_params;
		CHECK(MLGraphicsInitFrameParams(&frame_params));
		frame_params.near_clip = 0.1;
		frame_params.far_clip = 100.0;
		frame_params.focus_distance = 1.0;
		frame_params.projection_type = MLGraphicsProjectionType_SignedZ;
		frame_params.protected_surface = false;
		frame_params.surface_scale = 1.0;

		MLHandle frame;
		MLGraphicsVirtualCameraInfoArray cameras;
		MLResult frame_result = MLGraphicsBeginFrame(graphics, &frame_params, &frame, &cameras);

		if (frame_result == MLResult_Ok) {

			for (int k = 0; k < MLGraphicsVirtualCameraName_Count; ++k) {

				const MLGraphicsVirtualCameraInfo &camera = cameras.virtual_cameras[k];
				glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, cameras.color_id, 0, camera.virtual_camera_name);
				glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cameras.depth_id, 0, camera.virtual_camera_name);

				glViewport(cameras.viewport.x, cameras.viewport.y, cameras.viewport.w, cameras.viewport.h);

				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadMatrixf(camera.projection.matrix_colmajor);

				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glLoadMatrixf(glm::value_ptr(glm::inverse(mlToGL(camera.transform))));

				if (k == 0) {
					glNewList(calllist, GL_COMPILE_AND_EXECUTE);
					drawPlanes(query_results, query_nresults);
					glEndList();
				} else {
					glCallList(calllist);
				}

				glMatrixMode(GL_MODELVIEW);
				glPopMatrix();
				glMatrixMode(GL_PROJECTION);
				glPopMatrix();
				glBindFramebuffer(GL_FRAMEBUFFER, 0);

				MLGraphicsSignalSyncObjectGL(graphics, cameras.virtual_cameras[k].sync_object);

			}

			CHECK(MLGraphicsEndFrame(graphics, frame));
			graphics_context.swapBuffers();

		} else if (frame_result != MLResult_Timeout) { // sometimes it fails with timeout when device is busy

			ML_LOG(Error, "%s: MLGraphicsBeginFrame failed with %d", APP_NAME, frame_result);
			break;

		}

		// --------------------------------------------------------------------
		// Poll controllers for bumper button presses. The other way to do this
		// is to use MLInputSetControllerCallbacks for a callback-based 
		// approach. Polling serves our purpose here though.

		MLInputControllerState input_states[MLInput_MaxControllers];
		CHECK(MLInputGetControllerState(input, input_states));

		for (int k = 0; k < MLInput_MaxControllers; ++k)
			if (input_states[k].button_state[MLInputControllerButton_Bumper]) {
				ML_LOG(Info, "%s: Bye!", APP_NAME);
				quit = true;
				break;
			}

	} while (!quit);

	// ==== CLEANUP ===========================================================

	glDeleteLists(calllist, 1);
	glDeleteFramebuffers(1, &framebuffer);
	MLGraphicsDestroyClient(&graphics);
	graphics_context.cleanup();
	MLInputDestroy(input);
	MLPlanesDestroy(planes);
	MLHeadTrackingDestroy(head_tracking);
	MLPerceptionShutdown();
	ML_LOG(Info, "%s: Finished.", APP_NAME);

}


void drawPlanes(const MLPlane *p, int np) {

	// If no planes found, just turn the screen red. This is easier to see
	// than log messages, and simpler than setting controller LEDs.
	if (!np) {
		glClearColor(0.25, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		return;
	}

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
	glDisable(GL_LIGHTING);
	glLineWidth(2);
	glMatrixMode(GL_MODELVIEW);

	for (int k = 0; k < np; ++k, ++p) {

		if (p->flags & MLPlanesQueryFlag_Semantic_Ceiling)
			glColor3f(0, 1, 1);
		else if (p->flags & MLPlanesQueryFlag_Semantic_Floor)
			glColor3f(1, 0, 0);
		else
			glColor3f(1, 1, 1);

		MLTransform transform;
		transform.position = p->position;
		transform.rotation = p->rotation;
		float hw = p->width / 2;
		float hh = p->height / 2;

		glPushMatrix();
		glMultMatrixf(glm::value_ptr(mlToGL(transform)));

		glBlendColor(0, 0, 0, 1);
		glBegin(GL_LINE_LOOP);
		glVertex2f(-hw, -hh);
		glVertex2f( hw, -hh);
		glVertex2f( hw,  hh);
		glVertex2f(-hw,  hh);
		glEnd();

		glBlendColor(0, 0, 0, 0.25);
		glBegin(GL_LINES);
		for (float x = -hw + 0.1; x < hw; x += 0.1) {
			glVertex2f(x, -hh);
			glVertex2f(x, hh);
		}
		for (float y = -hh + 0.1; y < hh; y += 0.1) {
			glVertex2f(-hw, y);
			glVertex2f(hw, y);
		}
		glEnd();

		glBlendColor(0, 0, 0, 1);
		glBegin(GL_LINES);
		glColor3f(1, 1, 1);
		glVertex3f(0, 0, 0);
		glColor3f(1, 1, 0);
		glVertex3f(0, 0, 0.1);
		glEnd();

		glPopMatrix();

	}

}