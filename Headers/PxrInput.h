#ifndef PXR_INPUT_H
#define PXR_INPUT_H

#include "PxrTypes.h"
#include "PxrEnums.h"

typedef enum
{
    PXR_CONTROLLER_3DOF = 0,
    PXR_CONTROLLER_6DOF
} PxrControllerDof;

typedef enum
{
    PXR_CONTROLLER_BOND = 0,
    PXR_CONTROLLER_UNBOND
} PxrControllerBond;

typedef enum
{
    PXR_CONTROLLER_KEY_HOME = 0,
    PXR_CONTROLLER_KEY_AX = 1,
    PXR_CONTROLLER_KEY_BY= 2,
    PXR_CONTROLLER_KEY_BACK = 3,
    PXR_CONTROLLER_KEY_TRIGGER = 4,
    PXR_CONTROLLER_KEY_VOL_UP = 5,
    PXR_CONTROLLER_KEY_VOL_DOWN = 6,
    PXR_CONTROLLER_KEY_ROCKER = 7,
    PXR_CONTROLLER_KEY_GRIP = 8,
    PXR_CONTROLLER_KEY_TOUCHPAD = 9,
    PXR_CONTROLLER_KEY_LASTONE = 127,

    PXR_CONTROLLER_TOUCH_AX = 128,
    PXR_CONTROLLER_TOUCH_BY = 129,
    PXR_CONTROLLER_TOUCH_ROCKER = 130,
    PXR_CONTROLLER_TOUCH_TRIGGER = 131,
    PXR_CONTROLLER_TOUCH_THUMB = 132,
    PXR_CONTROLLER_TOUCH_LASTONE = 255
} PxrControllerKeyMap;

typedef enum
{
    PXR_CONTROLLER_HAVE_TOUCH = 0x00000001,
    PXR_CONTROLLER_HAVE_GRIP = 0x00000002,
    PXR_CONTROLLER_HAVE_ROCKER = 0x00000004,
    PXR_CONTROLLER_HAVE_TOUCHPAD = 0x00000008,
    PXR_CONTROLLER_HAVE_ALL = 0xFFFFFFFF

} PxrControllerAbilities;


typedef enum {
    PXR_NO_DEVICE = 0,
    PXR_HB_Controller = 1,
    PXR_CV_Controller = 2,
    PXR_HB2_Controller = 3,
    PXR_CV2_Controller = 4,
    PXR_CV3_Optics_Controller = 5,
} PxrControllerType;

typedef struct PxrControllerTracking_ {
    PxrSensorState localControllerPose;
    PxrSensorState globalControllerPose;
} PxrControllerTracking;

typedef struct PxrVector2f_ {
    float    x; //0~255
    float    y; //0~255
} PxrVector2f;

typedef struct PxrControllerInputState_ {
    PxrVector2f Joystick;   // 0-255
    int homeValue;          // 0/1
    int backValue;          // 0/1
    int touchpadValue;      // 0/1
    int volumeUp;           // 0/1
    int volumeDown;         // 0/1
    float triggerValue;       // 0-255 --> 0-1
    int batteryValue;       // 0-5
    int AXValue;            // 0/1
    int BYValue;            // 0/1
    int sideValue;          // 0/1
    float gripValue;          // 0-255  --> 0-1
    int reserved_key_0;
    int reserved_key_1;
    int reserved_key_2;
    int reserved_key_3;
    int reserved_key_4;

    int AXTouchValue;       // 0/1
    int BYTouchValue;       // 0/1
    int rockerTouchValue;   // 0/1
    int triggerTouchValue;  // 0/1
    int thumbrestTouchValue;// 0/1
    int reserved_touch_0;
    int reserved_touch_1;
    int reserved_touch_2;
    int reserved_touch_3;
    int reserved_touch_4;

} PxrControllerInputState;

typedef struct PxrControllerInputStateDowntimeStamp_ {

    long home;          // 0/1
    long back;          // 0/1
    long touchpad;      // 0/1
    long volumeUp;           // 0/1
    long volumeDown;         // 0/1
    long AX;            // 0/1
    long BY;            // 0/1
    long side;          // 0/1
    long grip;          // 0-255
    long reserved_key_0;
    long reserved_key_1;
    long reserved_key_2;
    long reserved_key_3;
    long reserved_key_4;

    long AXTouch;       // 0/1
    long BYTouch;       // 0/1
    long rockerTouch;   // 0/1
    long triggerTouch;  // 0/1
    long thumbrestTouch;// 0/1
    long reserved_touch_0;
    long reserved_touch_1;
    long reserved_touch_2;
    long reserved_touch_3;
    long reserved_touch_4;

} PxrControllerInputStateDowntimeStamp;


