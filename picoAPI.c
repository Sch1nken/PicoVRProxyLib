#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>
#include <android/log.h>

#include "Headers/PxrApi.h"
#include "Headers/PxrTypes.h"
#include "Headers/PxrInput.h"
#include "Headers/PxrEnums.h"

#include "Headers/VrApi_Config.h"

#include <GLES3/gl3.h>

#define VRAPI_PI 3.14159265358979323846f
#define VRAPI_ZNEAR 0.1f

int eyeLayerId = 0;
bool resumed = false;
uint64_t layerImages[PXR_EYE_MAX][3];
PxrVector2f joystick[PXR_CONTROLLER_COUNT];
uint32_t mainController;
double predictedDisplayTimeMs = 0.0f;

// from android samples
/* return current time in milliseconds */
static double now_ms(void) {

    struct timespec res;
    clock_gettime(CLOCK_REALTIME, &res);
    return 1000.0 * res.tv_sec + (double) res.tv_nsec / 1e6;

}

#if defined(ANDROID)
#include <jni.h>
#elif defined(__cplusplus)
typedef struct _JNIEnv JNIEnv;
typedef struct _JavaVM JavaVM;
typedef class _jobject* jobject;
#else
typedef const struct JNINativeInterface* JNIEnv;
typedef const struct JNIInvokeInterface* JavaVM;
typedef void* jobject;
#endif

/// A VR-capable device.
typedef enum ovrDeviceType_ {
        VRAPI_DEVICE_TYPE_OCULUSQUEST_START = 256,
        VRAPI_DEVICE_TYPE_OCULUSQUEST = VRAPI_DEVICE_TYPE_OCULUSQUEST_START + 3,
    VRAPI_DEVICE_TYPE_OCULUSQUEST_END = 319,
    VRAPI_DEVICE_TYPE_OCULUSQUEST2_START = 320,
    VRAPI_DEVICE_TYPE_OCULUSQUEST2 = VRAPI_DEVICE_TYPE_OCULUSQUEST2_START,
    VRAPI_DEVICE_TYPE_OCULUSQUEST2_END = 383,
                VRAPI_DEVICE_TYPE_UNKNOWN = -1,
} ovrDeviceType;

typedef enum ovrInitializeStatus_ {
    VRAPI_INITIALIZE_SUCCESS = 0,
    VRAPI_INITIALIZE_UNKNOWN_ERROR = -1,
    VRAPI_INITIALIZE_PERMISSIONS_ERROR = -2,
    VRAPI_INITIALIZE_ALREADY_INITIALIZED = -3,
    VRAPI_INITIALIZE_SERVICE_CONNECTION_FAILED = -4,
    VRAPI_INITIALIZE_DEVICE_NOT_SUPPORTED = -5,
} ovrInitializeStatus;

typedef enum ovrStructureType_ {
    VRAPI_STRUCTURE_TYPE_INIT_PARMS = 1,
    VRAPI_STRUCTURE_TYPE_MODE_PARMS = 2,
    VRAPI_STRUCTURE_TYPE_FRAME_PARMS = 3,
        VRAPI_STRUCTURE_TYPE_MODE_PARMS_VULKAN = 5,
    } ovrStructureType;

/// Java details about an activity
typedef struct ovrJava_ {
    JavaVM* Vm; //< Java Virtual Machine
    JNIEnv* Env; //< Thread specific environment
    jobject ActivityObject; //< Java activity object
} ovrJava;

/// Supported graphics APIs.
typedef enum ovrGraphicsAPI_ {
    VRAPI_GRAPHICS_API_TYPE_OPENGL_ES = 0x10000,
    VRAPI_GRAPHICS_API_OPENGL_ES_2 =
        (VRAPI_GRAPHICS_API_TYPE_OPENGL_ES | 0x0200), //< OpenGL ES 2.x context
    VRAPI_GRAPHICS_API_OPENGL_ES_3 =
        (VRAPI_GRAPHICS_API_TYPE_OPENGL_ES | 0x0300), //< OpenGL ES 3.x context

    VRAPI_GRAPHICS_API_TYPE_OPENGL = 0x20000,
    VRAPI_GRAPHICS_API_OPENGL_COMPAT =
        (VRAPI_GRAPHICS_API_TYPE_OPENGL | 0x0100), //< OpenGL Compatibility Profile
    VRAPI_GRAPHICS_API_OPENGL_CORE_3 =
        (VRAPI_GRAPHICS_API_TYPE_OPENGL | 0x0300), //< OpenGL Core Profile 3.x
    VRAPI_GRAPHICS_API_OPENGL_CORE_4 =
        (VRAPI_GRAPHICS_API_TYPE_OPENGL | 0x0400), //< OpenGL Core Profile 4.x

    VRAPI_GRAPHICS_API_TYPE_VULKAN = 0x40000,
    VRAPI_GRAPHICS_API_VULKAN_1 = (VRAPI_GRAPHICS_API_TYPE_VULKAN | 0x0100), //< Vulkan 1.x
} ovrGraphicsAPI;

/// Configuration details specified at initialization.
typedef struct ovrInitParms_ {
    ovrStructureType Type;
    int ProductVersion;
    int MajorVersion;
    int MinorVersion;
    int PatchVersion;
    ovrGraphicsAPI GraphicsAPI;
    ovrJava Java;
} ovrInitParms;

typedef enum ovrSuccessResult_ {
    ovrSuccess = 0,
    ovrSuccess_BoundaryInvalid = 1001,
    ovrSuccess_EventUnavailable = 1002,
    ovrSuccess_Skipped = 1003,

} ovrSuccessResult;

typedef enum ovrErrorResult_ {
    ovrError_MemoryAllocationFailure = -1000,
    ovrError_NotInitialized = -1004,
    ovrError_InvalidParameter = -1005,
    ovrError_DeviceUnavailable = -1010, //< device is not connected,
                                        // or not connected as input device
    ovrError_InvalidOperation = -1015,

    // enums not in CAPI
    ovrError_UnsupportedDeviceType = -1050, //< specified device type isn't supported on GearVR
    ovrError_NoDevice = -1051, //< specified device ID does not map to any current device
    ovrError_NotImplemented = -1052, //< executed an incomplete code path - this should not be
                                     // possible in public releases.
    /// ovrError_NotReady is returned when a subsystem supporting an API is not yet ready.
    /// For some subsystems, vrapi_PollEvent will return a ready event once the sub-system is
    /// available.
    ovrError_NotReady = -1053,
    /// Data is unavailable
    ovrError_Unavailable = -1054,

    ovrResult_EnumSize = 0x7fffffff
} ovrErrorResult;

extern ovrInitializeStatus vrapi_Initialize(const ovrInitParms* initParms) {
	__android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_Initialize");

    PxrInitParamData initParamData;

    initParamData.activity      = initParms->Java.ActivityObject;
    initParamData.vm            = initParms->Java.Vm;
    initParamData.controllerdof = 1;
    initParamData.headdof       = 1;

    Pxr_SetInitializeData(&initParamData);
    int ret = Pxr_Initialize();

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_Initialize ret = %d", ret);

    // VRAPI_INITIALIZE_SUCCESS == 0

    if(ret == 0) {
        return 0;
    }

	return -1;
}

/// Built-in convenience swapchains.
typedef enum ovrDefaultTextureSwapChain_ {
    VRAPI_DEFAULT_TEXTURE_SWAPCHAIN = 0x1,
    VRAPI_DEFAULT_TEXTURE_SWAPCHAIN_LOADING_ICON = 0x2
} ovrDefaultTextureSwapChain;

typedef struct ovrTextureSwapChain ovrTextureSwapChain;



extern void vrapi_DestroyTextureSwapChain(ovrTextureSwapChain* chain) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_DestroyTextureSwapChain");
}

typedef signed int ovrResult;
typedef struct ovrMobile ovrMobile;

/// To be optionally called at the start of the main thread.
extern ovrResult vrapi_WaitFrame(ovrMobile* ovr, uint64_t frameIndex) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_WaitFrame");

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "NOT IMPLEMENTED YET!");

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_WaitFrame");

    return ovrSuccess;
}

/// To be optionally called at the start of the render thread.
extern ovrResult vrapi_BeginFrame(ovrMobile* ovr, uint64_t frameIndex) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_BeginFrame");

    Pxr_BeginFrame();

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_BeginFrame");

    return ovrSuccess;
}

extern ovrTextureSwapChain* vrapi_CreateAndroidSurfaceSwapChain(int width, int height) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_CreateAndroidSurfaceSwapChain");
}

extern ovrTextureSwapChain* vrapi_CreateAndroidSurfaceSwapChain2(int width, int height, bool isProtected) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_CreateAndroidSurfaceSwapChain2");
}

/// 'flags' is specified as a combination of ovrAndroidSurfaceSwapChainFlags flags.
extern ovrTextureSwapChain* vrapi_CreateAndroidSurfaceSwapChain3(int width, int height, uint64_t flags) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_CreateAndroidSurfaceSwapChain3");

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_CreateAndroidSurfaceSwapChain3");
}

// From <vulkan/vulkan.h>:
#if !defined(VK_VERSION_1_0)
#define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;
#if defined(__LP64__) || defined(_WIN64) || (defined(__x86_64__) && !defined(__ILP32__)) || \
    defined(_M_X64) || defined(__ia64) || defined(_M_IA64) || defined(__aarch64__) ||       \
    defined(__powerpc64__)
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef struct object##_T* object;
#else
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef uint64_t object;
#endif
VK_DEFINE_HANDLE(VkInstance)
VK_DEFINE_HANDLE(VkPhysicalDevice)
VK_DEFINE_HANDLE(VkDevice)
VK_DEFINE_HANDLE(VkQueue)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkImage)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDeviceMemory)
#define VK_NULL_HANDLE 0
#endif

