// VR.cpp

#include <glad.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <openvr.h>
#include "VRXtras.h"

// see https://github.com/ValveSoftware/openvr/wiki/API-Documentation
// see https://www.dll-files.com/openvr_api.dll.html
// alternative to OpenVR: https://github.com/OpenHMD/OpenHMD

// a contact? Bruce Dawson: bruced@valvesoftware.com

using std::string;
using namespace vr;

TrackedDevicePose_t tracked_device_pose[k_unMaxTrackedDeviceCount];

string deviceName[k_unMaxTrackedDeviceCount];
ETrackedDeviceClass deviceType[k_unMaxTrackedDeviceCount];
string typeNames[] = {"Invalid", "HMD", "Controller", "GenericTracker", "TrackingReference", "DisplayRedirect"};

string GetTrackedDeviceName(int type) { return deviceName[type]; }

string GetTrackedDeviceString(IVRSystem *pHmd, TrackedDeviceIndex_t unDevice, TrackedDeviceProperty prop, TrackedPropertyError *peError = NULL ) {
	uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty( unDevice, prop, NULL, 0, peError );
	if( unRequiredBufferLen == 0 )
		return "";
	char *pchBuffer = new char[ unRequiredBufferLen ];
	unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty( unDevice, prop, pchBuffer, unRequiredBufferLen, peError );
	string sResult = pchBuffer;
	delete [] pchBuffer;
	return sResult;
}

bool VROOM::HmdPresent() {
	return VR_IsHmdPresent();
}

int VROOM::RecommendedWidth() {
	uint32_t recWidth = 0, recHeight = 0;
	ivr->GetRecommendedRenderTargetSize(&recWidth, &recHeight);
	return recWidth;
}

int VROOM::RecommendedHeight() {
	uint32_t recWidth = 0, recHeight = 0;
	ivr->GetRecommendedRenderTargetSize(&recWidth, &recHeight);
	return recHeight;
}

bool VROOM::InitOpenVR() {
	// see HelloOpenVR_GLFW.cpp
	openVR = VR_IsRuntimeInstalled();
	// check runtime path
	char runtimePath[500] = { 0 };
	uint32_t requiredBufferSize;
	if (VR_GetRuntimePath(runtimePath, 500, &requiredBufferSize))
		printf("found runtime at %s\n", runtimePath);
	else printf("runtimePath = %s\n", runtimePath[0]? runtimePath : "<null>");
	// initialize
	EVRInitError err = VRInitError_None;
	ivr = VR_Init(&err, VRApplication_Scene);
	if (err != VRInitError_None) {
		const char *s = VR_GetVRInitErrorAsEnglishDescription(err);
		printf("InitOpenVR::VR_Init error: %s\n", s);
	}
	if (ivr)
		printf("OpenVR runtime initialized\n");
	if (!VRCompositor())
		printf("can't initialize VR compositor\n");
	if (!ivr)
		return false;
	// ?
	string s1 = GetTrackedDeviceString(ivr, k_unTrackedDeviceIndex_Hmd, Prop_TrackingSystemName_String);
	string s2 = GetTrackedDeviceString(ivr, k_unTrackedDeviceIndex_Hmd, Prop_SerialNumber_String);
	printf("Prop_TrackingSystemName_String = %s\n", s1.c_str());
	printf("Prop_SerialNumber_String = %s\n", s2.c_str());
	// check for devices 
	int nBaseStations = 0;
	for (uint32_t td = k_unTrackedDeviceIndex_Hmd; td < k_unMaxTrackedDeviceCount; td++) {
		if (ivr->IsTrackedDeviceConnected(td)) {
			ETrackedDeviceClass tracked_device_class = ivr->GetTrackedDeviceClass(td);
			deviceName[td] = string(td > 5? "unknown" : typeNames[td]);
			deviceType[td] = tracked_device_class;
			printf("tracking device %i is connected (%s)\n", td, deviceName[td].c_str());
			if (tracked_device_class == ETrackedDeviceClass::TrackedDeviceClass_TrackingReference)
				nBaseStations++;
			if (td == k_unTrackedDeviceIndex_Hmd)
				printf("can't set driver name/serial, but ok?\n");
		}
	}
	printf("%i base station\n", nBaseStations);
	return true;
}

