
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/proc_fs.h>   //proc file use 
#include <linux/dma-mapping.h>
#include "../camera/kd_camera_hw.h"

#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_camera_feature.h"
#include "kd_imgsensor_errcode.h"

#include "kd_sensorlist.h"

#define USE_NEW_IOCTL

UINT32 kdGetSensorInitFuncList(ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT **ppSensorList)
{
    if (NULL == ppSensorList)
    {
        printk("[kdGetSensorInitFuncList]ERROR: NULL ppSensorList\n");
        return 1;
    }
    *ppSensorList = &kdSensorList[0];
	return 0;
} // kdGetSensorInitFuncList()

//i2c id for sensor device
// @Sean, move to kd_camera_hw.h
//#define IMG_SENSOR_I2C_GROUP_ID 0
#define CAMERA_HW_DRVNAME "kd_camera_hw"

/* SENSOR READ/WRITE ID */
#define CAMERA_HW_WRITE_ID    0xfe
#define CAMERA_HW_READ_ID     0xfd

#define PFX "[kd_sensorlist]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    printk(KERN_INFO PFX "%s: " fmt, __FUNCTION__ ,##arg)

#define DEBUG_CAMERA_HW_K
#ifdef DEBUG_CAMERA_HW_K
#define PK_DBG PK_DBG_FUNC
#define PK_ERR(fmt, arg...)         printk(KERN_ERR PFX "%s: " fmt, __FUNCTION__ ,##arg)
#else
#define PK_DBG(a,...)
#define PK_ERR(a,...)
#endif

#define PROFILE 1 
#if PROFILE 
static struct timeval tv1, tv2; 
inline void KD_IMGSENSOR_PROFILE_INIT(void)
{
    do_gettimeofday(&tv1);    
}

inline void KD_IMGSENSOR_PROFILE(char *tag)
{
    unsigned long TimeIntervalUS;    

    do_gettimeofday(&tv2);
    TimeIntervalUS = (tv2.tv_sec - tv1.tv_sec) * 1000000 + (tv2.tv_usec - tv1.tv_usec); 
    tv1 = tv2; 
    PK_DBG("[%s]Profile = %lu\n",tag, TimeIntervalUS);
}
#else 
inline static void KD_IMGSENSOR_PROFILE_INIT() {}
inline static void KD_IMGSENSOR_PROFILE(char *tag) {}
#endif 


//static unsigned short g_pu2Normal_i2c[] = {CAMERA_HW_WRITE_ID , I2C_CLIENT_END};
//static unsigned short g_u2Ignore = I2C_CLIENT_END;
static struct i2c_client * g_pstI2Cclient = NULL;

//81 is used for V4L driver
static dev_t g_CAMERA_HWdevno = MKDEV(250,0);
static struct cdev * g_pCAMERA_HW_CharDrv = NULL;
static struct class *sensor_class = NULL;
//static atomic_t g_CAMERA_HWatomic;
static atomic_t g_CamHWOpend; 
static atomic_t g_CamDrvOpenCnt; 

static DEFINE_MUTEX(kdCam_Mutex); 

static SENSOR_FUNCTION_STRUCT *g_pSensorFunc = NULL;
static BOOL bSesnorVsyncFlag = FALSE;
static CAMERA_DUAL_CAMERA_SENSOR_ENUM g_currDualSensorIdx = DUAL_CAMERA_NONE_SENSOR;
static char g_currSensorName[32] = KDIMGSENSOR_NOSENSOR;
static ACDK_KD_SENSOR_SYNC_STRUCT g_NewSensorExpGain = {128, 128, 128, 128, 1000, 640, 0xFF, 0xFF, 0xFF, 0};
static const struct i2c_device_id CAMERA_HW_i2c_id[] = {{CAMERA_HW_DRVNAME,0},{}};   
static unsigned short force[] = {IMG_SENSOR_I2C_GROUP_ID, CAMERA_HW_WRITE_ID, I2C_CLIENT_END, I2C_CLIENT_END};   
static const unsigned short * const forces[] = { force, NULL };              
static struct i2c_client_address_data addr_data = { .forces = forces,}; 
int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId)
{
    int  i4RetValue = 0;

    g_pstI2Cclient->addr = i2cId;
    //
    i4RetValue = i2c_master_send(g_pstI2Cclient, a_pSendData, a_sizeSendData);
    if (i4RetValue != a_sizeSendData) {
        PK_DBG("[CAMERA SENSOR] I2C send failed!!, Addr = 0x%x\n", a_pSendData[0]);
        return -1; 
    }

    i4RetValue = i2c_master_recv(g_pstI2Cclient, (char *)a_pRecvData, a_sizeRecvData);
    if (i4RetValue != a_sizeRecvData) {
        PK_DBG("[CAMERA SENSOR] I2C read failed!! \n");
        return -1; 
    }
    return 0;
}


	
	/*=============================================================================*/
	/* iReadReg
	/*=============================================================================*/
	
	
	int iReadReg(u16 a_u2Addr , u8 * a_puBuff , u16 i2cId)
	{
		int  i4RetValue = 0;
		
		   char puReadCmd[2] = {(char)(a_u2Addr >> 8) , (char)(a_u2Addr & 0xFF)};
		   g_pstI2Cclient->addr = i2cId;
		   i4RetValue = i2c_master_send(g_pstI2Cclient, puReadCmd, 2);
		   if (i4RetValue != 2)
		   {
			   //printk("[S5K5CAGXYUV] <iReadReg> I2C send failed!! \r\n");
			   return -1;
		   }
		
		   i4RetValue = i2c_master_recv(g_pstI2Cclient, (char *)a_puBuff, 2);
		   //if (i4RetValue != 1)
		   if (i4RetValue != 2)
		   {
			   //printk("[S5K5CAGXYUV] <iReadReg> I2C read failed!! \r\n");
			   return -1;
		   }
		
		   return 0;
		  
	}