/// Initialization parameters unique to Vulkan.
typedef struct ovrSystemCreateInfoVulkan_ {
    VkInstance Instance;
    VkPhysicalDevice PhysicalDevice;
    VkDevice Device;
} ovrSystemCreateInfoVulkan;

/// Initializes the API for Vulkan support.
/// This is lightweight and does not create any threads.
/// This is called after vrapi_Initialize and before texture swapchain creation, or
/// vrapi_enterVrMode.
/*

typedef struct PxrVulkanBinding_ {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    uint32_t queueFamilyIndex;
    uint32_t queueIndex;
    const void* next;
} PxrVulkanBinding;

*/
extern ovrResult vrapi_CreateSystemVulkan(ovrSystemCreateInfoVulkan* systemInfo) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_CreateSystemVulkan");

    PxrVulkanBinding binding;

    binding.instance = systemInfo->Instance;
    binding.physicalDevice = systemInfo->PhysicalDevice;
    binding.device = systemInfo->Device;

    int ret = Pxr_CreateVulkanSystem(&binding);

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_CreateSystemVulkan");

    if (ret == 0) {
        return ovrSuccess;
    }

    return ovrError_InvalidParameter;
}

/// Texture Swap Chain lifetime is explicitly controlled by the application via calls
/// to vrapi_CreateTextureSwapChain* or vrapi_CreateAndroidSurfaceSwapChain and
/// vrapi_DestroyTextureSwapChain. Swap Chains are associated with the VrApi instance,
/// not the VrApi ovrMobile. Therefore, calls to vrapi_EnterVrMode and vrapi_LeaveVrMode
/// will not destroy or cause the Swap Chain to become invalid.

/// Create a texture swap chain that can be passed to vrapi_SubmitFrame*().
/// Must be called from a thread with a valid OpenGL ES context current.
///
/// 'bufferCount' used to be a bool that selected either a single texture index
/// or a triple buffered index, but the new entry point vrapi_CreateTextureSwapChain2,
/// allows up to 16 buffers to be allocated, which is useful for maintaining a
/// deep video buffer queue to get better frame timing.
///
/// 'format' used to be an ovrTextureFormat but has been expanded to accept
/// platform specific format types. For GLES, this is the internal format.
/// If an unsupported format is provided, swapchain creation will fail.
///
/// SwapChain creation failures result in a return value of 'nullptr'.

typedef struct ovrSwapChainCreateInfo_ {
    /// GL/Vulkan format of the texture, e.g. GL_RGBA or VK_FORMAT_R8G8B8A8_UNORM),
    /// depending on GraphicsAPI used.
    int64_t Format;

    /// Width in pixels.
    int Width;

    /// Height in pixels.
    int Height;

    /// The number of levels of detail available for minified sampling of the image.
    int Levels;

    /// Number of faces, which can be either 6 (for cubemaps) or 1.
    int FaceCount;

    /// Number of array layers, 1 for 2D texture, 2 for texture 2D array (multiview case).
    int ArraySize;

    /// Number of buffers in the texture swap chain.
    int BufferCount;

    /// A bitmask of ovrSwapChainCreateFlags describing additional properties of
    /// the swapchain.
    uint64_t CreateFlags;

    /// A bitmask of ovrSwapChainUsageFlags describing intended usage of the
    /// swapchain's images.
    uint64_t UsageFlags;
} ovrSwapChainCreateInfo;

extern ovrTextureSwapChain* vrapi_CreateTextureSwapChain4(
    const ovrSwapChainCreateInfo* createInfo) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_CreateTextureSwapChain4");
    }

typedef enum ovrTextureType_ {
    VRAPI_TEXTURE_TYPE_2D = 0, //< 2D textures.
    VRAPI_TEXTURE_TYPE_2D_ARRAY = 2, //< Texture array.
    VRAPI_TEXTURE_TYPE_CUBE = 3, //< Cube maps.
    VRAPI_TEXTURE_TYPE_MAX = 4,
    } ovrTextureType;

uint64_t _chain;

extern ovrTextureSwapChain* vrapi_CreateTextureSwapChain3(
    ovrTextureType type,
    int64_t format, // 32856 == GL_RGBA8??
    int width,
    int height,
    int levels,
    int bufferCount) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_CreateTextureSwapChain3");
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "\ntype = %d\nformat = %" PRId64 "\nwidth = %d\nheight = %d\nlevels = %d\nbufferCount = %d", type, format, width, height, levels, bufferCount);
       
        PxrLayerParam layerParam = {};

        layerParam.layerId = 0;
        layerParam.layerShape = PXR_LAYER_PROJECTION;
        layerParam.layerLayout = PXR_LAYER_LAYOUT_STEREO;
        layerParam.width = width;
        layerParam.height = height;
        layerParam.faceCount = 1;
        layerParam.mipmapCount = 1;
        layerParam.sampleCount = 1;
        layerParam.arraySize = bufferCount;
        layerParam.format = format;
        int ret = Pxr_CreateLayer(&layerParam);

        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "Pxr_CreateLayer (id = %d, ret = %d)", layerParam.layerId, ret);
       
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_CreateTextureSwapChain3");

        _chain = 0;

        if(ret == 0) {
            return &_chain;
        }

        return NULL;
    }

typedef enum ovrTextureFormat_ {
    VRAPI_TEXTURE_FORMAT_NONE = 0,
    VRAPI_TEXTURE_FORMAT_565 = 1,
    VRAPI_TEXTURE_FORMAT_5551 = 2,
    VRAPI_TEXTURE_FORMAT_4444 = 3,
    VRAPI_TEXTURE_FORMAT_8888 = 4,
    VRAPI_TEXTURE_FORMAT_8888_sRGB = 5,
    VRAPI_TEXTURE_FORMAT_RGBA16F = 6,
    VRAPI_TEXTURE_FORMAT_DEPTH_16 = 7,
    VRAPI_TEXTURE_FORMAT_DEPTH_24 = 8,
    VRAPI_TEXTURE_FORMAT_DEPTH_24_STENCIL_8 = 9,
    VRAPI_TEXTURE_FORMAT_RG16 = 10,

    } ovrTextureFormat;

typedef enum ovrTextureFilter_ {
    VRAPI_TEXTURE_FILTER_NEAREST = 0,
    VRAPI_TEXTURE_FILTER_LINEAR = 1,
    VRAPI_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR = 2,
    VRAPI_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST = 3,
    VRAPI_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR = 4,
    VRAPI_TEXTURE_FILTER_CUBIC = 5,
    VRAPI_TEXTURE_FILTER_CUBIC_MIPMAP_NEAREST = 6,
    VRAPI_TEXTURE_FILTER_CUBIC_MIPMAP_LINEAR = 7,
} ovrTextureFilter;

typedef enum ovrTextureWrapMode_ {
    VRAPI_TEXTURE_WRAP_MODE_REPEAT = 0,
    VRAPI_TEXTURE_WRAP_MODE_CLAMP_TO_EDGE = 1,
    VRAPI_TEXTURE_WRAP_MODE_CLAMP_TO_BORDER = 2,
} ovrTextureWrapMode;

typedef struct ovrTextureSamplerState_ {
    ovrTextureFilter MinFilter;
    ovrTextureFilter MagFilter;
    ovrTextureWrapMode WrapModeS;
    ovrTextureWrapMode WrapModeT;
    float BorderColor[4];
    float MaxAnisotropy;
} ovrTextureSamplerState;

extern ovrTextureSwapChain* vrapi_CreateTextureSwapChain2(
    ovrTextureType type,
    ovrTextureFormat format,
    int width,
    int height,
    int levels,
    int bufferCount) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_CreateTextureSwapChain2");
    }

extern ovrTextureSwapChain* vrapi_CreateTextureSwapChain(
    ovrTextureType type,
    ovrTextureFormat format,
    int width,
    int height,
    int levels,
    bool buffered) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_CreateTextureSwapChain");
    }

extern void vrapi_DestroySystemVulkan() {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_DestroySystemVulkan");

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "NOT IMPLEMENTED BY PICO API!");

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_DestroySystemVulkan");
}

typedef struct ovrModeParms_ {
    ovrStructureType Type;

    /// Combination of ovrModeFlags flags.
    unsigned int Flags;

    /// The Java VM is needed for the time warp thread to create a Java environment.
    /// A Java environment is needed to access various system services. The thread
    /// that enters VR mode is responsible for attaching and detaching the Java
    /// environment. The Java Activity object is needed to get the windowManager,
    /// packageName, systemService, etc.
    ovrJava Java;

    
    OVR_VRAPI_PADDING_32_BIT(4)

    /// Display to use for asynchronous time warp rendering.
    /// Using EGL this is an EGLDisplay.
    unsigned long long Display;

    /// The ANativeWIndow associated with the application's Surface (requires
    /// VRAPI_MODE_FLAG_NATIVE_WINDOW). The ANativeWIndow is used for asynchronous time warp
    /// rendering.
    unsigned long long WindowSurface;

    /// The resources from this context will be shared with the asynchronous time warp.
    /// Using EGL this is an EGLContext.
    unsigned long long ShareContext;
} ovrModeParms;

extern ovrMobile* vrapi_EnterVrMode(const ovrModeParms* parms) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_EnterVrMode");
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "TODO: Grab ovrModeParms");
    int ret = Pxr_BeginXr();

    if (ret != 0) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "Could not initialize using Pxr_BeginXr() -> %d", ret);
    }

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_EnterVrMode");

    //TODO: Return ovrMobile struct ptr
}

