/**
  ******************************************************************************
  * @file    SysTick/TimeBase/main.c 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main program body.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "system_init.h"
#include "sim800l.h"

#if defined TEST_SIM
#define DBG_BUF     255
char rx_dbg[DBG_BUF];
uint8_t cmd_available,res_available,cmd_len,res_len;
#endif
char a[22]="AT+CMGS=\"+84929629746\"";
uint16_t RxCounter;
char rx_buf[SIM_BUFFER];
char publish_mes[MAX_PUBLISH_MES][LEN_PUBLISH_MES]={"LOI LOC NUOC HET HAN",
                                                    "Dau do khong co nuoc",
                                                    "*DANG KY THANH CONG*"};
char topic[LEN_TOPIC]="858173002686";
uint8_t tds_over_range=0,tds_under_range=0,sim_signal;
struct PHONEBOOK contact[MAX_CLIENT];

#if defined ADC
float averageVoltage = 0,tdsValue = 0,temperature = 25;
#endif

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{
    uint8_t i;
    booting();
    Delay(1000);

    //interact with sim via text mode
    while( (sim_set_text_mode(1,rx_buf) & SIM_RES_OK) == 0 );
    Delay(500);
    //do not generate interrupt when new sms comes
    while( (sim_set_cnmi_mode(0,0,0,0,0,rx_buf) & SIM_RES_OK) == 0 );
    Delay(500);
    //disable phonecall
    while( (sim_rej_in_call(1,rx_buf) & SIM_RES_OK) == 0 );
    Delay(500);
#if defined TEST_SIM
    while(1)
    {}
#endif
    //free phonebook
    for(i=0;i<MAX_CLIENT;i++)
    {
        memset(contact[i].number,'x',LEN_PHONE_NUM);
        contact[i].stat   = 0;
    }

    while (1)
    {
        sim_signal = sim_signal_strength(rx_buf);
        if( (2 < sim_signal) && (30 > sim_signal) )
        {
            //sim signal is strong enough
            GPIO_WriteBit(GPIOA, GPIO_Pin_6, (BitAction)(1));
        }
        else
        {
            //sim signal is too low or no signal at all
            GPIO_WriteBit(GPIOA, GPIO_Pin_6, (BitAction)(0));
        }
        
        Delay(1000);
        update_phonebook();
        Delay(1000);
#if defined ADC
        tds_over_range=0;
        tds_under_range=0;
        for(i=0;i<TDS_MEASURE_REPEAT;i++){
            Delay(1000);
            read_tds();
            if(tdsValue>TDS_LIMIT){
                tds_over_range++;
            }
            else if(tdsValue == 0){
                tds_under_range++;
            }
        }
        if( (tds_over_range == TDS_MEASURE_REPEAT) || (tds_under_range == TDS_MEASURE_REPEAT) )
            inform_customer();
#endif
        // GPIO_WriteBit(GPIOA, GPIO_Pin_6, (BitAction)(1 - GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_6)));
    }
}


#if defined ADC
/*-----------------------------------------------------------------*/
/*
 * this function send out warning message to customer if the probe
 * is not plugged into water or the TDS value is bigger the limittation
 */
void inform_customer(void)
{
    uint8_t i;
    for( i=0; i<MAX_CLIENT; i++ )
    {
        //subscribed contact
        if( (contact[i].stat & SUBSCONFIRMEDF) != 0 )
        {
            //water unsafe not sent yet
            if( (tds_over_range == TDS_MEASURE_REPEAT) && ((contact[i].stat & UNSAFEF) == 0) )
            {
                if( (sim_send_sms(contact[i].number,publish_mes[PUBLISH_WATER_UNSAFE],rx_buf) & SIM_RES_OK) !=0 )
                    contact[i].stat |= UNSAFEF;
            }
            //tds probe is not digged into water
            else if( (tds_under_range == TDS_MEASURE_REPEAT) && ((contact[i].stat & UNDIGF) == 0) )
            {
                if( (sim_send_sms(contact[i].number,publish_mes[PUBLISH_TDS_PROBE_NOWATER],rx_buf) & SIM_RES_OK) !=0 )
                    contact[i].stat |= UNDIGF;
            }
        }
    }
}
#endif

/**
  * @this function loop through all sms in memory. 
  * @>  If message contain valid activating code:
  * @     - If this is the first time reading (message status is "REC UNREAD"), consider this is
  * @       the activating message sent from user
  * @     - Updated to phonebook
  * @       + If this contact is already in the phonebook -> ignore
  * @       + If this is a new contact, put into the end of phonebook
  * @       + respond ok if this message status is "REC UNREAD"
  * @>  Else if message does not contain activate code -> remove this message
  */