//static int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes)
/*=============================================================================*/
int iWriteReg(u16 a_u2Addr , u32 a_u4Data , u32 a_u4Bytes , u16 i2cId)
{
    int  i4RetValue = 0;
    int u4Index = 0;
	u16 ireadreg=0;
    u8 * puDataInBytes = (u8 *)&a_u4Data;
    int retry = 3; 

    char puSendCmd[6] = {(char)(a_u2Addr >> 8) , (char)(a_u2Addr & 0xFF) ,
        0 , 0 , 0 , 0};

//    PK_DBG("Addr : 0x%x,Val : 0x%x \n",a_u2Addr,a_u4Data);

#if PROFILE
	struct timeval ktv1, ktv2;
	unsigned long TimeIntervalUS;
	do_gettimeofday(&ktv1);
#endif 
    g_pstI2Cclient->addr = i2cId;
	g_pstI2Cclient->timing = 400;

    if(a_u4Bytes > 2)
    {
        PK_DBG("[CAMERA SENSOR] exceed 2 bytes \n");
        return -1;
    }
    
    if(a_u4Data >> (a_u4Bytes << 3))
    {
        PK_DBG("[CAMERA SENSOR] warning!! some data is not sent!! \n");
    }

   
	puSendCmd[2]=puDataInBytes[1];
	puSendCmd[3]=puDataInBytes[0];
	
    i4RetValue = i2c_master_send(g_pstI2Cclient, puSendCmd, (a_u4Bytes + 2));

    if (i4RetValue != (a_u4Bytes + 2))
    {
        //printk("[S5K5CAGXYUV] <iWriteReg> I2C send failed!! \r\n");
        return -1;
    }
#if PROFILE
	do_gettimeofday(&ktv2);
	if(ktv2.tv_sec > ktv1.tv_sec)
	{
		TimeIntervalUS = ktv1.tv_usec + 1000000 - ktv2.tv_usec;
	}
	else
	{
		TimeIntervalUS = ktv2.tv_usec - ktv1.tv_usec;
	}
	//printk("%lu\n",TimeIntervalUS);
#endif 
    return 0;
}


#define MAX_CMD_LEN          255
int iBurstWriteReg(u8 *pData, u32 bytes, u16 i2cId) 
{

    u32 phyAddr = 0; 
    u8 *buf = NULL;
    u32 old_addr = 0; 
    int ret = 0; 
    int retry = 0; 
    
    if (bytes > MAX_CMD_LEN) {
        PK_DBG("[iBurstWriteReg] exceed the max write length \n"); 
        return 1; 
    }
    phyAddr = 0; 

    buf = dma_alloc_coherent(0, bytes, &phyAddr, GFP_KERNEL);
    
    if (NULL == buf) {
        PK_DBG("[iBurstWriteReg] Not enough memory \n"); 
        return -1; 
    }

    memcpy(buf, pData, bytes); 
    //PK_DBG("[iBurstWriteReg] bytes = %d, phy addr = 0x%x \n", bytes, phyAddr ); 
    
    old_addr = g_pstI2Cclient->addr; 
    g_pstI2Cclient->addr = ( (g_pstI2Cclient->addr &  I2C_MASK_FLAG) | I2C_DMA_FLAG ); 

    ret = 0; 
#if 0
    int i =0; 
    for (i = 0; i < bytes; i++) {
        printk("0x%02x ", buf[i]); 
    }
#endif     

    retry = 3; 
    do {
        ret = i2c_master_send(g_pstI2Cclient, (u8*)phyAddr, bytes);     
        retry --; 
        if (ret != bytes) {
            PK_DBG("Error sent I2C ret = %d\n", ret); 
        }
    }while ((ret != bytes) && (retry > 0)); 

    dma_free_coherent(0, bytes, buf, phyAddr);
    g_pstI2Cclient->addr = old_addr; 

    return 0; 
}


int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId)
{
    int  i4RetValue = 0;
    int retry = 3; 

//    PK_DBG("Addr : 0x%x,Val : 0x%x \n",a_u2Addr,a_u4Data);

    //KD_IMGSENSOR_PROFILE_INIT(); 
    g_pstI2Cclient->addr = i2cId;
    //
    do {
        i4RetValue = i2c_master_send(g_pstI2Cclient, a_pSendData, a_sizeSendData);
        if (i4RetValue != a_sizeSendData) {
            PK_DBG("[CAMERA SENSOR] I2C send failed!!, Addr = 0x%x, Data = 0x%x \n", a_pSendData[0], a_pSendData[1] );
        }
        else {
            break; 
        }
        uDELAY(50); 
    } while ((retry--) > 0);
    //KD_IMGSENSOR_PROFILE("iWriteRegI2C");
    return 0;
}

extern int kdCISModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName,BOOL On, char* mode_name);


int kdModulePowerOn(CAMERA_DUAL_CAMERA_SENSOR_ENUM SensorIdx, char *currSensorName, BOOL On, char* mode_name)
{
    return kdCISModulePowerOn(SensorIdx,currSensorName,On,mode_name);
}

int kdSetDriver(unsigned int* pDrvIndex)
{
    ACDK_KD_SENSOR_INIT_FUNCTION_STRUCT *pSensorList = NULL;
    unsigned int drvIdx = (*pDrvIndex & KDIMGSENSOR_DUAL_MASK_LSB);

    //set driver for MAIN or SUB sensor
    g_currDualSensorIdx = (CAMERA_DUAL_CAMERA_SENSOR_ENUM)((*pDrvIndex & KDIMGSENSOR_DUAL_MASK_MSB)>>KDIMGSENSOR_DUAL_SHIFT);
    
    if (0 != kdGetSensorInitFuncList(&pSensorList))
    {
        PK_ERR("ERROR:kdGetSensorInitFuncList()\n");
        return -EIO;
    }

    if (drvIdx < MAX_NUM_OF_SUPPORT_SENSOR)
    {
        if (NULL == pSensorList[drvIdx].SensorInit)
        {
            PK_ERR("ERROR:kdSetDriver()\n");
            return -EIO;
        }

        pSensorList[drvIdx].SensorInit(&g_pSensorFunc);
        if (NULL == g_pSensorFunc)
        {
            PK_ERR("ERROR:NULL g_pSensorFunc\n");
            return -EIO;
        }

        //get sensor name
        memcpy((char*)g_currSensorName,(char*)pSensorList[drvIdx].drvname,sizeof(pSensorList[drvIdx].drvname));
        //return sensor ID
        *pDrvIndex = (unsigned int)pSensorList[drvIdx].SensorId;
        PK_DBG("[kdSetDriver] :%d,%d,%s,%d\n",g_currDualSensorIdx,drvIdx,g_currSensorName,sizeof(pSensorList[drvIdx].drvname));
    }
	return 0;
}

