/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/

// Demo files

// Data 
#include "project.h"
#include <time.h>

#include <stdio.h>
#include <stdlib.h>

#include "SD_I2C.h"

//extern CYBLE_GAPP_DISC_MODE_INFO_T cyBle_discoveryModeInfo;
//#define advPayload (cyBle_discoveryModeInfo.advData->advData)

CYBLE_CONN_HANDLE_T connHandle;
CYBLE_GATT_HANDLE_VALUE_PAIR_T thermoHandle;

uint8 collectedData[17]= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint8 isNotify = 0;
uint8 updateIsNotifyCCCDAttr = 0;

uint8 bleConnected = 0;

uint8 thermostat[5] = {1,1,1,1,1};

uint16 x, y, hum=0, tempe=0;

int temperatureC=0, temperatureF=0, humidity=0, tmp431=0;
char temp[25];

struct errorCode{
    
    
};

typedef struct  {
    
    float localTemp;
    float remoteTemp1;
    float remoteTemp2; 
    float remoteTemp3;
    float voltCompressor;
    float voltFan;
    float volt;
    float currentCompressor;
    float currentFan; 
    float current;
    float lineFreqComp;
    float lineFreqFan;
    float linFreq;
    float airFlow;
    uint16 accX;
    uint16 accY;
    uint16 accZ;
    
}dataCollected;
dataCollected mydata;

void updateThermostatData(void){
   
    thermoHandle.attrHandle = CYBLE_RVAC_THERMOSTAT_THERMOCCCD_DESC_HANDLE;
    thermoHandle.value.val = thermostat;
    thermoHandle.value.len = 5; 
    thermoHandle.value.actualLen = 5; 
    
    CyBle_GattsWriteAttributeValue(&thermoHandle, 0, &cyBle_connHandle, 0);
    // CyBle_GattcReadCharacteristicValue(&cyBle_connHandle, read);
}

void bleStack(uint32 event, void *eventParam){
    
    CYBLE_GATTS_WRITE_REQ_PARAM_T wrReq;
    
    switch(event){
     
        case CYBLE_EVT_STACK_ON:
            CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);

        case CYBLE_EVT_GAP_DEVICE_CONNECTED:
            //CyBle_GappStopAdvertisement(); 
            break;
        case CYBLE_EVT_GAP_DEVICE_DISCONNECTED:
            CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
            isNotify = 0;
            break;
        case CYBLE_EVT_GAPP_ADVERTISEMENT_START_STOP:
             if(CyBle_GetState() == CYBLE_STATE_DISCONNECTED){
                 CyBle_GappStartAdvertisement(CYBLE_SCANNING_FAST);
            }
            break;
        case CYBLE_EVT_GATT_CONNECT_IND:
          
            bleConnected = 1;
            CyBle_GappStopAdvertisement();
            break;
        case CYBLE_EVT_GATT_DISCONNECT_IND:
        
            isNotify = 0;
            bleConnected = 0;
            // Update the notifications CCCD
            updateIsNotifyCCCDAttr = 1;
            
            // update the thermostat array with the thermosstat info.
            // ex. themostat[0] = readSensor();
            // update the data in the thermostat attibute
            updateThermostatData();
            
           break;
        case CYBLE_EVT_GATTS_WRITE_REQ:
            wrReq = *(CYBLE_GATTS_WRITE_REQ_PARAM_T *)eventParam; //CYBLE_DAC_SERVICE_VOLT_VOLTCCCD_DESC_HANDLE
           
            // This handle the notofications
            if(wrReq.handleValPair.attrHandle == CYBLE_RVAC_SENSOR_SENSORCCCD_DESC_HANDLE){
                
               // CyBle_GattsWriteAttributeValue( &wrReq->handleValPair, 0, &connHandle, CYBLE_GATT_DB_LOCALLY_INITIATED);
                isNotify = wrReq.handleValPair.value.val[0];//CYBLE_RVAC_SENSOR_SENSORCCCD_DESC_INDEX];
                
                updateIsNotifyCCCDAttr = 1;
            }
            
            // This handle data incoming
            if(wrReq.handleValPair.attrHandle == CYBLE_RVAC_THERMOSTAT_CHAR_HANDLE){
            
                thermostat[0] = wrReq.handleValPair.value.val[0];
                thermostat[1] = wrReq.handleValPair.value.val[1];
                thermostat[2] = wrReq.handleValPair.value.val[2];
                thermostat[3] = wrReq.handleValPair.value.val[3];
                thermostat[4] = wrReq.handleValPair.value.val[4];
                
                updateThermostatData();
                
            }

            CyBle_GattsWriteRsp(cyBle_connHandle);
           
            
            break;
        
        
     default:
        break;
    }
    
}
// function to read thermostat data on the phone
void send_Thermo_to_phone(){
    
    CYBLE_GATT_HANDLE_VALUE_PAIR_T thermoRead;
    
     thermoRead.attrHandle = CYBLE_RVAC_THERMOSTAT_CHAR_HANDLE;
        thermoRead.value.val = thermostat;
        thermoRead.value.len = 5;
    
 
   CyBle_GattsWriteAttributeValue(&thermoRead, 0, &cyBle_connHandle, CYBLE_GATT_DB_PEER_INITIATED);
}