typedef enum ovrControllerType_ {
    ovrControllerType_None = 0,
    ovrControllerType_Reserved0 = (1 << 0), //< LTouch in CAPI
    ovrControllerType_Reserved1 = (1 << 1), //< RTouch in CAPI
    ovrControllerType_TrackedRemote = (1 << 2),
        ovrControllerType_Gamepad = (1 << 4), // Deprecated, will be removed in a future release
    ovrControllerType_Hand = (1 << 5),

        ovrControllerType_StandardPointer = (1 << 7),
        ovrControllerType_EnumSize = 0x7fffffff
} ovrControllerType;

typedef uint32_t ovrDeviceID;

typedef struct ovrInputCapabilityHeader_ {
    ovrControllerType Type;

    /// A unique ID for the input device
    ovrDeviceID DeviceID;
} ovrInputCapabilityHeader;


// 0 = PXR_CONTROLLER_LEFT
// 1 = PXR_CONTROLLER_RIGHT
// 2 = PXR_CONTROLLER_COUNT

//TODO: Fix, figure out what device index means on quest 2
extern ovrResult vrapi_EnumerateInputDevices(
    ovrMobile* ovr,
    const uint32_t index,
    ovrInputCapabilityHeader* capsHeader) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_EnumerateInputDevices");

        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "Application is requesting index %d", index);

        if (index >= 2) {
            return -1;
        }

        PxrControllerCapability capability;
        
        int ret = Pxr_GetControllerCapabilities(index, &capability);

        capsHeader->Type = index + 1; //ovrControllerType_Reserved0 + ovrControllerType_Reserved1 = 1 & 2
        capsHeader->DeviceID = index + 1;
        
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "Pico Controller caps for index %d (type = %d), ", index, capability.type);

        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_EnumerateInputDevices");

        return ovrSuccess;
    }

/// A 3D vector.
typedef struct ovrVector3f_ {
    float x, y, z;
} ovrVector3f;

extern ovrResult vrapi_GetBoundaryGeometry(
    ovrMobile* ovr,
    const uint32_t pointsCountInput,
    uint32_t* pointsCountOutput,
    ovrVector3f* points) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_GetBoundaryGeometry");

        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "NOT IMPLEMENTED YET!");

        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_GetBoundaryGeometry");
    }

/// Quaternion.
typedef struct ovrQuatf_ {
    float x, y, z, w;
} ovrQuatf;

typedef struct ovrPosef_ {
    ovrQuatf Orientation;
    union {
        ovrVector3f Position;
        ovrVector3f Translation;
    };
} ovrPosef;

extern ovrResult
vrapi_GetBoundaryOrientedBoundingBox(ovrMobile* ovr, ovrPosef* pose, ovrVector3f* scale) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_GetBoundaryOrientedBoundingBox");

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "NOT IMPLEMENTED YET!");

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_GetBoundaryOrientedBoundingBox");
}

typedef enum ovrTrackedDeviceTypeId_ {
    VRAPI_TRACKED_DEVICE_NONE = -1,
    VRAPI_TRACKED_DEVICE_HMD = 0, //< Headset
    VRAPI_TRACKED_DEVICE_HAND_LEFT = 1, //< Left controller
    VRAPI_TRACKED_DEVICE_HAND_RIGHT = 2, //< Right controller
    VRAPI_NUM_TRACKED_DEVICES = 3,
} ovrTrackedDeviceTypeId;

/// Guardian boundary trigger state information based on a given tracked device type
typedef struct ovrBoundaryTriggerResult_ {
    /// Closest point on the boundary surface.
    ovrVector3f ClosestPoint;

    /// Normal of the closest point on the boundary surface.
    ovrVector3f ClosestPointNormal;

    /// Distance to the closest guardian boundary surface.
    float ClosestDistance;

    /// True if the boundary system is being triggered. Note that due to fade in/out effects this
    /// may not exactly match visibility.
    bool IsTriggering;
} ovrBoundaryTriggerResult;

extern ovrResult vrapi_GetBoundaryTriggerState(
    ovrMobile* ovr,
    const ovrTrackedDeviceTypeId deviceId,
    ovrBoundaryTriggerResult* result) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_GetBoundaryTriggerState");
    }

extern ovrResult vrapi_GetBoundaryVisible(ovrMobile* ovr, bool* visible) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_GetBoundaryVisible");

    *visible = false;
    int ret = Pxr_GetBoundaryVisible();

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_GetBoundaryVisible");

    if (ret == 0) {
        *visible = true;
        return ovrSuccess;
    }

    return ovrError_Unavailable;
}

typedef struct ovrInputStateHeader_ {
    /// Type type of controller
    ovrControllerType ControllerType;

    /// System time when the controller state was last updated.
    double TimeInSeconds;
} ovrInputStateHeader;


extern ovrResult vrapi_GetCurrentInputState(
    ovrMobile* ovr,
    const ovrDeviceID deviceID,
    ovrInputStateHeader* inputState) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_GetCurrentInputState");
    }

extern ovrResult
vrapi_GetDeviceExtensionsVulkan(char* extensionNames, uint32_t* extensionNamesSize) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_GetDeviceExtensionVulkan");

    int ret = Pxr_GetDeviceExtensionsVk((const char **)extensionNames, extensionNamesSize);

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_GetDeviceExtensionVulkan");

    if (ret == 0) {
        return ovrSuccess;
    }

    return ovrError_InvalidParameter;
}

typedef enum ovrHandedness_ {
    VRAPI_HAND_UNKNOWN = 0,
    VRAPI_HAND_LEFT = 1,
    VRAPI_HAND_RIGHT = 2
} ovrHandedness;

/// Unified version struct
typedef enum ovrHandVersion_ {
    ovrHandVersion_1 = 0xdf000001, /// Current

    
    ovrHandVersion_EnumSize = 0x7fffffff
} ovrHandVersion;

// Header for all mesh structures.
typedef struct ovrHandMeshHeader_ {
    // The version number of the mesh structure.
    ovrHandVersion Version;
} ovrHandMeshHeader;

extern ovrResult
vrapi_GetHandMesh(ovrMobile* ovr, const ovrHandedness handedness, ovrHandMeshHeader* header) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_GetHandMesh");
    
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "NOT IMPLEMENTED YET!");
    
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_GetHandMesh");
}

// Header for all hand pose structures.
typedef struct ovrHandPoseHeader_ {
    // The version number of the Pose structure.
    // When requesting a pose with vrapi_GetHandPose this MUST be set to the proper version.
    // If this is not set to a known version, or if the version it is set to is no longer
    // supported for the current SDK, ovr_GetHand* functions will return ovrError_InvalidParameter.
    ovrHandVersion Version;

    /// Reserved for later use
    double Reserved;
} ovrHandPoseHeader;

extern ovrResult vrapi_GetHandPose(
    ovrMobile* ovr,
    const ovrDeviceID deviceID,
    const double absTimeInSeconds,
    ovrHandPoseHeader* header) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_GetHandPose");

        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "NOT IMPLEMENTED YET!");

        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_GetHandPose");
    }

// Header for all mesh structures.
typedef struct ovrHandSkeletonHeader_ {
    // The version number of the skeleton structure.
    ovrHandVersion Version;
} ovrHandSkeletonHeader;

extern ovrResult vrapi_GetHandSkeleton(
    ovrMobile* ovr,
    const ovrHandedness handedness,
    ovrHandSkeletonHeader* header) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_GetHandSkeleton");

        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "NOT IMPLEMENTED YET!");
        
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_GetHandSkeleton");
    }

    typedef enum ovrColorSpace_ {
    /// No color correction, not recommended for production use. See notes above for more info
    VRAPI_COLORSPACE_UNMANAGED = 0,
    /// Preferred color space for standardized color across all Oculus HMDs with D65 white point
    VRAPI_COLORSPACE_REC_2020 = 1,
    /// Rec. 709 is used on Oculus Go and shares the same primary color coordinates as sRGB
    VRAPI_COLORSPACE_REC_709 = 2,
    /// Oculus Rift CV1 uses a unique color space, see enum description for more info
    VRAPI_COLORSPACE_RIFT_CV1 = 3,
    /// Oculus Rift S uses a unique color space, see enum description for more info
    VRAPI_COLORSPACE_RIFT_S = 4,
    /// Oculus Quest's native color space is slightly different than Rift CV1
    VRAPI_COLORSPACE_QUEST = 5,
    /// Similar to DCI-P3. See notes above for more details on P3
    VRAPI_COLORSPACE_P3 = 6,
    /// Similar to sRGB but with deeper greens using D65 white point
    VRAPI_COLORSPACE_ADOBE_RGB = 7,
} ovrColorSpace;

    typedef struct ovrHmdColorDesc_ {
    /// See ovrColorSpace for more info.
    ovrColorSpace ColorSpace;
    OVR_VRAPI_PADDING(4)
} ovrHmdColorDesc;

extern ovrHmdColorDesc vrapi_GetHmdColorDesc(ovrMobile* ovr) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_GetHmdColorDesc");

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "NOT IMPLEMENTED YET! Find out if Pico supports getting or only setting");

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_GetHmdColorDesc");

    //TODO: Figure out how to return struct, also implement padding?
}

extern ovrResult
vrapi_GetInputDeviceCapabilities(ovrMobile* ovr, ovrInputCapabilityHeader* capsHeader) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_GetInputDeviceCapabilities");
}

typedef struct ovrRigidBodyPosef_ {
    ovrPosef Pose;
    ovrVector3f AngularVelocity;
    ovrVector3f LinearVelocity;
    ovrVector3f AngularAcceleration;
    ovrVector3f LinearAcceleration;
    OVR_VRAPI_PADDING(4)
    double TimeInSeconds; //< Absolute time of this pose.
    double PredictionInSeconds; //< Seconds this pose was predicted ahead.
} ovrRigidBodyPosef;