int kdSetSensorSyncFlag(BOOL bSensorSync)
{
    bSesnorVsyncFlag = bSensorSync;		
//    PK_DBG("[Sensor] kdSetSensorSyncFlag:%d\n", bSesnorVsyncFlag);
    return 0;
}

//static u16 g_SensorAFPos = 0; 
//static atomic_t g_SetSensorAF; 
int kdSensorSyncFunctionPtr(UINT16 *pRAWGain)
{
    unsigned int FeatureParaLen = 0;
    //PK_DBG("[Sensor] kdSensorSyncFunctionPtr1:%d %d %d\n", g_NewSensorExpGain.uSensorExpDelayFrame, g_NewSensorExpGain.uSensorGainDelayFrame, g_NewSensorExpGain.uISPGainDelayFrame);    
    mutex_lock(&kdCam_Mutex);
    if (NULL == g_pSensorFunc) {
        PK_ERR("ERROR:NULL g_pSensorFunc\n");
        mutex_unlock(&kdCam_Mutex);        
        return -EIO;
    }           
    //PK_DBG("[Sensor] Exposure time:%d, Gain = %d\n", g_NewSensorExpGain.u2SensorNewExpTime,g_NewSensorExpGain.u2SensorNewGain );
    // exposure time 
    if (g_NewSensorExpGain.uSensorExpDelayFrame == 0) {
        FeatureParaLen = 2;
        g_pSensorFunc->SensorFeatureControl(SENSOR_FEATURE_SET_ESHUTTER, (unsigned char*)&g_NewSensorExpGain.u2SensorNewExpTime, (unsigned int*) &FeatureParaLen);        
        g_NewSensorExpGain.uSensorExpDelayFrame = 0xFF; // disable                         
    }
    else if(g_NewSensorExpGain.uSensorExpDelayFrame != 0xFF) {
        g_NewSensorExpGain.uSensorExpDelayFrame --;  
    }
   
    // exposure gain 
    if (g_NewSensorExpGain.uSensorGainDelayFrame == 0) {
        FeatureParaLen = 2;
        g_pSensorFunc->SensorFeatureControl(SENSOR_FEATURE_SET_GAIN, (unsigned char*)&g_NewSensorExpGain.u2SensorNewGain, (unsigned int*) &FeatureParaLen);
        g_NewSensorExpGain.uSensorGainDelayFrame = 0xFF; // disable 
     }
    else if(g_NewSensorExpGain.uSensorGainDelayFrame != 0xFF) {
        g_NewSensorExpGain.uSensorGainDelayFrame --;  
     }
    
    //
    *pRAWGain = 0x00; 
    *(pRAWGain+1) = 0x00; 
    *(pRAWGain+2) = 0x00; 
    *(pRAWGain+3) = 0x00; 

    if(g_NewSensorExpGain.uISPGainDelayFrame == 0)    {  // synchronize the isp gain
         *pRAWGain = g_NewSensorExpGain.u2ISPNewRGain; 
      	  *(pRAWGain+1) = g_NewSensorExpGain.u2ISPNewGrGain; 
	       *(pRAWGain+2) = g_NewSensorExpGain.u2ISPNewGbGain; 
        *(pRAWGain+3) = g_NewSensorExpGain.u2ISPNewBGain; 
//        PK_DBG("[Sensor] ISP Gain:%d\n", g_NewSensorExpGain.u2ISPNewRGain, g_NewSensorExpGain.u2ISPNewGrGain, 
//			g_NewSensorExpGain.u2ISPNewGbGain, g_NewSensorExpGain.u2ISPNewBGain);    
        g_NewSensorExpGain.uISPGainDelayFrame = 0xFF; // disable         
    }
    else if(g_NewSensorExpGain.uISPGainDelayFrame != 0xFF) {
        g_NewSensorExpGain.uISPGainDelayFrame --;  
    }	
    mutex_unlock(&kdCam_Mutex);           
    return 0;
}
//
//winmo public control interface
//

inline static int adopt_CAMERA_HW_Open(void)
{
    UINT32 err = 0;

    KD_IMGSENSOR_PROFILE_INIT(); 
    // power on sensor
    if (atomic_read(&g_CamHWOpend) == 0) {
        // turn on power 
        kdModulePowerOn((CAMERA_DUAL_CAMERA_SENSOR_ENUM) g_currDualSensorIdx, g_currSensorName,true, CAMERA_HW_DRVNAME);
        //wait for power stable 
        mDELAY(10); 
        KD_IMGSENSOR_PROFILE("kdModulePowerOn"); 

        //
        if (g_pSensorFunc) {
            err = g_pSensorFunc->SensorOpen();
            if(ERROR_NONE != err) {
                PK_DBG(" ERROR:SensorOpen(), turn off power \n");                
                kdModulePowerOn((CAMERA_DUAL_CAMERA_SENSOR_ENUM) g_currDualSensorIdx, NULL, false, CAMERA_HW_DRVNAME);
            }
        }
        else {
            PK_DBG(" ERROR:NULL g_pSensorFunc\n");
        }
        KD_IMGSENSOR_PROFILE("SensorOpen"); 
    }
    if (err == 0 ) {
        atomic_set(&g_CamHWOpend, 1); 
    }
    return err?-EIO:err;
}	/* adopt_CAMERA_HW_Open() */