typedef struct PxrInputEvent_ {
    union
    {
        int int_value;
        float  float_value;
    };
//    int int_value;
    bool up;
    bool down;
    bool shortpress;
    bool longpress;
} PxrInputEvent;

typedef struct PxrControllerInputEvent_ {
    PxrInputEvent home;          // 0/1
    PxrInputEvent back;          // 0/1
    PxrInputEvent touchpad;      // 0/1
    PxrInputEvent volumeUp;      // 0/1
    PxrInputEvent volumeDown;    // 0/1
    PxrInputEvent AX;            // 0/1
    PxrInputEvent BY;            // 0/1
    PxrInputEvent side;          // 0/1
    PxrInputEvent reserved_0_Key;// 0/1
    PxrInputEvent reserved_1_Key;// 0/1
    PxrInputEvent reserved_2_Key;// 0/1
    PxrInputEvent reserved_3_Key;// 0/1
    PxrInputEvent reserved_4_Key;// 0/1

    PxrInputEvent AXTouch;       // 0/1
    PxrInputEvent BYTouch;       // 0/1
    PxrInputEvent rockerTouch;   // 0/1
    PxrInputEvent triggerTouch;  // 0/1
    PxrInputEvent thumbrestTouch;// 0/1
    PxrInputEvent reserved_0_Touch;// 0/1
    PxrInputEvent reserved_1_Touch;// 0/1
    PxrInputEvent reserved_2_Touch;// 0/1
    PxrInputEvent reserved_3_Touch;// 0/1
    PxrInputEvent reserved_4_Touch;// 0/1

} PxrControllerInputEvent;


typedef struct PxrControllerCapability_ {
    PxrControllerType             type;
    PxrControllerDof              Dof;
    PxrControllerBond             inputBond;
    u_int64_t                 Abilities;
} PxrControllerCapability;

typedef struct PxrControllerInfo_ {
    PxrControllerType             type;
    char* mac;
    char* sn;
    char* version;
} PxrControllerInfo;

/*******************************************************************************************************************************************************************
*
*                                                 Main Controller Feature Function
*
*********************************************************************************************************************************************************************/

/*
 * get input connecte state，type,input dof,touch and mac addr.
 */
int Pxr_GetControllerCapabilities(uint32_t deviceID, PxrControllerCapability *capability);

/*
 * get input connecte state.
 */
int Pxr_GetControllerConnectStatus(uint32_t deviceID);

//get input sensor data
//attention: predictTime can be set 0 if user do not need predict. headSensorData can be set NULL too.
int Pxr_GetControllerTrackingState(uint32_t deviceID, double predictTime, float headSensorData[],
                                   PxrControllerTracking *tracking);

//get input key state,please refer to PxrControllerInputState struct
int Pxr_GetControllerInputState(uint32_t deviceID, PxrControllerInputState *state);

//get input key Event,please refer to PxrControllerInputEvent struct
int Pxr_GetControllerInputEvent(uint32_t deviceID, PxrControllerInputEvent *event);

/******************************************************
 * set input vibration
 ******************************************************/
int Pxr_SetControllerVibration(uint32_t deviceID, float strength, int time);


/*******************************************************************************************************************************************************************
*
*                                          Controller Manager Feature Function
*
*********************************************************************************************************************************************************************/

//set app can get key value
int Pxr_SetControllerEnableKey(bool isEnable,PxrControllerKeyMap Key);

//set main input device
int Pxr_SetControllerMainInputHandle(uint32_t deviceID);

//get main input device
int Pxr_GetControllerMainInputHandle(uint32_t *deviceID);

//get main input device
int Pxr_SetControllerUnbind(uint32_t deviceID);

//get main input device
int Pxr_SetControllerEnterPairing(uint32_t deviceID);

//get main input device
int Pxr_SetControllerStopPairing(uint32_t deviceID);

//get main input device
int Pxr_SetControllerUpgrade(uint32_t deviceID,int rule,char* station_path_by_char,char* controller_path_by_char);

/*
 * get input device type/mac/sn addr.
 */
int Pxr_GetControllerinfo(uint32_t deviceID, PxrControllerInfo *info);

/*******************************************************************************************************************************************************************
*
*                                                 Main Hand Feature Function
*
*********************************************************************************************************************************************************************/

/*
 * there is some function about hand,so put hand function here.
 */





/*******************************************************************************************************************************************************************
*
*                                          legacy Function  don't use!
*
*********************************************************************************************************************************************************************/

int *Pxr_GetControllerKeyEventExt(int controllerSerialNum, int devicetype);
int Pxr_GetControllerTouchEvent(int controllerSerialNum, int length, int *value);
int Pxr_SetInputEventCallback(bool enable_Input_callback);
//reset input pose
int Pxr_RecenterInputPose(uint32_t deviceID);
//get head raw sensor data
int Pxr_GetHeadSensorData(float *data);

#endif //PXR_INPUT_H