/// Bit flags describing the current status of sensor tracking.
typedef enum ovrTrackingStatus_ {
    VRAPI_TRACKING_STATUS_ORIENTATION_TRACKED = 1 << 0, //< Orientation is currently tracked.
    VRAPI_TRACKING_STATUS_POSITION_TRACKED = 1 << 1, //< Position is currently tracked.
    VRAPI_TRACKING_STATUS_ORIENTATION_VALID = 1 << 2, //< Orientation reported is valid.
    VRAPI_TRACKING_STATUS_POSITION_VALID = 1 << 3, //< Position reported is valid.
    VRAPI_TRACKING_STATUS_HMD_CONNECTED = 1 << 7 //< HMD is available & connected.
} ovrTrackingStatus;

typedef struct ovrTracking_ {
    /// Sensor status described by ovrTrackingStatus flags.
    unsigned int Status;

    OVR_VRAPI_PADDING(4)

    /// Predicted head configuration at the requested absolute time.
    /// The pose describes the head orientation and center eye position.
    ovrRigidBodyPosef HeadPose;
} ovrTracking;

extern ovrResult vrapi_GetInputTrackingState(
    ovrMobile* ovr,
    const ovrDeviceID deviceID,
    const double absTimeInSeconds,
    ovrTracking* tracking) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_GetInputTrackingState");
    }

extern ovrResult
vrapi_GetInstanceExtensionsVulkan(char* extensionNames, uint32_t* extensionNamesSize) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_GetInstanceExtensionsVulkan");

    int ret = Pxr_GetInstanceExtensionsVk((const char **)extensionNames, extensionNamesSize);

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_GetInstanceExtensionsVulkan");

    if (ret == 0) {
        return ovrSuccess;
    }

    return ovrError_Unavailable;
}

extern double vrapi_GetPredictedDisplayTime(ovrMobile* ovr, long long frameIndex) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_GetPredictedDisplayTime");

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "Requested Frame: %lld", frameIndex);

    int ret = Pxr_GetPredictedDisplayTime(&predictedDisplayTimeMs);

    if (ret == 0) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "Predicted Display Time: %f", predictedDisplayTimeMs);
    }

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_GetPredictedDisplayTime");

    return predictedDisplayTimeMs;
}

/// One of the user's eyes.
typedef enum ovrEye_ { VRAPI_EYE_LEFT = 0, VRAPI_EYE_RIGHT = 1, VRAPI_EYE_COUNT = 2 } ovrEye;

typedef struct ovrMatrix4f_ {
    float M[4][4];
} ovrMatrix4f;

/// Returns a projection matrix based on the specified dimensions.
/// The projection matrix transforms -Z=forward, +Y=up, +X=right to the appropriate clip space for
/// the graphics API. The far plane is placed at infinity if farZ <= nearZ. An infinite projection
/// matrix is preferred for rasterization because, except for things *right* up against the near
/// plane, it always provides better precision:
///		"Tightening the Precision of Perspective Rendering"
///		Paul Upchurch, Mathieu Desbrun
///		Journal of Graphics Tools, Volume 16, Issue 1, 2012
static inline ovrMatrix4f ovrMatrix4f_CreateProjection(
    const float minX,
    const float maxX,
    float const minY,
    const float maxY,
    const float nearZ,
    const float farZ) {
    const float width = maxX - minX;
    const float height = maxY - minY;
    const float offsetZ = nearZ; // set to zero for a [0,1] clip space

    ovrMatrix4f out;
    if (farZ <= nearZ) {
        // place the far plane at infinity
        out.M[0][0] = 2 * nearZ / width;
        out.M[0][1] = 0;
        out.M[0][2] = (maxX + minX) / width;
        out.M[0][3] = 0;

        out.M[1][0] = 0;
        out.M[1][1] = 2 * nearZ / height;
        out.M[1][2] = (maxY + minY) / height;
        out.M[1][3] = 0;

        out.M[2][0] = 0;
        out.M[2][1] = 0;
        out.M[2][2] = -1;
        out.M[2][3] = -(nearZ + offsetZ);

        out.M[3][0] = 0;
        out.M[3][1] = 0;
        out.M[3][2] = -1;
        out.M[3][3] = 0;
    } else {
        // normal projection
        out.M[0][0] = 2 * nearZ / width;
        out.M[0][1] = 0;
        out.M[0][2] = (maxX + minX) / width;
        out.M[0][3] = 0;

        out.M[1][0] = 0;
        out.M[1][1] = 2 * nearZ / height;
        out.M[1][2] = (maxY + minY) / height;
        out.M[1][3] = 0;

        out.M[2][0] = 0;
        out.M[2][1] = 0;
        out.M[2][2] = -(farZ + offsetZ) / (farZ - nearZ);
        out.M[2][3] = -(farZ * (nearZ + offsetZ)) / (farZ - nearZ);

        out.M[3][0] = 0;
        out.M[3][1] = 0;
        out.M[3][2] = -1;
        out.M[3][3] = 0;
    }
    return out;
}

/// Returns a projection matrix based on the given FOV.
static inline ovrMatrix4f ovrMatrix4f_CreateProjectionFov(
    const float fovDegreesX,
    const float fovDegreesY,
    const float offsetX,
    const float offsetY,
    const float nearZ,
    const float farZ) {
    const float halfWidth = nearZ * tanf(fovDegreesX * (VRAPI_PI / 180.0f * 0.5f));
    const float halfHeight = nearZ * tanf(fovDegreesY * (VRAPI_PI / 180.0f * 0.5f));

    const float minX = offsetX - halfWidth;
    const float maxX = offsetX + halfWidth;

    const float minY = offsetY - halfHeight;
    const float maxY = offsetY + halfHeight;

    return ovrMatrix4f_CreateProjection(minX, maxX, minY, maxY, nearZ, farZ);
}

/// Returns a projection matrix based on the given asymmetric FOV.
static inline ovrMatrix4f ovrMatrix4f_CreateProjectionAsymmetricFov(
    const float leftDegrees,
    const float rightDegrees,
    const float upDegrees,
    const float downDegrees,
    const float nearZ,
    const float farZ) {
    const float minX = -nearZ * tanf(leftDegrees * (VRAPI_PI / 180.0f));
    const float maxX = nearZ * tanf(rightDegrees * (VRAPI_PI / 180.0f));

    const float minY = -nearZ * tanf(downDegrees * (VRAPI_PI / 180.0f));
    const float maxY = nearZ * tanf(upDegrees * (VRAPI_PI / 180.0f));

    return ovrMatrix4f_CreateProjection(minX, maxX, minY, maxY, nearZ, farZ);
}

typedef struct ovrTracking2_ {
    /// Sensor status described by ovrTrackingStatus flags.
    unsigned int Status;

    OVR_VRAPI_PADDING(4)

    /// Predicted head configuration at the requested absolute time.
    /// The pose describes the head orientation and center eye position.
    ovrRigidBodyPosef HeadPose;
    struct {
        ovrMatrix4f ProjectionMatrix;
        ovrMatrix4f ViewMatrix;
    } Eye[VRAPI_EYE_COUNT];
} ovrTracking2;

extern ovrTracking2 vrapi_GetPredictedTracking2(ovrMobile* ovr, double absTimeInSeconds) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_GetPredictedTracking2");

    // TODO: Hacky, but we'll try
    Pxr_BeginFrame();

    int sensorFrameIndex;
    int eyeCount = 2;
    PxrPosef pose[eyeCount];
    PxrSensorState sensorState;

    int ret = Pxr_GetPredictedMainSensorStateWithEyePose(predictedDisplayTimeMs, &sensorState, &sensorFrameIndex, eyeCount, pose);

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "NOT IMPLEMENTED YET!");

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_GetPredictedTracking2");

    ovrTracking2 nil;
    nil.Status = VRAPI_TRACKING_STATUS_POSITION_TRACKED;

    nil.HeadPose.Pose.Orientation.w = pose[0].orientation.w;
    nil.HeadPose.Pose.Orientation.x = pose[0].orientation.x;
    nil.HeadPose.Pose.Orientation.y = pose[0].orientation.y;
    nil.HeadPose.Pose.Orientation.z = pose[0].orientation.z;

    nil.HeadPose.Pose.Position.x = pose[0].position.x;
    nil.HeadPose.Pose.Position.y = pose[0].position.y;
    nil.HeadPose.Pose.Position.z = pose[0].position.z;

    nil.Eye[0].ProjectionMatrix = ovrMatrix4f_CreateProjectionFov(90.0f, 90.0f, 0.0f, 0.0f, 0.1f, 0.0f);
    nil.Eye[1].ProjectionMatrix = ovrMatrix4f_CreateProjectionFov(90.0f, 90.0f, 0.0f, 0.0f, 0.1f, 0.0f);

    // Convert quaternion to 4x4 rotation?

/*    float q0 = pose[0].orientation.x;
    float q1 = pose[0].orientation.y;
    float q2 = pose[0].orientation.z;
    float q3 = pose[0].orientation.w;



    ovrMatrix4f mat1;*/
    /*mat1.M[0][0] = pose[0].orientation.

    nil.Eye[0].ProjectionMatrix*/

    return nil;
}

extern ovrTracking vrapi_GetPredictedTracking(ovrMobile* ovr, double absTimeInSeconds) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_GetPredictedTracking");
}

typedef enum ovrProperty_ {
        VRAPI_FOVEATION_LEVEL = 15, //< Used by apps that want to control swapchain foveation levels.
    
    VRAPI_EAT_NATIVE_GAMEPAD_EVENTS =
        20, //< Used to tell the runtime not to eat gamepad events.  If this is false on a native
    // app, the app must be listening for the events.
        VRAPI_ACTIVE_INPUT_DEVICE_ID = 24, //< Used by apps to query which input device is most 'active'
                                       // or primary, a -1 means no active input device
        VRAPI_DEVICE_EMULATION_MODE = 29, //< Used by apps to determine if they are running in an
                                      // emulation mode. Is a ovrDeviceEmulationMode value

    VRAPI_DYNAMIC_FOVEATION_ENABLED =
        30, //< Used by apps to enable / disable dynamic foveation adjustments.
    } ovrProperty;