inline static int adopt_CAMERA_HW_CheckIsAlive(void)
{
    UINT32 err = 0;

    KD_IMGSENSOR_PROFILE_INIT(); 
    //power on sensor
    kdModulePowerOn((CAMERA_DUAL_CAMERA_SENSOR_ENUM) g_currDualSensorIdx, g_currSensorName,true, CAMERA_HW_DRVNAME);
    //wait for power stable 
    mDELAY(10); 
    KD_IMGSENSOR_PROFILE("kdModulePowerOn"); 

    //            
    if (g_pSensorFunc)
    {
        MUINT32 sensorID = 0; 
        MUINT32 retLen = 0; 
        err = g_pSensorFunc->SensorFeatureControl(SENSOR_FEATURE_CHECK_SENSOR_ID, (MUINT8*)&sensorID, &retLen);
        if (sensorID == 0) {    //not implement this feature ID 
            PK_DBG(" Not implement!!, use old open function to check\n"); 
            err = g_pSensorFunc->SensorOpen();                                     
        }
        else if (sensorID == 0xFFFFFFFF) {    //fail to open the sensor 
            PK_DBG(" No Sensor Found"); 
            err = ERROR_SENSOR_CONNECT_FAIL; 
        }
        else {
            PK_DBG(" Sensor found ID = 0x%x\n", sensorID); 
            err = ERROR_NONE;                 
        }            
        if(ERROR_NONE != err)
        {
            PK_DBG("ERROR:adopt_CAMERA_HW_CheckIsAlive(), No imgsensor alive \n");
        }
    }
    else
    {
        PK_DBG("ERROR:NULL g_pSensorFunc\n");
    }
    kdModulePowerOn((CAMERA_DUAL_CAMERA_SENSOR_ENUM) g_currDualSensorIdx, NULL, false, CAMERA_HW_DRVNAME);
    KD_IMGSENSOR_PROFILE("CheckIsAlive"); 
    
    return err?-EIO:err;
}	/* adopt_CAMERA_HW_Open() */


inline static int adopt_CAMERA_HW_GetResolution(void *pBuf)
{ 
    if (g_pSensorFunc) {
        g_pSensorFunc->SensorGetResolution(pBuf);
    }
    else {
        PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
    }
    return 0;
}	/* adopt_CAMERA_HW_GetResolution() */


inline static int adopt_CAMERA_HW_GetInfo(void *pBuf)
{
    ACDK_SENSOR_GETINFO_STRUCT *pSensorGetInfo = (ACDK_SENSOR_GETINFO_STRUCT*)pBuf;
    ACDK_SENSOR_INFO_STRUCT info;
    ACDK_SENSOR_CONFIG_STRUCT config;
    memset (&info, 0, sizeof(ACDK_SENSOR_INFO_STRUCT)); 
    memset (&config, 0, sizeof(ACDK_SENSOR_CONFIG_STRUCT)); 

    if (NULL == pSensorGetInfo) {
        PK_DBG("[CAMERA_HW] NULL arg.\n");
        return -EFAULT;
    }

    if (NULL == pSensorGetInfo->pInfo || NULL == pSensorGetInfo->pConfig)  {
        PK_DBG("[CAMERA_HW] NULL arg.\n");
        return -EFAULT;
    }

    if (g_pSensorFunc) {
        g_pSensorFunc->SensorGetInfo(pSensorGetInfo->ScenarioId,&info,&config);
    }
    else {
        PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
    }

    // SenorInfo 
    if(copy_to_user((void __user *) pSensorGetInfo->pInfo, (void*)&info , sizeof(ACDK_SENSOR_INFO_STRUCT))) {
        PK_DBG("[CAMERA_HW][info] ioctl copy to user failed\n");
        return -EFAULT;
    }

    // SensorConfig 
    if(copy_to_user((void __user *) pSensorGetInfo->pConfig , (void*)&config , sizeof(ACDK_SENSOR_CONFIG_STRUCT))) {
        PK_DBG("[CAMERA_HW][config] ioctl copy to user failed\n");
        return -EFAULT;
    }
    return 0;
}	/* adopt_CAMERA_HW_GetInfo() */

inline static int adopt_CAMERA_HW_Control(void *pBuf)
{
    int ret = 0; 
    ACDK_SENSOR_CONTROL_STRUCT *pSensorCtrl = (ACDK_SENSOR_CONTROL_STRUCT*)pBuf;
    ACDK_SENSOR_EXPOSURE_WINDOW_STRUCT imageWindow;
    ACDK_SENSOR_CONFIG_STRUCT sensorConfigData;
    memset(&imageWindow, 0, sizeof(ACDK_SENSOR_EXPOSURE_WINDOW_STRUCT)); 
    memset(&sensorConfigData, 0, sizeof(ACDK_SENSOR_CONFIG_STRUCT)); 

    if (NULL == pSensorCtrl ) {
        PK_DBG("[CAMERA_HW] NULL arg.\n");
        return -EFAULT;
    }
    
    if (NULL == pSensorCtrl->pImageWindow || NULL == pSensorCtrl->pSensorConfigData) {
        PK_DBG("[CAMERA_HW] NULL arg.\n");
        return  -EFAULT;
    }

    if(copy_from_user((void*)&imageWindow , (void *) pSensorCtrl->pImageWindow, sizeof(ACDK_SENSOR_EXPOSURE_WINDOW_STRUCT))) {
        PK_DBG("[CAMERA_HW][pFeatureData32] ioctl copy from user failed\n");
        return  -EFAULT;
    }
    
    if(copy_from_user((void*)&sensorConfigData , (void *) pSensorCtrl->pSensorConfigData, sizeof(ACDK_SENSOR_CONFIG_STRUCT))) {
        PK_DBG("[CAMERA_HW][pFeatureData32] ioctl copy from user failed\n");
        return  -EFAULT;
    }

    // 
    if (g_pSensorFunc) {
        ret = g_pSensorFunc->SensorControl(pSensorCtrl->ScenarioId,&imageWindow,&sensorConfigData);
    }
    else {
        PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
    }

    //
    if(copy_to_user((void __user *) pSensorCtrl->pImageWindow, (void*)&imageWindow , sizeof(ACDK_SENSOR_EXPOSURE_WINDOW_STRUCT))) {
        PK_DBG("[CAMERA_HW][imageWindow] ioctl copy to user failed\n");
        return -EFAULT;
    }

    //
    if(copy_to_user((void __user *) pSensorCtrl->pSensorConfigData, (void*)&sensorConfigData , sizeof(ACDK_SENSOR_CONFIG_STRUCT))) {
        PK_DBG("[CAMERA_HW][imageWindow] ioctl copy to user failed\n");
        return  -EFAULT;
    }
    return ret;
}//adopt_CAMERA_HW_Control