const char *GetCompositorError(int err) {
	return
		err == VRCompositorError_None?							"None" :
		err == VRCompositorError_RequestFailed?					"RequestFailed" :
		err == VRCompositorError_IncompatibleVersion?			"IncompatibleVersion" :
		err == VRCompositorError_DoNotHaveFocus?				"DoNotHaveFocus" :
		err == VRCompositorError_InvalidTexture?				"InvalidTexture" :
		err == VRCompositorError_IsNotSceneApplication?			"IsNotSceneApplication" :
		err == VRCompositorError_TextureIsOnWrongDevice?		"TextureIsOnWrongDevice" :
		err == VRCompositorError_TextureUsesUnsupportedFormat?	"TextureUsesUnsupportedFormat" :
		err == VRCompositorError_SharedTexturesNotSupported?	"SharedTexturesNotSupported" :
		err == VRCompositorError_IndexOutOfRange?				"IndexOutOfRange" :
		err == VRCompositorError_AlreadySubmitted?				"AlreadySubmitted" :
		err == VRCompositorError_InvalidBounds?					"InvalidBounds" :
		err == VRCompositorError_AlreadySet?					"AlreadySet" :
																"Unknown";
}

// HMD Pose

string TrackingResultName(int r) {
	return string(r==1? "uninitialized" : r==100? "calib in progress" : r==101? "calib out of range" :
				  r==200? "running ok" : r==201? "running out of range" : r==300? "fallback rotation only" : "unknown");
}

void PrintMatrix(HmdMatrix34_t m, int pose) {
	printf("matrix for pose %i:\n", pose);
	for (int row = 0; row < 3; row++)
		printf("%3.2f  %3.2f  %3.2f  %3.2f\n", m.m[row][0], m.m[row][1], m.m[row][2], m.m[row][3]);
}

// mat4 hmdTransform, lHandTransform, rHandTransform;

bool onceVR = false;

mat3 mat3from3x4(HmdMatrix34_t m) {
	// 3 rows of 4 columns
	// **** transpose?
	return mat3(vec3(m.m[0][0], m.m[0][1], m.m[0][2]),
				vec3(m.m[1][0], m.m[1][1], m.m[1][2]),
				vec3(m.m[2][0], m.m[2][1], m.m[2][2]));
}

vec3 vec3from3x4(HmdMatrix34_t m) {
	return vec3(m.m[0][3], m.m[1][3], m.m[2][3]);
}

mat4 mat4from3x4(HmdMatrix34_t m) {
	// compensate for reverse x-rotation, presume unit uniform scale
	mat3 m3 = mat3from3x4(m);	// 3x3 rotation matrix
	vec3 v = vec3from3x4(m);	// translation vector
	mat3 xrev(vec3(-1,0,0), vec3(0,1,0), vec3(0,0,1));
	mat3 yrev(vec3(1,0,0), vec3(0,-1,0), vec3(0,0,1));
	mat3 rot = yrev*m3*yrev; // xrev*m3*xrev;
	vec4 row0(rot[0],v.x), row1(rot[1],v.y), row2(rot[2],v.z), row3(0,0,0,1);
	return mat4(row0, row1, row2, row3);
}

mat4 From3By4(HmdMatrix34_t m) {
	// 3 rows of 4 columns
	return mat4(vec4(m.m[0][0], m.m[0][1], m.m[0][2], m.m[0][3]),
				vec4(m.m[1][0], m.m[1][1], m.m[1][2], m.m[1][3]),
				vec4(m.m[2][0], m.m[2][1], m.m[2][2], m.m[2][3]),
				vec4(0, 0, 0, 1));
}

TrackedDevicePose_t pRenderPoseArray[k_unMaxTrackedDeviceCount], pGamePoseArray[k_unMaxTrackedDeviceCount];