/// Returns false if the property cannot be read.
extern bool
vrapi_GetPropertyInt(const ovrJava* java, const ovrProperty propType, int* intVal) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_GetPropertyInt");

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "Tryint to read prop %d", propType);
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "NOT IMPLEMENTED YET!");

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_GetPropertyInt");
}

typedef enum ovrSystemProperty_ {
    VRAPI_SYS_PROP_DEVICE_TYPE = 0,
    VRAPI_SYS_PROP_MAX_FULLSPEED_FRAMEBUFFER_SAMPLES = 1,
    /// Physical width and height of the display in pixels.
    VRAPI_SYS_PROP_DISPLAY_PIXELS_WIDE = 2,
    VRAPI_SYS_PROP_DISPLAY_PIXELS_HIGH = 3,
    /// Returns the refresh rate of the display in cycles per second.
    VRAPI_SYS_PROP_DISPLAY_REFRESH_RATE = 4,
    /// With a display resolution of 2560x1440, the pixels at the center
    /// of each eye cover about 0.06 degrees of visual arc. To wrap a
    /// full 360 degrees, about 6000 pixels would be needed and about one
    /// quarter of that would be needed for ~90 degrees FOV. As such, Eye
    /// images with a resolution of 1536x1536 result in a good 1:1 mapping
    /// in the center, but they need mip-maps for off center pixels. To
    /// avoid the need for mip-maps and for significantly improved rendering
    /// performance this currently returns a conservative 1024x1024.
    VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH = 5,
    VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT = 6,
    /// This is a product of the lens distortion and the screen size,
    /// but there is no truly correct answer.
    /// There is a tradeoff in resolution and coverage.
    /// Too small of an FOV will leave unrendered pixels visible, but too
    /// large wastes resolution or fill rate.  It is unreasonable to
    /// increase it until the corners are completely covered, but we do
    /// want most of the outside edges completely covered.
    /// Applications might choose to render a larger FOV when angular
    /// acceleration is high to reduce black pull in at the edges by
    /// the time warp.
    /// Currently symmetric 90.0 degrees.
    VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_X = 7,
    VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_Y = 8,
    VRAPI_SYS_PROP_DEVICE_REGION = 10,
    /// Returns an ovrHandedness enum indicating left or right hand.
    VRAPI_SYS_PROP_DOMINANT_HAND = 15,

    /// Returns VRAPI_TRUE if the system supports orientation tracking.
    VRAPI_SYS_PROP_HAS_ORIENTATION_TRACKING = 16,
    /// Returns VRAPI_TRUE if the system supports positional tracking.
    VRAPI_SYS_PROP_HAS_POSITION_TRACKING = 17,

    /// Returns the number of display refresh rates supported by the system.
    VRAPI_SYS_PROP_NUM_SUPPORTED_DISPLAY_REFRESH_RATES = 64,
    /// Returns an array of the supported display refresh rates.
    VRAPI_SYS_PROP_SUPPORTED_DISPLAY_REFRESH_RATES = 65,

    /// Returns the number of swapchain texture formats supported by the system.
    VRAPI_SYS_PROP_NUM_SUPPORTED_SWAPCHAIN_FORMATS = 66,
    /// Returns an array of the supported swapchain formats.
    /// Formats are platform specific. For GLES, this is an array of
    /// GL internal formats.
    VRAPI_SYS_PROP_SUPPORTED_SWAPCHAIN_FORMATS = 67,
        /// Returns VRAPI_TRUE if on-chip foveated rendering of swapchains is supported
    /// for this system, otherwise VRAPI_FALSE.
    VRAPI_SYS_PROP_FOVEATION_AVAILABLE = 130,
    } ovrSystemProperty;

/// Returns a system property. These are constants for a particular device.
/// These functions can be called any time from any thread once the VrApi is initialized.
// VRAPI_DEVICE_TYPE_OCULUSQUEST2
extern int vrapi_GetSystemPropertyInt(
    const ovrJava* java,
    const ovrSystemProperty propType) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_GetSystemPropertyInt");
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "%d", propType);

        int ret = 0;

        switch (propType) {
            case VRAPI_SYS_PROP_DEVICE_TYPE:
                ret = VRAPI_DEVICE_TYPE_OCULUSQUEST2; //Mock Quest 2 (closest to Pico Neo 3, almost identically actually)
                break;
            case VRAPI_SYS_PROP_DISPLAY_REFRESH_RATE:
                {
                    float refreshRate = 90;
                    Pxr_GetDisplayRefreshRate(&refreshRate);
                    ret = (int)refreshRate;
                }
                break;
            case VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH:
                {
                    uint32_t maxW,maxH,recommendW,recommendH;
                    Pxr_GetConfigViewsInfos(&maxW,&maxH,&recommendW,&recommendH);
                    ret = recommendW;
                }
                break;
                
            case VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT:
                {
                    uint32_t maxW,maxH,recommendW,recommendH;
                    Pxr_GetConfigViewsInfos(&maxW,&maxH,&recommendW,&recommendH);
                    ret = recommendH;
                }
                break;
            case VRAPI_SYS_PROP_DISPLAY_PIXELS_WIDE:
                {
                    ret = (int)3664; //1832
                }
                break;
            case VRAPI_SYS_PROP_DISPLAY_PIXELS_HIGH:
                {
                    ret = (int)1920;
                }
                break;
            default:
                __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "Unhandled propType = %d", propType);
                ret = 0;
        }

        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "Returning %d for propType %d", ret, propType);

        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_GetSystemPropertyInt");

        return ret;
    }

extern float vrapi_GetSystemPropertyFloat(
    const ovrJava* java,
    const ovrSystemProperty propType) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_GetSystemPropertyFloat");
    }

/// Returns the number of elements written to values array.
extern int vrapi_GetSystemPropertyFloatArray(
    const ovrJava* java,
    const ovrSystemProperty propType,
    float* values,
    int numArrayValues) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_GetSystemPropertyFloatArray");
    }

extern int vrapi_GetSystemPropertyInt64Array(
    const ovrJava* java,
    const ovrSystemProperty propType,
    int64_t* values,
    int numArrayValues) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_GetSystemPropertyInt64Array");
    }

/// The return memory is guaranteed to be valid until the next call to
/// vrapi_GetSystemPropertyString.
extern const char* vrapi_GetSystemPropertyString(
    const ovrJava* java,
    const ovrSystemProperty propType) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_GetSystemPropertyString");
    }

/// System status bits.
typedef enum ovrSystemStatus_ {
    // enum 0 used to be VRAPI_SYS_STATUS_DOCKED.
    VRAPI_SYS_STATUS_MOUNTED = 1, //< Device is mounted.
    VRAPI_SYS_STATUS_THROTTLED = 2, //< Device is in powersave mode.

    // enum  3 used to be VRAPI_SYS_STATUS_THROTTLED2.

    // enum  4 used to be VRAPI_SYS_STATUS_THROTTLED_WARNING_LEVEL.

    VRAPI_SYS_STATUS_RENDER_LATENCY_MILLISECONDS =
        5, //< Average time between render tracking sample and scanout.
    VRAPI_SYS_STATUS_TIMEWARP_LATENCY_MILLISECONDS =
        6, //< Average time between timewarp tracking sample and scanout.
    VRAPI_SYS_STATUS_SCANOUT_LATENCY_MILLISECONDS = 7, //< Average time between Vsync and scanout.
    VRAPI_SYS_STATUS_APP_FRAMES_PER_SECOND =
        8, //< Number of frames per second delivered through vrapi_SubmitFrame.
    VRAPI_SYS_STATUS_SCREEN_TEARS_PER_SECOND = 9, //< Number of screen tears per second (per eye).
    VRAPI_SYS_STATUS_EARLY_FRAMES_PER_SECOND =
        10, //< Number of frames per second delivered a whole display refresh early.
    VRAPI_SYS_STATUS_STALE_FRAMES_PER_SECOND = 11, //< Number of frames per second delivered late.

    // enum 12 used to be VRAPI_SYS_STATUS_HEADPHONES_PLUGGED_IN

    VRAPI_SYS_STATUS_RECENTER_COUNT = 13, //< Returns the current HMD recenter count. Defaults to 0.
    // enum 14 used to be VRAPI_SYS_STATUS_SYSTEM_UX_ACTIVE
    VRAPI_SYS_STATUS_USER_RECENTER_COUNT = 15, //< Returns the current HMD recenter count for user
                                               // initiated recenters only. Defaults to 0.

    
        VRAPI_SYS_STATUS_FRONT_BUFFER_SRGB =
        130, //< VRAPI_TRUE if the front buffer uses the sRGB color space.

    VRAPI_SYS_STATUS_SCREEN_CAPTURE_RUNNING =
        131, // VRAPI_TRUE if the screen is currently being recorded.

    } ovrSystemStatus;

    /// Returns a system status. These are variables that may change at run-time.
/// This function can be called any time from any thread once the VrApi is initialized.
extern int vrapi_GetSystemStatusInt(
    const ovrJava* java,
    const ovrSystemStatus statusType) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_GetSystemStatusInt");
    }

extern float vrapi_GetSystemStatusFloat(
    const ovrJava* java,
    const ovrSystemStatus statusType) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_GetSystemStatusFloat");
    }

extern jobject vrapi_GetTextureSwapChainAndroidSurface(ovrTextureSwapChain* chain) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_GetTextureSwapChainAndroidSurface");
}