inline static int  adopt_CAMERA_HW_FeatureControl(void *pBuf)
{
    ACDK_SENSOR_FEATURECONTROL_STRUCT *pFeatureCtrl = (ACDK_SENSOR_FEATURECONTROL_STRUCT*)pBuf;
    unsigned int FeatureParaLen = 0;
    void *pFeaturePara = NULL;
    
    ACDK_SENSOR_GROUP_INFO_STRUCT *pSensorGroupInfo = NULL;
    ACDK_KD_SENSOR_SYNC_STRUCT *pSensorSyncInfo = NULL;
    char kernelGroupNamePtr[128];
    unsigned char *pUserGroupNamePtr = NULL;
    //UINT16 u2RAWGain[4];

    if (NULL == pFeatureCtrl ) {
        PK_ERR(" NULL arg.\n");
        return -EFAULT;
    }

    if(SENSOR_FEATURE_SINGLE_FOCUS_MODE == pFeatureCtrl->FeatureId || SENSOR_FEATURE_CANCEL_AF == pFeatureCtrl->FeatureId
		|| SENSOR_FEATURE_CONSTANT_AF == pFeatureCtrl->FeatureId)
    {//YUV AF_init and AF_constent and AF_single has no params
    }
    else 
    {
        if (NULL == pFeatureCtrl->pFeaturePara || NULL == pFeatureCtrl->pFeatureParaLen) {
            PK_ERR(" NULL arg.\n");
            return -EFAULT;
        }
    }

    if(copy_from_user((void*)&FeatureParaLen , (void *) pFeatureCtrl->pFeatureParaLen, sizeof(unsigned int))) {
        PK_ERR(" ioctl copy from user failed\n");
        return -EFAULT;
    }

    pFeaturePara = kmalloc(FeatureParaLen,GFP_KERNEL);
    if(NULL == pFeaturePara) {
        PK_ERR(" ioctl allocate mem failed\n");
        return  -ENOMEM;
    }

    //copy from user
    switch (pFeatureCtrl->FeatureId)
    {
        case SENSOR_FEATURE_SET_ESHUTTER:
        case SENSOR_FEATURE_SET_GAIN:
            // reset the delay frame flag	
	          g_NewSensorExpGain.uSensorExpDelayFrame = 0xFF;
            g_NewSensorExpGain.uSensorGainDelayFrame = 0xFF;
	          g_NewSensorExpGain.uISPGainDelayFrame = 0xFF;			
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
        case SENSOR_FEATURE_SET_REGISTER:
        case SENSOR_FEATURE_GET_REGISTER:        	
        case SENSOR_FEATURE_SET_CCT_REGISTER:
        case SENSOR_FEATURE_SET_ENG_REGISTER:
        case SENSOR_FEATURE_SET_ITEM_INFO:
        case SENSOR_FEATURE_GET_ITEM_INFO:
        case SENSOR_FEATURE_GET_ENG_INFO:
        case SENSOR_FEATURE_SET_VIDEO_MODE:
        case SENSOR_FEATURE_SET_YUV_CMD:            
        case SENSOR_FEATURE_MOVE_FOCUS_LENS:            
        case SENSOR_FEATURE_SET_AF_WINDOW:	        
        case SENSOR_FEATURE_SET_CALIBRATION_DATA:	        
            //
            if(copy_from_user((void*)pFeaturePara , (void *) pFeatureCtrl->pFeaturePara, FeatureParaLen)) {
                kfree(pFeaturePara);
                PK_DBG("[CAMERA_HW][pFeaturePara] ioctl copy from user failed\n");
                return -EFAULT;
            }
            break;
        case SENSOR_FEATURE_SET_SENSOR_SYNC:    // Update new sensor exposure time and gain to keep
            if(copy_from_user((void*)pFeaturePara , (void *) pFeatureCtrl->pFeaturePara, FeatureParaLen)) {
                PK_DBG("[CAMERA_HW][pFeaturePara] ioctl copy from user failed\n");
  	        return  -EFAULT;
  	    }
            // keep the information to wait Vsync synchronize
            pSensorSyncInfo = (ACDK_KD_SENSOR_SYNC_STRUCT*)pFeaturePara;
	    g_NewSensorExpGain.u2SensorNewExpTime = pSensorSyncInfo->u2SensorNewExpTime;
            g_NewSensorExpGain.u2SensorNewGain = pSensorSyncInfo->u2SensorNewGain;
	    g_NewSensorExpGain.u2ISPNewRGain = pSensorSyncInfo->u2ISPNewRGain;
	    g_NewSensorExpGain.u2ISPNewGrGain = pSensorSyncInfo->u2ISPNewGrGain;
	    g_NewSensorExpGain.u2ISPNewGbGain = pSensorSyncInfo->u2ISPNewGbGain;
            g_NewSensorExpGain.u2ISPNewBGain = pSensorSyncInfo->u2ISPNewBGain;
            g_NewSensorExpGain.uSensorExpDelayFrame = pSensorSyncInfo->uSensorExpDelayFrame;
	    g_NewSensorExpGain.uSensorGainDelayFrame = pSensorSyncInfo->uSensorGainDelayFrame;
	    g_NewSensorExpGain.uISPGainDelayFrame = pSensorSyncInfo->uISPGainDelayFrame;
	    
//            PK_DBG("[pFeaturePara] NewExp:%d NewSensorGain:%d NewISPGain:%d %d %d %d ExpDelay:%d SensorGainDelay:%d ISPGainDelay:%d\n", 
//				g_NewSensorExpGain.u2SensorNewExpTime,	g_NewSensorExpGain.u2SensorNewGain, g_NewSensorExpGain.u2ISPNewRGain, 
//				g_NewSensorExpGain.u2ISPNewGrGain, g_NewSensorExpGain.u2ISPNewGbGain, g_NewSensorExpGain.u2ISPNewBGain, 
//				g_NewSensorExpGain.uSensorExpDelayFrame, g_NewSensorExpGain.uSensorGainDelayFrame, g_NewSensorExpGain.uISPGainDelayFrame);            
//            if(bSesnorVsyncFlag == FALSE)   // set to sensor driver for 1st frame
//            {
//               PK_DBG("[CAMERA_HW][pFeaturePara] kdSensorSyncFunctionPtr \n");
//               kdSensorSyncFunctionPtr(u2RAWGain);
//            }

	           break;
        case SENSOR_FEATURE_GET_GROUP_INFO:
            if(copy_from_user((void*)pFeaturePara , (void *) pFeatureCtrl->pFeaturePara, FeatureParaLen)) {
                kfree(pFeaturePara);
                PK_DBG("[CAMERA_HW][pFeaturePara] ioctl copy from user failed\n");
                return -EFAULT;
            }
            pSensorGroupInfo = (ACDK_SENSOR_GROUP_INFO_STRUCT*)pFeaturePara;
            pUserGroupNamePtr = pSensorGroupInfo->GroupNamePtr;
            //
            if (NULL == pUserGroupNamePtr) {
                kfree(pFeaturePara);
                PK_DBG("[CAMERA_HW] NULL arg.\n");
                return  -EFAULT;
            }
            pSensorGroupInfo->GroupNamePtr = kernelGroupNamePtr;
            break;
        //copy to user
        case SENSOR_FEATURE_GET_RESOLUTION:
        case SENSOR_FEATURE_GET_PERIOD:
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
        case SENSOR_FEATURE_GET_CONFIG_PARA:
        case SENSOR_FEATURE_GET_GROUP_COUNT:
        case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
        //do nothing
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
        case SENSOR_FEATURE_SINGLE_FOCUS_MODE:
        case SENSOR_FEATURE_CANCEL_AF:
        case SENSOR_FEATURE_CONSTANT_AF:
		     default:
			        break;
	  }
    
    //
    if (g_pSensorFunc) {
        //sync with AE taske 
        //if (pFeatureCtrl->FeatureId == SENSOR_FEATURE_MOVE_FOCUS_LENS) {
        //    if (*(u16*)pFeaturePara != g_SensorAFPos) {
        //        g_SensorAFPos = *(u16 *)pFeaturePara;             
        //        atomic_set(&g_SetSensorAF, 1);  
                //PK_DBG("Set AF Pos = %d\n", g_SensorAFPos); 
        //    }
       //}
       //else {
        g_pSensorFunc->SensorFeatureControl(pFeatureCtrl->FeatureId,(unsigned char*)pFeaturePara,(unsigned int*) &FeatureParaLen);            
        //}
    }
    else {
        PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
    }

    //copy to user
    switch (pFeatureCtrl->FeatureId)
    {
        case SENSOR_FEATURE_SET_ESHUTTER:
        case SENSOR_FEATURE_SET_GAIN:
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
        case SENSOR_FEATURE_SET_REGISTER:
        case SENSOR_FEATURE_SET_CCT_REGISTER:
        case SENSOR_FEATURE_SET_ENG_REGISTER:
        case SENSOR_FEATURE_SET_ITEM_INFO:
        //do nothing
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            break;
        //copy to user
        case SENSOR_FEATURE_GET_RESOLUTION:
        case SENSOR_FEATURE_GET_PERIOD:
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
        case SENSOR_FEATURE_GET_REGISTER:
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
        case SENSOR_FEATURE_GET_CONFIG_PARA:
        case SENSOR_FEATURE_GET_GROUP_COUNT:
        case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
        case SENSOR_FEATURE_GET_ITEM_INFO:
        case SENSOR_FEATURE_GET_ENG_INFO:
        case SENSOR_FEATURE_GET_AF_STATUS:            
        case SENSOR_FEATURE_GET_AF_INF:            
        case SENSOR_FEATURE_GET_AF_MACRO:            
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            //
            if(copy_to_user((void __user *) pFeatureCtrl->pFeaturePara, (void*)pFeaturePara , FeatureParaLen))
            {
                kfree(pFeaturePara);
                PK_DBG("[CAMERA_HW][pSensorRegData] ioctl copy to user failed\n");
                return  -EFAULT;
            }
            break;
        //copy from and to user
        case SENSOR_FEATURE_GET_GROUP_INFO:
            //copy 32 bytes
            if(copy_to_user((void __user *) pUserGroupNamePtr, (void*)kernelGroupNamePtr , sizeof(char)*32))
            {
                kfree(pFeaturePara);
                PK_DBG("[CAMERA_HW][pFeatureReturnPara32] ioctl copy to user failed\n");
                return  -EFAULT;
            }
            pSensorGroupInfo->GroupNamePtr = pUserGroupNamePtr;
            if(copy_to_user((void __user *) pFeatureCtrl->pFeaturePara, (void*)pFeaturePara , FeatureParaLen))
            {
                kfree(pFeaturePara);
                PK_DBG("[CAMERA_HW][pFeatureReturnPara32] ioctl copy to user failed\n");
                return -EFAULT;
            }
            break;
	     default:
			       break;
	  }

    kfree(pFeaturePara);
    if(copy_to_user((void __user *) pFeatureCtrl->pFeatureParaLen, (void*)&FeatureParaLen , sizeof(unsigned int))) {
        PK_DBG("[CAMERA_HW][pFeatureParaLen] ioctl copy to user failed\n");
        return -EFAULT;
    }
    return 0;
}	/* adopt_CAMERA_HW_FeatureControl() */