void PrintPoses() {
	printf("k_unMaxTrackedDeviceCount = %i\n", k_unMaxTrackedDeviceCount);
	for (int i = 0; i < k_unMaxTrackedDeviceCount; i++) {
		TrackedDevicePose_t pose = pGamePoseArray[i];
		string trackResult = TrackingResultName(pose.eTrackingResult);
		printf("pose device %i: pose is %svalid, device is %sconnected, tracking result is %s\n",
			i, pose.bPoseIsValid? "" : "not ", pose.bDeviceIsConnected? "" : "not ", trackResult.c_str());
	}
}


bool VROOM::GetTransforms(mat4 &head, mat4 &leftHand, mat4 &rightHand) {
	EVRCompositorError err = VRCompositor()->WaitGetPoses(pRenderPoseArray, k_unMaxTrackedDeviceCount, pGamePoseArray, k_unMaxTrackedDeviceCount);
	if (err) {
		printf("VRCompositor:WaitGetPoses: %s\n", GetCompositorError(err));
		return false;
	}
	if (!onceVR) PrintPoses();
	// it seems device[0] = HMD, device[2] = left hand, device[5] = right hand
	TrackedDeviceIndex_t devices[100];
	// headsets
	int nHMDs = ivr->GetSortedTrackedDeviceIndicesOfClass(TrackedDeviceClass_HMD, devices, 100);
	if (!onceVR) printf("%i head displays\n", nHMDs);
	TrackedDevicePose_t pose = pGamePoseArray[devices[0]]; // presumed HMD
	if (pose.bPoseIsValid && pose.bDeviceIsConnected) {
		HmdMatrix34_t m = pose.mDeviceToAbsoluteTracking;
		// head = From3By4(m);
		//	   this would seem proper transfer 3x4 to 4x4, but x-rotation reversed
		head = mat4from3x4(m);
	} 
	// hands
	int nControllers = ivr->GetSortedTrackedDeviceIndicesOfClass(TrackedDeviceClass_Controller, devices, 100);
	if (!onceVR) printf("%i controllers\n", nControllers);
	// assuming devices[0] left, devices[1] right 
	if (nControllers > 0) {
		pose = pGamePoseArray[devices[0]];
		if (pose.bPoseIsValid && pose.bDeviceIsConnected)
			leftHand = mat4from3x4(pose.mDeviceToAbsoluteTracking);
		//	leftHand = From3By4(pose.mDeviceToAbsoluteTracking);
	}
	if (nControllers > 1) {
		pose = pGamePoseArray[devices[1]];
		if (pose.bPoseIsValid && pose.bDeviceIsConnected)
			rightHand = mat4from3x4(pose.mDeviceToAbsoluteTracking);
		//	rightHand = From3By4(pose.mDeviceToAbsoluteTracking);
	}
	onceVR = true;
	return true;
}


// transfer images to HMD

void VROOM::SubmitOpenGLFrames(GLuint leftTextureUnit, GLuint rightTextureUnit) {
	Texture_t leftEyeTexture = {(void *) leftTextureUnit, TextureType_OpenGL, ColorSpace_Auto}; // Linear};
	Texture_t rightEyeTexture = {(void *) rightTextureUnit, TextureType_OpenGL, ColorSpace_Auto}; // Linear};
	EVRCompositorError err = VRCompositorError_None;
	if (!VRCompositor())
		return;
	glBindTexture(GL_TEXTURE_2D, leftTextureUnit); // ?
	err = VRCompositor()->Submit(Eye_Left, &leftEyeTexture);
	if (err) printf("VRCompositor:Submit(left): %s\n", GetCompositorError(err));
	glBindTexture(GL_TEXTURE_2D, rightTextureUnit); // ?
	err = VRCompositor()->Submit(Eye_Right, &rightEyeTexture);
	if (err) printf("VRCompositor:Submit(right): %s\n", GetCompositorError(err));
	glFlush();
	VRCompositor()->PostPresentHandoff();
}

bool multisample = false; // *** fails ***