/// Get the foveation VkImage and corresponding size at the given index within the chain.
/// In case of failure, this returns a null image handle and zero width and height.
extern ovrResult vrapi_GetTextureSwapChainBufferFoveationVulkan(
    ovrTextureSwapChain* chain,
    int index,
    VkImage* image,
    uint32_t* imageWidth,
    uint32_t* imageHeight) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_GetTextureSwapChainBufferFoveationVulkan");
    }

    /// Get the VkImage at the given index within the chain.
extern VkImage
vrapi_GetTextureSwapChainBufferVulkan(ovrTextureSwapChain* chain, int index) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_GetTextureSwapChainBufferVulkan");
}

/// Returns the number of textures in the swap chain.
extern int vrapi_GetTextureSwapChainLength(ovrTextureSwapChain* chain) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_GetTextureSwapChainLength");

    uint32_t num = 0;

    int ret = Pxr_GetLayerImageCount(0, PXR_EYE_LEFT, &num);

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "SwapChainLength for id %d and eye %d = %d", 0, PXR_EYE_LEFT, num);

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_GetTextureSwapChainLength");

    if ( ret == 0) {
        return num;
    }

    return 0;
}

/// Get the OpenGL name of the texture at the given index.
extern unsigned int vrapi_GetTextureSwapChainHandle(
    ovrTextureSwapChain* chain,
    int index) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_GetTextureSwapChainHandle");

        int imageIndex = 0;
        Pxr_GetLayerNextImageIndex(0, &imageIndex);

        chain = &_chain;

        return 0;
    }

/// Returns global, absolute high-resolution time in seconds. This is the same value
/// as used in sensor messages and on Android also the same as Java's system.nanoTime(),
/// which is what the V-sync timestamp is based on.
/// \warning Do not use this time as a seed for simulations, animations or other logic.
/// An animation, for instance, should not be updated based on the "real time" the
/// animation code is executed. Instead, an animation should be updated based on the
/// time it will be displayed. Using the "real time" will introduce intra-frame motion
/// judder when the code is not executed at a consistent point in time every frame.
/// In other words, for simulations, animations and other logic use the time returned
/// by vrapi_GetPredictedDisplayTime().
/// Can be called any time from any thread.
extern double vrapi_GetTimeInSeconds() {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_GetTimeInSeconds");

    double time = now_ms();
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "Time: %f", time);

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_GetTimeInSeconds");

    return time;
}

typedef enum ovrTrackingSpace_ {
    VRAPI_TRACKING_SPACE_LOCAL = 0, // Eye level origin - controlled by system recentering
    VRAPI_TRACKING_SPACE_LOCAL_FLOOR = 1, // Floor level origin - controlled by system recentering
    VRAPI_TRACKING_SPACE_LOCAL_TILTED =
        2, // Tilted pose for "bed mode" - controlled by system recentering
    VRAPI_TRACKING_SPACE_STAGE = 3, // Floor level origin - controlled by Guardian setup
        VRAPI_TRACKING_SPACE_LOCAL_FIXED_YAW = 7, // Position of local space, but yaw stays constant
} ovrTrackingSpace;

/// Returns the current tracking space
extern ovrTrackingSpace vrapi_GetTrackingSpace(ovrMobile* ovr) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_GetTrackingSpace");
}

/// The TrackingTransform API has been deprecated because it was superceded by the
/// TrackingSpace API. The key difference in the TrackingSpace API is that LOCAL
/// and LOCAL_FLOOR spaces are mutable, so user/system recentering is transparently
/// applied without app intervention.
//OVR_VRAPI_DEPRECATED(OVR_VRAPI_EXPORT ovrPosef vrapi_GetTrackingTransform(
//    ovrMobile* ovr,
//    ovrTrackingTransform whichTransform));

char *version = "1.1.40.0-0-0 Nov  4 2020 16:13:28";
// 1.1.40.0-0-0 Nov  4 2020 16:13:28

/// Returns the version + compile time stamp as a string.
/// Can be called any time from any thread.
extern const char* vrapi_GetVersionString() {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_GetVersionString");

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "TODO: Find out what the string is supposed to return?");

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_GetVersionString");

    return version;
}

/// Shut everything down for window destruction or when the activity is paused.
/// The ovrMobile object is freed by this function.
///
/// Must be called from the same thread that called vrapi_EnterVrMode(). If the
/// application did not explicitly pass in the Android window surface, then this
/// thread *must* have the same OpenGL ES context that was current on the Android
/// window surface before calling vrapi_EnterVrMode(). By calling this function,
/// the time warp gives up ownership of the Android window surface, and on return,
/// the context from the calling thread will be current again on the Android window
/// surface.
extern void vrapi_LeaveVrMode(ovrMobile* ovr) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_LeaveVrMode");

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_LeaveVrMode");
}

/// Returns pose of the requested space relative to the current space.
/// The returned value is not affected by the current tracking transform.
extern ovrPosef vrapi_LocateTrackingSpace(ovrMobile* ovr, ovrTrackingSpace target) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_LocateTrackingSpace");
}

typedef enum ovrEventType_ {
    // No event. This is returned if no events are pending.
    VRAPI_EVENT_NONE = 0,
    // Events were lost due to event queue overflow.
    VRAPI_EVENT_DATA_LOST = 1,
    // The application's frames are visible to the user.
    VRAPI_EVENT_VISIBILITY_GAINED = 2,
    // The application's frames are no longer visible to the user.
    VRAPI_EVENT_VISIBILITY_LOST = 3,
    // The current activity is in the foreground and has input focus.
    VRAPI_EVENT_FOCUS_GAINED = 4,
    // The current activity is in the background (but possibly still visible) and has lost input
    // focus.
    VRAPI_EVENT_FOCUS_LOST = 5,
            // The display refresh rate has changed
    VRAPI_EVENT_DISPLAY_REFRESH_RATE_CHANGE = 11,
} ovrEventType;

typedef struct ovrEventHeader_ {
    ovrEventType EventType;
} ovrEventHeader;

/// Returns VrApi state information to the application.
/// The application should read from the VrApi event queue with regularity.
///
/// The caller must pass a pointer to memory that is at least the size of the largest event
/// structure, VRAPI_LARGEST_EVENT_TYPE. On return, the structure is filled in with the current
/// event's data. All event structures start with the ovrEventHeader, which contains the
/// type of the event. Based on this type, the caller can cast the passed ovrEventHeader
/// pointer to the appropriate event type.
///
/// Returns ovrSuccess if no error occured.
/// If no events are pending the event header EventType will be VRAPI_EVENT_NONE.
extern ovrResult vrapi_PollEvent(ovrEventHeader* event) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_PollEvent");
}

/// Can be called from any thread while in VR mode. Recenters the tracked remote to the current yaw
/// of the headset. Input: ovr, device ID Output: None
//OVR_VRAPI_DEPRECATED(
//    OVR_VRAPI_EXPORT void vrapi_RecenterInputPose(ovrMobile* ovr, const ovrDeviceID deviceID));

// vrapi_RecenterPose() is being deprecated because it is supported at the user
// level via system interaction, and at the app level, the app is free to use
// any means it likes to control the mapping of virtual space to physical space.
//OVR_VRAPI_DEPRECATED(OVR_VRAPI_EXPORT void vrapi_RecenterPose(ovrMobile* ovr));

/// Used to force Guardian System mesh visibility to true.  Forcing to false will set the Guardian
/// System back to normal operation.
extern ovrResult vrapi_RequestBoundaryVisible(ovrMobile* ovr, const bool visible) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_RequestBoundaryVisible");
}

/// Sets the color space actively being used by the client app.
///
/// This value does not have to follow the color space provided in ovr_GetHmdColorDesc. It should
/// reflect the color space used in the final rendered frame the client has submitted to the SDK.
/// If this function is never called, the session will keep using the default color space deemed
/// appropriate by the runtime. See remarks in ovrColorSpace enum for more info on default behavior.
///
/// \return Returns an ovrResult indicating success or failure.
extern ovrResult
vrapi_SetClientColorDesc(ovrMobile* ovr, const ovrHmdColorDesc* colorDesc) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_SetClientColorDesc");
}

/// Set the CPU and GPU performance levels.
///
/// Increasing the levels increases performance at the cost of higher power consumption
/// which likely leads to a greater chance of overheating.
///
/// Levels will be clamped to the expected range. Default clock levels are cpuLevel = 2, gpuLevel
/// = 2.
extern ovrResult
vrapi_SetClockLevels(ovrMobile* ovr, const int32_t cpuLevel, const int32_t gpuLevel) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_SetClockLevels");

    //TODO: Find out if levels are the same between APIs
    int a = Pxr_SetPerformanceLevels(PXR_PERF_SETTINGS_CPU, cpuLevel);
    int b = Pxr_SetPerformanceLevels(PXR_PERF_SETTINGS_GPU, gpuLevel);

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_SetClockLevels");

    if(a == 0 && b == 0) {
        return ovrSuccess;
    }

    return ovrError_InvalidParameter;
}

/// Set the Display Refresh Rate.
/// Returns ovrSuccess or an ovrError code.
/// Returns 'ovrError_InvalidParameter' if requested refresh rate is not supported by the device.
/// Returns 'ovrError_InvalidOperation' if the display refresh rate request was not allowed (such as
/// when the device is in low power mode).
extern ovrResult vrapi_SetDisplayRefreshRate(ovrMobile* ovr, const float refreshRate) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_SetDisplayRefreshRate");

    ovrResult result;

    int ret = Pxr_SetDisplayRefreshRate(refreshRate);
    if(ret == 0) {
        result = ovrSuccess;
    }
    else {
        result = ovrError_NotInitialized;
    }

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_SetDisplayRefreshRate");

    return result;
}