inline static int adopt_CAMERA_HW_Close(void)
{
    if (atomic_read(&g_CamHWOpend) == 0) {
         return 0; 
    }
    else if (atomic_read(&g_CamHWOpend) == 1) {        
        if (g_pSensorFunc) {
            g_pSensorFunc->SensorClose();
        }
        else {
            PK_DBG("[CAMERA_HW]ERROR:NULL g_pSensorFunc\n");
        }
        //power off sensor
        kdModulePowerOn((CAMERA_DUAL_CAMERA_SENSOR_ENUM) g_currDualSensorIdx, NULL, false, CAMERA_HW_DRVNAME);
   }
   atomic_set(&g_CamHWOpend, 0); 
    
   // reset the delay frame flag	
   g_NewSensorExpGain.uSensorExpDelayFrame = 0xFF;
   g_NewSensorExpGain.uSensorGainDelayFrame = 0xFF;
   g_NewSensorExpGain.uISPGainDelayFrame = 0xFF;

   return 0;
}	/* adopt_CAMERA_HW_Close() */

#ifdef USE_NEW_IOCTL
static int CAMERA_HW_Ioctl(
    struct file * a_pstFile,
    unsigned int a_u4Command,
    unsigned long a_u4Param
)
#else 
static int CAMERA_HW_Ioctl(struct inode * a_pstInode,
struct file * a_pstFile,
unsigned int a_u4Command,
unsigned long a_u4Param)
#endif 
{
    int i4RetValue = 0;
    void * pBuff = NULL;
    u32 *pIdx = NULL;

    //PK_DBG("%x, %x \n",a_u4Command,a_u4Param);
    mutex_lock(&kdCam_Mutex); 

    if(_IOC_NONE == _IOC_DIR(a_u4Command))
    {
    }
    else
    {
        pBuff = kmalloc(_IOC_SIZE(a_u4Command),GFP_KERNEL);

        if(NULL == pBuff)
        {
            PK_DBG("[CAMERA SENSOR] ioctl allocate mem failed\n");
            i4RetValue = -ENOMEM;
            goto CAMERA_HW_Ioctl_EXIT;
        }

        if(_IOC_WRITE & _IOC_DIR(a_u4Command))
        {
            if(copy_from_user(pBuff , (void *) a_u4Param, _IOC_SIZE(a_u4Command)))
            {
                kfree(pBuff);
                PK_DBG("[CAMERA SENSOR] ioctl copy from user failed\n");
                i4RetValue =  -EFAULT;
                goto CAMERA_HW_Ioctl_EXIT;
            }
        }
    }

    pIdx = (u32*)pBuff;
    switch(a_u4Command)
    {
#if 0
        case KDIMGSENSORIOC_X_POWER_ON:
            i4RetValue = kdModulePowerOn((CAMERA_DUAL_CAMERA_SENSOR_ENUM) *pIdx, true, CAMERA_HW_DRVNAME);
            break;
        case KDIMGSENSORIOC_X_POWER_OFF:
            i4RetValue = kdModulePowerOn((CAMERA_DUAL_CAMERA_SENSOR_ENUM) *pIdx, false, CAMERA_HW_DRVNAME);
            break;
#endif
        case KDIMGSENSORIOC_X_SET_DRIVER:
            i4RetValue = kdSetDriver((unsigned int*)pBuff);
            break;
        case KDIMGSENSORIOC_T_OPEN:
            i4RetValue = adopt_CAMERA_HW_Open();
            break;
        case KDIMGSENSORIOC_X_GETINFO:
            i4RetValue = adopt_CAMERA_HW_GetInfo(pBuff);
            break;
        case KDIMGSENSORIOC_X_GETRESOLUTION:
            i4RetValue = adopt_CAMERA_HW_GetResolution(pBuff);
            break;
        case KDIMGSENSORIOC_X_FEATURECONCTROL:
            i4RetValue = adopt_CAMERA_HW_FeatureControl(pBuff);
            break;
        case KDIMGSENSORIOC_X_CONTROL:
            i4RetValue = adopt_CAMERA_HW_Control(pBuff);
            break;
        case KDIMGSENSORIOC_T_CLOSE:
            i4RetValue = adopt_CAMERA_HW_Close();
            break;
        case KDIMGSENSORIOC_T_CHECK_IS_ALIVE:
            i4RetValue = adopt_CAMERA_HW_CheckIsAlive(); 
            break; 
       	default :
            PK_DBG("No such command \n");
            i4RetValue = -EPERM;
            break;
        		
    }

    if(_IOC_READ & _IOC_DIR(a_u4Command))
    {
        if(copy_to_user((void __user *) a_u4Param , pBuff , _IOC_SIZE(a_u4Command)))
        {
            kfree(pBuff);
            PK_DBG("[CAMERA SENSOR] ioctl copy to user failed\n");
            i4RetValue =  -EFAULT;
            goto CAMERA_HW_Ioctl_EXIT;
        }
    }

    kfree(pBuff);
CAMERA_HW_Ioctl_EXIT:
    mutex_unlock(&kdCam_Mutex);             
    return i4RetValue;
}