void SendDataNotify(uint8* data, uint8 len){
    CYBLE_GATTS_HANDLE_VALUE_NTF_T notiHandle;
    

     notiHandle.attrHandle = CYBLE_RVAC_SENSOR_CHAR_HANDLE;
     notiHandle.value.val = data;
     notiHandle.value.len = len;
        
        CyBle_GattsNotification(cyBle_connHandle, &notiHandle);

}

void updateNotificationCCCD(void){
    CYBLE_GATT_HANDLE_VALUE_PAIR_T sensorNotCCCDHandle;
    
    uint8 sensorCCCDVal[2];
    
    if(updateIsNotifyCCCDAttr == 1){
        
     updateIsNotifyCCCDAttr = 0;
        sensorCCCDVal[0] = isNotify;
        sensorCCCDVal[1] = 0x00;
        
        sensorNotCCCDHandle.attrHandle = CYBLE_RVAC_SENSOR_SENSORCCCD_DESC_HANDLE;
        sensorNotCCCDHandle.value.val = sensorCCCDVal;
        sensorNotCCCDHandle.value.len = 2;
        
        CyBle_GattsWriteAttributeValue(&sensorNotCCCDHandle, 0, &cyBle_connHandle, CYBLE_GATT_DB_PEER_INITIATED);
        CyBle_ProcessEvents();
    }
}



uint8 randomNum(){
 
    return (uint16) (rand()%100)+1;
}

void wdtSleepInt(){
  
     //Red_Write(0);
    
        collectedData[0] = randomNum();
        collectedData[1] = randomNum();
        collectedData[2] = randomNum();
        collectedData[3] = randomNum();
         collectedData[4] = randomNum();
         collectedData[5] = randomNum();
         collectedData[6] = randomNum();
         collectedData[7] = randomNum();
         collectedData[8] = randomNum();
         collectedData[9] = randomNum();
 collectedData[10] = randomNum();
 collectedData[11] = randomNum();
 collectedData[12] = randomNum();
 collectedData[14] += 3;
 collectedData[15] += 2;
 collectedData[16] += 1;

if(bleConnected == 1){

        updateNotificationCCCD();
        
        if(isNotify == 1){
            Green_Write(1);
            if(CyBle_GattGetBusStatus() == CYBLE_STACK_STATE_FREE){
             Blue_Write(0);
                SendDataNotify(collectedData, 17);
                Blue_Write(1);
           }
        }
        else{
            Green_Write(thermostat[0]);
        }
    }else{ 
        Blue_Write(1);
        Green_Write(1);
        if(!isNotify && Green_Read()!=0){
        Red_Write(0);}
    }
        
        
         sprintf(temp, "the volt is: %d",y);
    UART_1_PutString(temp);
    UART_1_PutCRLF();
    
//     sprintf(temp, "Hum reading: %d", humidity);
//    UART_1_PutString(temp);
//    UART_1_PutCRLF();
//    
//      sprintf(temp, "temp reading in C: %d and in F: %d", temperatureC, temperatureF);
//    UART_1_PutString(temp);
//    UART_1_PutCRLF();
//    
//    sprintf(temp, "temp reading from TI sensor in C: %d", tmp431);
//    UART_1_PutString(temp);
//    UART_1_PutCRLF();
        
 
     CySysWdtClearInterrupt(CY_SYS_WDT_COUNTER0_INT);
}





int main(void)
{
    CyGlobalIntEnable; /* Enable global interrupts. */
    isr_1_StartEx(wdtSleepInt);
    CySysWdtEnable(CY_SYS_WDT_COUNTER0);
    
    
    CyBle_Start(bleStack);
    //isr_1_Start();
    
    I2C_1_Start();
    
    UART_1_Start();
    
    Volt_Regulator_Start(); // ADC at P3.2

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    srand(time(0));
    
   
    UART_1_PutString("Hello world!");
    UART_1_PutCRLF();
    
    for(;;)
    {
        
        
    Volt_Regulator_StartConvert();
    Volt_Regulator_IsEndConversion(Volt_Regulator_WAIT_FOR_RESULT);
        Red_Write(1);
        //WDT_ISR_StartEx();
       
        /* Place your application code here. */
        
        
        
        

       
    
    x = Volt_Regulator_GetResult16(0);
    y = Volt_Regulator_CountsTo_Volts(0,x)*10;
    
    
    
   
    
//    hum = humidity_read(ADDR_HUMTEMP, REG_HUMSENSOR_HUM);
//    humidity = ((125*hum)/65536)-6; 
//    
//    
//   
//    
//    tempe = Read_sensor_byte(ADDR_HUMTEMP, REG_HUMSENSOR_TEM);
//    temperatureC = (((tempe*175.72)/65536.0)-46.85);
//    temperatureF = (temperatureC *1.8+32)*10;
//    
//    tmp431 = temperature_local_read(ADDR_TEMP431, REG_LH_TMP431);
   
  

        //CyBle_GapUpdateAdvData(cyBle_discoveryModeInfo.advData, cyBle_discoveryModeInfo.scanRspData); //Update data in Advertisment Packet
    send_Thermo_to_phone();
        CyBle_ProcessEvents();
    }
}

/* [] END OF FILE */