bool VROOM::InitFrameBuffer(int w, int h) {
	width = w;
	height = h;
	pixels = (float *) new float[4*width*height]; // deleted in destructor
	// make new frame buffer of texture and depth buffer
	glGenFramebuffers(1, &framebuffer);
	glGenTextures(1, &framebufferTextureName);
	glGenRenderbuffers(1, &depthBuffer);
	// setup texture
	glActiveTexture(GL_TEXTURE0+framebufferTextureName);
	if (multisample) {
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebufferTextureName);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA, width, height, GL_TRUE);
		glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	}
	else {
		glBindTexture(GL_TEXTURE_2D, framebufferTextureName);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, 0);
	//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	// depth buffer
	glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
	if (multisample)
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, width, height);
	else
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
	// configure frame buffer with depth buffer and color attachment
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
	if (multisample)
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferTextureName, 0);
	else
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, framebufferTextureName, 0);
	// enable drawing
	GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, drawBuffers);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		printf("bad frame buffer status\n");
	return framebuffer > 0 && depthBuffer > 0;
}

void VROOM::CopyFramebufferToEyeTexture(GLuint textureName, GLuint textureUnit) {
	// read from framebuffer
	glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, pixels);
	// store pixels as GL texture
	glActiveTexture(GL_TEXTURE0+textureUnit);
	glBindTexture(GL_TEXTURE_2D, textureName); // bind active texture to textureName
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);     // accommodate width not multiple of 4
//	***** some Occulus devices reporting error: TextureUsesUnsupportedFormat
//	***** OpenVR says: scene textures must be compatible with DXGI sharing rules - e.g. uncompressed, no mips, etc.
//	***** so, as test, perhaps following two lines were causing the error
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, pixels);
//	***** could Occulus want GL_RGB, rather than GL_RGBA??
}