typedef enum ovrExtraLatencyMode_ {
    VRAPI_EXTRA_LATENCY_MODE_OFF = 0,
    VRAPI_EXTRA_LATENCY_MODE_ON = 1,
    VRAPI_EXTRA_LATENCY_MODE_DYNAMIC = 2
} ovrExtraLatencyMode;

/// If VRAPI_EXTRA_LATENCY_MODE_ON specified, adds an extra frame of latency for full GPU
/// utilization. Default is VRAPI_EXTRA_LATENCY_MODE_OFF.
///
/// The latency mode specified will be applied on the next call to vrapi_SubmitFrame2().
extern ovrResult
vrapi_SetExtraLatencyMode(ovrMobile* ovr, const ovrExtraLatencyMode mode) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_SetExtraLatencyMode");
}

/// The buffer data for playing haptics
typedef struct ovrHapticBuffer_ {
    /// Start time of the buffer
    double BufferTime;

    /// Number of samples in the buffer;
    uint32_t NumSamples;

    // True if this is the end of the buffers being sent
    bool Terminated;

    uint8_t* HapticBuffer;
} ovrHapticBuffer;

/// Fills the haptic vibration buffer of a haptic device
/// there should only be one call to vrapi_SetHapticVibrationSimple or
/// vrapi_SetHapticVibrationBuffer per frame
///  additional calls of either will return ovrError_InvalidOperation and have undefined behavior
/// Input: ovr, deviceID, pointer to a hapticBuffer with filled in data.
extern ovrResult vrapi_SetHapticVibrationBuffer(
    ovrMobile* ovr,
    const ovrDeviceID deviceID,
    const ovrHapticBuffer* hapticBuffer) {

    }

/// Sets the vibration level of a haptic device.
/// there should only be one call to vrapi_SetHapticVibrationSimple or
/// vrapi_SetHapticVibrationBuffer per frame
///  additional calls of either will return ovrError_InvalidOperation and have undefined behavior
/// Input: ovr, deviceID, intensity: 0.0 - 1.0
extern ovrResult
vrapi_SetHapticVibrationSimple(ovrMobile* ovr, const ovrDeviceID deviceID, const float intensity) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_SetHapticVibrationSimple");
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "Vibrating device %d with intensity %f", deviceID, intensity);
    
    // TODO: Map DeviceIDs to Controller Left/Right

    Pxr_SetControllerVibration(PXR_CONTROLLER_RIGHT, intensity, 20);

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_SetHapticVibrationSimple");
}

/// Identifies a VR-related application thread.
typedef enum ovrPerfThreadType_ {
    VRAPI_PERF_THREAD_TYPE_MAIN = 0,
    VRAPI_PERF_THREAD_TYPE_RENDERER = 1,
} ovrPerfThreadType;

/// Specify which app threads should be given higher scheduling priority.
extern ovrResult
vrapi_SetPerfThread(ovrMobile* ovr, const ovrPerfThreadType type, const uint32_t threadId) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_SetPerfThread");

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "Application requests threadId %d (type = %d) to have higher priority", threadId, type);
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "NOT IMPLEMENTED YET!");

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_SetPerfThread");

    return ovrSuccess;
}

/// Returns a VrApi property.
/// These functions can be called any time from any thread once the VrApi is initialized.
extern void
vrapi_SetPropertyInt(const ovrJava* java, const ovrProperty propType, const int intVal) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_SetPropertyInt");
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "%d = %d", propType, intVal);

    switch(propType) {
        case VRAPI_FOVEATION_LEVEL:
            __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "Unhandled VRAPI_FOVEATION_LEVEL");
            break;

        case VRAPI_EAT_NATIVE_GAMEPAD_EVENTS:
            __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "Unhandled VRAPI_EAT_NATIVE_GAMEPAD_EVENTS");
            break;

        case VRAPI_ACTIVE_INPUT_DEVICE_ID:
            __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "Unhandled VRAPI_ACTIVE_INPUT_DEVICE_ID");
            break;

        case VRAPI_DEVICE_EMULATION_MODE:
            __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "Unhandled VRAPI_DEVICE_EMULATION_MODE");
            break;

        case VRAPI_DYNAMIC_FOVEATION_ENABLED:
            __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "Unhandled VRAPI_DYNAMIC_FOVEATION_ENABLED");
            break;

        default:
            __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "Unhandled UKNOWN PROP (%d)", intVal);
    }

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_SetPropertyInt");
}

extern void
vrapi_SetPropertyFloat(const ovrJava* java, const ovrProperty propType, const float floatVal) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_SetPropertyFloat");
}

extern ovrResult vrapi_SetTextureSwapChainSamplerState(
    ovrTextureSwapChain* chain,
    const ovrTextureSamplerState* samplerState) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_SetTextureSwapChainSamplerState");
    }

/// Set the tracking space. There are currently two options:
///   * VRAPI_TRACKING_SPACE_LOCAL (default)
///         The local tracking space's origin is at the nominal head position
///         with +y up, and -z forward. This space is volatile and will change
///         when system recentering occurs.
///   * VRAPI_TRACKING_SPACE_LOCAL_FLOOR
///         The local floor tracking space is the same as the local tracking
///         space, except its origin is translated down to the floor. The local
///         floor space differs from the local space only in its y translation.
///         This space is volatile and will change when system recentering occurs.
extern ovrResult vrapi_SetTrackingSpace(ovrMobile* ovr, ovrTrackingSpace whichSpace) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_SetTrackingSpace");

    int pico_mode = 0;

    switch(whichSpace) {
        case VRAPI_TRACKING_SPACE_LOCAL:
            pico_mode = PXR_EYE_LEVEL;
            break;

        case VRAPI_TRACKING_SPACE_LOCAL_FLOOR:
            pico_mode = PXR_FLOOR_LEVEL;
            break;

        case VRAPI_TRACKING_SPACE_LOCAL_TILTED:
            pico_mode = PXR_FLOOR_LEVEL; // Pico supports tilted/bed mode, but the enum currently does not reflect that?
            break;

        case VRAPI_TRACKING_SPACE_STAGE:
            pico_mode = PXR_STAGE_LEVEL;
            break;

        case VRAPI_TRACKING_SPACE_LOCAL_FIXED_YAW:
        default:
            __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "trackingSpace %d not supportedl,using PXR_EYE_LEVEL/VRAPI_TRACKING_SPACE_LOCAL", VRAPI_TRACKING_SPACE_LOCAL_FIXED_YAW);
            pico_mode = PXR_EYE_LEVEL;
            break;
    }

    int ret = Pxr_SetTrackingOrigin(pico_mode);

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "Tracking Space = %d", whichSpace);

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_SetTrackingSpace");

    if (ret == 0) {
        return ovrSuccess;
    }

    return ovrError_InvalidParameter;
}

// Sets the transform used to convert between tracking coordinates and a canonical
/// application-defined space.
/// Only the yaw component of the orientation is used.
//OVR_VRAPI_DEPRECATED(
//    OVR_VRAPI_EXPORT void vrapi_SetTrackingTransform(ovrMobile* ovr, ovrPosef pose));

//OVR_VRAPI_DEPRECATED(OVR_VRAPI_EXPORT void vrapi_ShowFatalError(
//    const ovrJava* java,
//    const char* title,
//    const char* message,
//    const char* fileName,
//    const unsigned int lineNumber));

typedef enum ovrSystemUIType_ {
    VRAPI_SYS_UI_CONFIRM_QUIT_MENU = 1, // Display the 'Confirm Quit' Menu.
    } ovrSystemUIType;

/// Display a specific System UI.
extern bool vrapi_ShowSystemUI(const ovrJava* java, const ovrSystemUIType type) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_ShowSystemUI");
}

/// Shuts down the API on application exit.
/// This is typically called from onDestroy() or shortly thereafter.
/// Can be called from any thread.
extern void vrapi_Shutdown() {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "vrapi_Shutdown");
}

/// \deprecated The vrapi_SubmitFrame2 path with flexible layer types should be used instead.
//OVR_VRAPI_DEPRECATED(
//    OVR_VRAPI_EXPORT void vrapi_SubmitFrame(ovrMobile* ovr, const ovrFrameParms* parms));


enum { ovrMaxLayerCount = 16 };

/// A layer type.
typedef enum ovrLayerType2_ {
    VRAPI_LAYER_TYPE_PROJECTION2 = 1,
        VRAPI_LAYER_TYPE_CYLINDER2 = 3,
    VRAPI_LAYER_TYPE_CUBE2 = 4,
    VRAPI_LAYER_TYPE_EQUIRECT2 = 5,
    VRAPI_LAYER_TYPE_LOADING_ICON2 = 6,
    VRAPI_LAYER_TYPE_FISHEYE2 = 7,
        VRAPI_LAYER_TYPE_EQUIRECT3 = 10,
    } ovrLayerType2;


/// A 4D vector.
typedef struct ovrVector4f_ {
    float x, y, z, w;
} ovrVector4f;

/// Selects an operation for alpha blending two images.
typedef enum ovrFrameLayerBlend_ {
    VRAPI_FRAME_LAYER_BLEND_ZERO = 0,
    VRAPI_FRAME_LAYER_BLEND_ONE = 1,
    VRAPI_FRAME_LAYER_BLEND_SRC_ALPHA = 2,
        VRAPI_FRAME_LAYER_BLEND_ONE_MINUS_SRC_ALPHA = 5
} ovrFrameLayerBlend;

/// Properties shared by any type of layer.
typedef struct ovrLayerHeader2_ {
    ovrLayerType2 Type;
    /// Combination of ovrFrameLayerFlags flags.
    uint32_t Flags;

    ovrVector4f ColorScale;
    ovrFrameLayerBlend SrcBlend;
    ovrFrameLayerBlend DstBlend;
        /// \unused parameter.
    void* Reserved;
} ovrLayerHeader2;