void update_phonebook(void){
    uint8_t i,j;
    uint32_t stat;
    char temp_contact[LEN_PHONE_NUM]="+xxxxxxxxxxx";//no phonenumber, not sent message yet, not in phonebook
    char temp_data[MIN_BUFFER];
    //go through all sms(read/unread) in sim
    for(i=1;i<=MAX_SMS;i++)
    {
        stat=sim_read_sms(i,1,rx_buf); // not change status of sms record (if it is REC UNREAD, it is still REC UNREAD)
        //check if message contain activate code
        if( ((stat & SIM_RES_OK)  != 0 ) && ((sim_get_sms_data(temp_data,rx_buf) & SIM_RES_OK) != 0 ) )
        {
            // strcpy(inpsut_topic,temp_data,LEN_TOPIC);
            if( !strcmp(topic,temp_data,LEN_TOPIC) )
            {
                sim_get_sms_contact(temp_contact,rx_buf);

                for(j=0; j<MAX_CLIENT; j++)
                {
                    //if this contact already exist, just ignore it
                    if( !strcmp(temp_contact,contact[j].number,LEN_PHONE_NUM) )
                        break;

                    //this is new contact, push it to the end of phonebook
                    if( (contact[j].stat & SUBSCONFIRMEDF) == 0 ){
                        contact[j].stat |= SUBSCONFIRMEDF;
                        strcpy(contact[j].number,temp_contact,LEN_PHONE_NUM);
                        break;
                    }

                }

                //if this is "REC UNREAD"
                if( (j< MAX_CLIENT) && (sim_get_sms_state(rx_buf) == SMS_UNREAD) )
                {
                    if( (sim_send_sms(contact[j].number,publish_mes[PUBLISH_SUBSCRIBED_OK],rx_buf) & SIM_RES_OK) !=0 )
                        contact[j].stat |= SUBSCONFIRMEDF;
                    stat=sim_read_sms(i,0,rx_buf); // re-read this sms to change it's status to REC READ)
                }
            }
            //this is a trash message, remove it
            else
            {
                sim_dele_sms(i,rx_buf);
            }
        }
    }
}


#if defined ADC
/**
  * @this function read adc value.
  */
uint16_t read_adc(void)
{
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);                                   // start conversion
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);                    // end of conversion
    ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
    ADC_SoftwareStartConvCmd(ADC1, DISABLE);                                  // disable conversino
    return ADC_GetConversionValue(ADC1);                                      // return ADC value 
}
#endif


#if defined ADC
/**
  * @this function calculate tds value from the ADC red from read_adc() function.
  */
void read_tds(void)
{
    uint8_t analogBufferIndex;
    int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
    float compensationCoefficient;
    float compensationVolatge;

    for( analogBufferIndex=0; analogBufferIndex < SCOUNT; analogBufferIndex++)
    {
        analogBuffer[analogBufferIndex] = read_adc();
        Delay(50);//delay 50ms
    }

    // read the analog value more stable by the median filtering
    // algorithm, and convert to voltage value
    averageVoltage = getMedianNum(analogBuffer) * (float)VREF / 4096.0; //ADC 12 bits
    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
    compensationCoefficient=1.0+0.02*(temperature-25.0);
    compensationVolatge=averageVoltage/compensationCoefficient;  //temperature compensation
    //convert voltage value to tds value
    tdsValue=(uint16_t)((133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*0.5);
}
#endif


#if defined ADC
int getMedianNum(int* bArray) 
{
    int i, j, bTemp;
    int bTab[SCOUNT];
      for (i = 0; i<SCOUNT; i++)
	  bTab[i] = bArray[i];
      for (j = 0; j < SCOUNT - 1; j++) 
      {
	  for (i = 0; i < SCOUNT - j - 1; i++) 
          {
	    if (bTab[i] > bTab[i + 1]) 
            {
		bTemp = bTab[i];
	        bTab[i] = bTab[i + 1];
		bTab[i + 1] = bTemp;
	     }
	  }
      }
      if ((SCOUNT & 1) > 0)
	bTemp = bTab[(SCOUNT - 1) / 2];
      else
	bTemp = (bTab[SCOUNT / 2] + bTab[SCOUNT / 2 - 1]) / 2;
      return bTemp;
}
#endif

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}

#endif

/**
  * @}
  */

/**
  * @}
  */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