/*version 1
#ifdef VROOM_V1
bool GetHMD(mat4 &hmd, bool print = false) {
	EVRCompositorError err = VRCompositor()->WaitGetPoses(pRenderPoseArray, k_unMaxTrackedDeviceCount, pGamePoseArray, k_unMaxTrackedDeviceCount);
	if (err)
		printf("VRCompositor:WaitGetPoses: %s\n", GetCompositorError(err));
	else {
		if (!once)
			PrintPoses();
		for (int i = 0; i < k_unMaxTrackedDeviceCount; i++) {
			TrackedDevicePose_t pose = pGamePoseArray[i];
			if (pose.bPoseIsValid && pose.bDeviceIsConnected) {
				HmdMatrix34_t m = pose.mDeviceToAbsoluteTracking;
				if (print) PrintMatrix(m, i);
				hmd = From3By4(m);
//	HOW DO WE KNOW THIS IS FOR HMD, AND NOT FOR A HAND CONTROLLER?
//	VERIFY k_unMaxTrackedDeviceCount IS 64
//	HOW MANY OF pGamePoseArray IS bPoseIsValid?
//				hmd = mat4(vec4(m.m[0][0], m.m[0][1], m.m[0][2], m.m[0][3]),
//						   vec4(m.m[1][0], m.m[1][1], m.m[1][2], m.m[1][3]),
//						   vec4(m.m[2][0], m.m[2][1], m.m[2][2], m.m[2][3]),
//						   vec4(0, 0, 0, 1));
				return true;
			}
		}
	}
	return false;
}
// initialize/update application transform
bool VROOM::StartTransformHMD(mat4 *m) {
	// calibrate raw HMD transform, set m as target (ie, application) HMD transform
	appTransform = m;
	appTransformStart = *m;
	mat4 hmd;
/	THIS IS "RELATIVE ORIENTATION" APPROACH THAT APPEARS TOTALLY INAPPROPRIATE - IS HmdMatrix34_t ALWAYS ABSOLUTE?
//	if (VRCompositor() && GetHMD(hmd)) {
//		qStart = Quaternion(hmd);
//		qStartConjugate = Quaternion(-qStart.x, -qStart.y, -qStart.z, qStart.w);
//		pStart = vec3(hmd[0][3], hmd[1][3], hmd[2][3]);
//		return true;
//	} 
	return false;
}
void VROOM::UpdateTransformHMD() {
	GetHMD(*appTransform, !once);
	once = true;
//	mat4 r;
//	if (GetHMD(r)) {
//		Quaternion qNow(r), qDif = qNow*qStartConjugate;
//		vec3 pNow(r[0][3], r[1][3], r[2][3]), pDif = translationScale*(pNow-pStart);
//		// build matrix that is difference between start and present HMD poses
//		mat4 mDif = qDif.GetMatrix();
//		mDif[0][3] = pDif.x; mDif[1][3] = pDif.y; mDif[2][3] = pDif.z;
//		*appTransform = mDif*(appTransformStart);
//	} 
}
#else
mat4 VROOM::GetHeadTransform() {
	TrackedDeviceIndex_t devices[100];
	int nDevices = hmd->GetSortedTrackedDeviceIndicesOfClass(TrackedDeviceClass_HMD, devices, 100);
	if (!once) printf("#i head displays\n", nDevices);
	once = true;
	TrackedDevicePose_t pose = pGamePoseArray[0];
	if (pose.bPoseIsValid && pose.bDeviceIsConnected) {
		HmdMatrix34_t m = pose.mDeviceToAbsoluteTracking;
		return (From3By4(m));
	}
	return mat4(1);
}
mat4 VROOM::GetLeftHandTransform() {
	TrackedDeviceIndex_t devices[100];
	int nDevices = hmd->GetSortedTrackedDeviceIndicesOfClass(TrackedDeviceClass_Controller, devices, 100);
	HmdMatrix34_t m;
	return From3By4(m);
}
mat4 VROOM::GetRightHandTransform() {
	HmdMatrix34_t m;
	return From3By4(m);
}
#endif
// from openvr.h
enum ETrackedDeviceClass {
	TrackedDeviceClass_Invalid = 0,				// the ID was not valid.
	TrackedDeviceClass_HMD = 1,					// Head-Mounted Displays
	TrackedDeviceClass_Controller = 2,			// Tracked controllers
	TrackedDeviceClass_GenericTracker = 3,		// Generic trackers, similar to controllers
	TrackedDeviceClass_TrackingReference = 4,	// Camera and base stations that serve as tracking reference points
	TrackedDeviceClass_DisplayRedirect = 5,		// Accessories that aren't necessarily tracked themselves, but may redirect video output from other tracked devices
	TrackedDeviceClass_Max
};
// string driver_name, driver_serial;
// string GetDriverName() { return driver_name; }
// string GetDriverSerial() { return driver_serial; }
// following of no value?
//const char *VROOM::ButtonName(const VREvent_t &e) {
//	uint32_t t = e.eventType;
//	if (t==VREvent_ButtonPress || t==VREvent_ButtonUnpress || t==VREvent_ButtonTouch || t==VREvent_ButtonUntouch) {
//		VREvent_Controller_t controller_data = e.data.controller;
//		return hmd->GetButtonIdNameFromEnum((EVRButtonId) controller_data.button);
////		return vr_context->GetButtonIdNameFromEnum((EVRButtonId) controller_data.button);
//	}
//	return NULL;
//}
 void VROOM::ProcessEvent(const VREvent_t &e) {
	switch(e.eventType) {
		case VREvent_TrackedDeviceActivated:
			printf("Event: device %i activated\n", e.trackedDeviceIndex);
			break;
		case VREvent_TrackedDeviceDeactivated:
			printf("Event: device %i deactivated\n", e.trackedDeviceIndex);
			break;
		case VREvent_TrackedDeviceUpdated:
			printf("Event: device %i updated\n", e.trackedDeviceIndex);
			break;
		case VREvent_ButtonPress:
			printf("Event: device %i, %s button pressed\n", e.trackedDeviceIndex, ButtonName(e));
			break;
		case VREvent_ButtonUnpress:
			printf("Event: device %i, %s button un-pressed\n", e.trackedDeviceIndex, ButtonName(e));
			break;
		case VREvent_ButtonTouch:
			printf("Event: device %i, %s button touched\n", e.trackedDeviceIndex, ButtonName(e));
			break;
		case VREvent_ButtonUntouch:
			printf("Event: device %i, %s button un-touched\n", e.trackedDeviceIndex, ButtonName(e));
			break;
	}
} */