//
//below is for linux driver system call
//change prefix or suffix only
//

static int CAMERA_HW_Open(struct inode * a_pstInode, struct file * a_pstFile)
{
    //
    atomic_inc(&g_CamDrvOpenCnt); 
    return 0;
}

static int CAMERA_HW_Release(struct inode * a_pstInode, struct file * a_pstFile)
{
    atomic_dec(&g_CamDrvOpenCnt); 
    
    return 0;
}

static const struct file_operations g_stCAMERA_HW_fops =
{
    .owner = THIS_MODULE,
    .open = CAMERA_HW_Open,
    .release = CAMERA_HW_Release,
    #ifdef USE_NEW_IOCTL
    .unlocked_ioctl = CAMERA_HW_Ioctl
    #else 
    .ioctl = CAMERA_HW_Ioctl
    #endif 
};

#define CAMERA_HW_DYNAMIC_ALLOCATE_DEVNO 1
inline static int RegisterCAMERA_HWCharDrv(void)
{
    struct device* sensor_device = NULL;

#if CAMERA_HW_DYNAMIC_ALLOCATE_DEVNO
    if( alloc_chrdev_region(&g_CAMERA_HWdevno, 0, 1,CAMERA_HW_DRVNAME) )
    {
        PK_DBG("[CAMERA SENSOR] Allocate device no failed\n");

        return -EAGAIN;
    }
#else
    if( register_chrdev_region(  g_CAMERA_HWdevno , 1 , CAMERA_HW_DRVNAME) )
    {
        PK_DBG("[CAMERA SENSOR] Register device no failed\n");

        return -EAGAIN;
    }
#endif

    //Allocate driver
    g_pCAMERA_HW_CharDrv = cdev_alloc();

    if(NULL == g_pCAMERA_HW_CharDrv)
    {
        unregister_chrdev_region(g_CAMERA_HWdevno, 1);

        PK_DBG("[CAMERA SENSOR] Allocate mem for kobject failed\n");

        return -ENOMEM;
    }

    //Attatch file operation.
    cdev_init(g_pCAMERA_HW_CharDrv, &g_stCAMERA_HW_fops);

    g_pCAMERA_HW_CharDrv->owner = THIS_MODULE;

    //Add to system
    if(cdev_add(g_pCAMERA_HW_CharDrv, g_CAMERA_HWdevno, 1))
    {
        PK_DBG("[mt6516_IDP] Attatch file operation failed\n");

        unregister_chrdev_region(g_CAMERA_HWdevno, 1);

        return -EAGAIN;
    }

    sensor_class = class_create(THIS_MODULE, "sensordrv");
    if (IS_ERR(sensor_class)) {
        int ret = PTR_ERR(sensor_class);
        PK_DBG("Unable to create class, err = %d\n", ret);
        return ret;
    }
    sensor_device = device_create(sensor_class, NULL, g_CAMERA_HWdevno, NULL, CAMERA_HW_DRVNAME);

    return 0;
}