/// A rectangle with 2D size and position.
typedef struct ovrRectf_ {
    float x;
    float y;
    float width;
    float height;
} ovrRectf;

/// The user's eye (left or right) that can see a layer.
typedef enum ovrFrameLayerEye_ {
    VRAPI_FRAME_LAYER_EYE_LEFT = 0,
    VRAPI_FRAME_LAYER_EYE_RIGHT = 1,
    VRAPI_FRAME_LAYER_EYE_MAX = 2
} ovrFrameLayerEye;

/// ovrLayerProjection2 provides support for a typical world view layer.
/// \note Any layer textures that are dynamic must be triple buffered.
typedef struct ovrLayerProjection2_ {
    /// Header.Type must be VRAPI_LAYER_TYPE_PROJECTION2.
    ovrLayerHeader2 Header;
    OVR_VRAPI_PADDING_32_BIT(4)

    ovrRigidBodyPosef HeadPose;

    struct {
        ovrTextureSwapChain* ColorSwapChain;
        int SwapChainIndex;
        ovrMatrix4f TexCoordsFromTanAngles;
        ovrRectf TextureRect;
    } Textures[VRAPI_FRAME_LAYER_EYE_MAX];
} ovrLayerProjection2;

typedef struct ovrLayerCylinder2_ {
    /// Header.Type must be VRAPI_LAYER_TYPE_CYLINDER2.
    ovrLayerHeader2 Header;
    OVR_VRAPI_PADDING_32_BIT(4)

    ovrRigidBodyPosef HeadPose;

    struct {
        /// Texture type used to create the swapchain must be a 2D target (VRAPI_TEXTURE_TYPE_2D_*).
        ovrTextureSwapChain* ColorSwapChain;
        int SwapChainIndex;
        ovrMatrix4f TexCoordsFromTanAngles;
        ovrRectf TextureRect;
        /// \note textureMatrix is set up like the following:
        /// sx,  0, tx, 0
        /// 0,  sy, ty, 0
        ///	0,   0,  1, 0
        ///	0,   0,  0, 1
        /// since we do not need z coord for mapping to 2d texture.
        ovrMatrix4f TextureMatrix;
    } Textures[VRAPI_FRAME_LAYER_EYE_MAX];
} ovrLayerCylinder2;

typedef struct ovrLayerCube2_ {
    /// Header.Type must be VRAPI_LAYER_TYPE_CUBE2.
    ovrLayerHeader2 Header;
    OVR_VRAPI_PADDING_32_BIT(4)

    ovrRigidBodyPosef HeadPose;
    ovrMatrix4f TexCoordsFromTanAngles;

    ovrVector3f Offset;

    struct {
        /// Texture type used to create the swapchain must be a cube target
        /// (VRAPI_TEXTURE_TYPE_CUBE).
        ovrTextureSwapChain* ColorSwapChain;
        int SwapChainIndex;
    } Textures[VRAPI_FRAME_LAYER_EYE_MAX];
#ifdef __i386__
    uint32_t Padding;
#endif
} ovrLayerCube2;

typedef struct ovrLayerEquirect2_ {
    /// Header.Type must be VRAPI_LAYER_TYPE_EQUIRECT2.
    ovrLayerHeader2 Header;
    OVR_VRAPI_PADDING_32_BIT(4)

    ovrRigidBodyPosef HeadPose;
    ovrMatrix4f TexCoordsFromTanAngles;

    struct {
        /// Texture type used to create the swapchain must be a 2D target (VRAPI_TEXTURE_TYPE_2D_*).
        ovrTextureSwapChain* ColorSwapChain;
        int SwapChainIndex;
        ovrRectf TextureRect;
        /// \note textureMatrix is set up like the following:
        ///	sx,  0, tx, 0
        ///	0,  sy, ty, 0
        ///	0,   0,  1, 0
        ///	0,   0,  0, 1
        /// since we do not need z coord for mapping to 2d texture.
        ovrMatrix4f TextureMatrix;
    } Textures[VRAPI_FRAME_LAYER_EYE_MAX];
} ovrLayerEquirect2;

typedef struct ovrLayerEquirect3_ {
    /// Header.Type must be VRAPI_LAYER_TYPE_EQUIRECT3.
    ovrLayerHeader2 Header;
    OVR_VRAPI_PADDING_32_BIT(4)

    ovrRigidBodyPosef HeadPose;

    struct {
        /// Texture type used to create the swapchain must be a 2D target (VRAPI_TEXTURE_TYPE_2D_*).
        ovrTextureSwapChain* ColorSwapChain;
        int SwapChainIndex;
        ovrMatrix4f TexCoordsFromTanAngles;
        ovrRectf TextureRect;
        /// \note textureMatrix is set up like the following:
        ///	sx,  0, tx, 0
        ///	0,  sy, ty, 0
        ///	0,   0,  1, 0
        ///	0,   0,  0, 1
        /// since we do not need z coord for mapping to 2d texture.
        ovrMatrix4f TextureMatrix;
    } Textures[VRAPI_FRAME_LAYER_EYE_MAX];
} ovrLayerEquirect3;

/// ovrLayerLoadingIcon2 provides support for a monoscopic spinning layer.
///
typedef struct ovrLayerLoadingIcon2_ {
    /// Header.Type must be VRAPI_LAYER_TYPE_LOADING_ICON2.
    ovrLayerHeader2 Header;

    float SpinSpeed; //< radians per second
    float SpinScale;

    /// Only monoscopic texture supported for spinning layer.
    ovrTextureSwapChain* ColorSwapChain;
    int SwapChainIndex;
} ovrLayerLoadingIcon2;

typedef struct ovrLayerFishEye2_ {
    /// Header.Type must be VRAPI_LAYER_TYPE_FISHEYE2.
    ovrLayerHeader2 Header;
    OVR_VRAPI_PADDING_32_BIT(4)

    ovrRigidBodyPosef HeadPose;

    struct {
        ovrTextureSwapChain* ColorSwapChain;
        int SwapChainIndex;
        ovrMatrix4f LensFromTanAngles; //< transforms a tanAngle ray into lens space
        ovrRectf TextureRect; //< packed stereo images will need to clamp at the mid border
        ovrMatrix4f TextureMatrix; //< transform from a -1 to 1 ideal fisheye to the texture
        ovrVector4f Distortion; //< Not currently used.
    } Textures[VRAPI_FRAME_LAYER_EYE_MAX];
} ovrLayerFishEye2;

/// Union that combines ovrLayer types in a way that allows them
/// to be used in a polymorphic way.
typedef union ovrLayer_Union2_ {
    ovrLayerHeader2 Header;
    ovrLayerProjection2 Projection;
        ovrLayerCylinder2 Cylinder;
    ovrLayerCube2 Cube;
    ovrLayerEquirect2 Equirect;
    ovrLayerEquirect3 Equirect3;
    ovrLayerLoadingIcon2 LoadingIcon;
    ovrLayerFishEye2 FishEye;
    } ovrLayer_Union2;

/// Parameters for frame submission.
typedef struct ovrSubmitFrameDescription2_ {
    /// Combination of ovrFrameFlags flags.
    uint32_t Flags;
    uint32_t SwapInterval;
    uint64_t FrameIndex;
    double DisplayTime;
            /// \unused parameter.
        unsigned char Pad[8];
            uint32_t LayerCount;
    const ovrLayerHeader2* const* Layers;
} ovrSubmitFrameDescription2;

/// vrapi_SubmitFrame2 takes a frameDescription describing per-frame information such as:
/// a flexible list of layers which should be drawn this frame and a frame index.
extern ovrResult
vrapi_SubmitFrame2(ovrMobile* ovr, const ovrSubmitFrameDescription2* frameDescription) {
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_SubmitFrame2");

    PxrLayerHeader head;
    head.layerId = 0;

    int ret = Pxr_SubmitLayer(&head);

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "NOT IMPLEMENTED YET!");
    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "ret = %d", ret);

    __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_SubmitFrame2");

    // TODO: Remove either this or submitlayer
    Pxr_EndFrame();

    if (ret == 0) {
        return ovrSuccess;
    }

    return ovrError_InvalidParameter;
}


/// Tests collision/proximity of a 3D point against the Guardian System Boundary and returns whether
/// or not a given point is inside or outside of the boundary.  If a more detailed set of boundary
/// trigger information is requested a ovrBoundaryTriggerResult may be passed in.  However null may
/// also be passed in to just return whether a point is inside the boundary or not.
/*

typedef struct PxrBoundaryTriggerInfo_ {
	bool                  isTriggering;
    float                 closestDistance;
	PxrVector3f           closestPoint;
	PxrVector3f           closestPointNormal;
	bool                  valid;
}PxrBoundaryTriggerInfo;

*/

PxrBoundaryTriggerInfo btinfo;

extern ovrResult vrapi_TestPointIsInBoundary(
    ovrMobile* ovr,
    const ovrVector3f point,
    bool* pointInsideBoundary,
    ovrBoundaryTriggerResult* result) {
        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "+vrapi_TextPointIsInBoundary");

        *pointInsideBoundary = false;
        PxrVector3f _point;
        _point.x = point.x;
        _point.y = point.y;
        _point.z = point.z;

        int ret = Pxr_TestPointIsInBoundary(&_point, true, &btinfo);

        __android_log_print(ANDROID_LOG_DEBUG, "piccolo", "-vrapi_TextPointIsInBoundary");

        if (ret == 0) {
            *pointInsideBoundary = true;
            return ovrSuccess;
        }

        return ovrError_Unavailable;
    }