inline static void UnregisterCAMERA_HWCharDrv(void)
{
    //Release char driver
    cdev_del(g_pCAMERA_HW_CharDrv);

    unregister_chrdev_region(g_CAMERA_HWdevno, 1);

    device_destroy(sensor_class, g_CAMERA_HWdevno);
    class_destroy(sensor_class);
}

static int CAMERA_HW_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) 
{         
    strcpy(info->type, CAMERA_HW_DRVNAME);
    return 0;
}

static int CAMERA_HW_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id) 
{             
    int i4RetValue = 0;
    PK_DBG("[CAMERA_HW] Attach I2C \n");
 
    //get sensor i2c client
    g_pstI2Cclient = client;

    //Register char driver
    i4RetValue = RegisterCAMERA_HWCharDrv();

    if(i4RetValue){
        PK_ERR("[CAMERA_HW] register char device failed!\n");
        return i4RetValue;
    }

    //spin_lock_init(&g_CamHWLock);

    PK_DBG("[CAMERA_HW] Attached!! \n");
    return 0;                                                                                       
} 


static int CAMERA_HW_i2c_remove(struct i2c_client *client)
{
    return 0;
}

struct i2c_driver CAMERA_HW_i2c_driver = {                       
    .probe = CAMERA_HW_i2c_probe,                                   
    .remove = CAMERA_HW_i2c_remove,                           
    .detect = CAMERA_HW_i2c_detect,                           
    .driver.name = CAMERA_HW_DRVNAME,
    .id_table = CAMERA_HW_i2c_id,                             
    .address_data = &addr_data,                        
};

static int CAMERA_HW_probe(struct platform_device *pdev)
{
    return i2c_add_driver(&CAMERA_HW_i2c_driver);
}


static int CAMERA_HW_remove(struct platform_device *pdev)
{
    i2c_del_driver(&CAMERA_HW_i2c_driver);
    return 0;
}


static int CAMERA_HW_suspend(struct platform_device *pdev, pm_message_t mesg)
{
    return 0;
}

static int CAMERA_HW_resume(struct platform_device *pdev)
{
    return 0;
}

static int  CAMERA_HW_DumpReg_To_Proc(char *page, char **start, off_t off,
                                                                                       int count, int *eof, void *data)
{
    return count;
}

static int  CAMERA_HW_Reg_Debug( struct file *file, const char *buffer, unsigned long count,
                                                                     void *data)
{
    char regBuf[64] = {'\0'}; 
    u32 u4CopyBufSize = (count < (sizeof(regBuf) - 1)) ? (count) : (sizeof(regBuf) - 1); 

    MSDK_SENSOR_REG_INFO_STRUCT sensorReg; 
    memset(&sensorReg, 0, sizeof(MSDK_SENSOR_REG_INFO_STRUCT));

    if (copy_from_user(regBuf, buffer, u4CopyBufSize))
        return -EFAULT;
    
    if (sscanf(regBuf, "%x %x",  &sensorReg.RegAddr, &sensorReg.RegData) == 2) {        
        if (g_pSensorFunc != NULL) {
            g_pSensorFunc->SensorFeatureControl(SENSOR_FEATURE_SET_REGISTER, (MUINT8*)&sensorReg, (MUINT32*)sizeof(MSDK_SENSOR_REG_INFO_STRUCT)); 
            g_pSensorFunc->SensorFeatureControl(SENSOR_FEATURE_GET_REGISTER, (MUINT8*)&sensorReg, (MUINT32*)sizeof(MSDK_SENSOR_REG_INFO_STRUCT));
            PK_DBG("write addr = 0x%08x, data = 0x%08x\n", sensorReg.RegAddr, sensorReg.RegData); 
        }
    }
    else if (sscanf(regBuf, "%x", &sensorReg.RegAddr) == 1) {    
        if (g_pSensorFunc != NULL) { 
            g_pSensorFunc->SensorFeatureControl(SENSOR_FEATURE_GET_REGISTER, (MUINT8*)&sensorReg, (MUINT32*)sizeof(MSDK_SENSOR_REG_INFO_STRUCT)); 
            PK_DBG("read addr = 0x%08x, data = 0x%08x\n", sensorReg.RegAddr, sensorReg.RegData); 
        }
    }

    return count; 
}



static struct platform_driver g_stCAMERA_HW_Driver = {
    .probe		= CAMERA_HW_probe,
    .remove	    = CAMERA_HW_remove,
    .suspend	= CAMERA_HW_suspend,
    .resume	    = CAMERA_HW_resume,
    .driver		= {
        .name	= "image_sensor",
        .owner	= THIS_MODULE,
    }
};

static int __init CAMERA_HW_i2C_init(void)
{
    struct proc_dir_entry *prEntry;

    if(platform_driver_register(&g_stCAMERA_HW_Driver)){
        PK_ERR("failed to register CAMERA_HW driver\n");
        return -ENODEV;
    }

    //Register proc file for sensor register debug 
    prEntry = create_proc_entry("driver/camsensor", 0, NULL); 
    if (prEntry) {
        prEntry->read_proc = CAMERA_HW_DumpReg_To_Proc; 
        prEntry->write_proc = CAMERA_HW_Reg_Debug; 
    }
    else {
        PK_ERR("add /proc/driver/camsensor entry fail \n");  
    }
    atomic_set(&g_CamHWOpend, 0); 
    atomic_set(&g_CamDrvOpenCnt, 0);
    return 0;
}


static void __exit CAMERA_HW_i2C_exit(void)
{
    platform_driver_unregister(&g_stCAMERA_HW_Driver);
}

EXPORT_SYMBOL(kdSetSensorSyncFlag); 
EXPORT_SYMBOL(kdSensorSyncFunctionPtr); 

module_init(CAMERA_HW_i2C_init);
module_exit(CAMERA_HW_i2C_exit);

MODULE_DESCRIPTION("CAMERA_HW driver");
MODULE_AUTHOR("Jackie Su <jackie.su@Mediatek.com>");
MODULE_LICENSE("GPL");





