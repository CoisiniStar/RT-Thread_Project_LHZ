/*				||                          ||		    ||                         ||||||||||||||||||||||
				||									 ||          ||                                              |||
				||									 ||          ||                                           |||
				||									 ||          ||                                         |||
				||									 ||||||||||||||                                      |||
				||									 ||          ||                                   |||                     
				||									 ||          ||                               |||
				||									 ||          ||                           |||
				||									 ||          ||                       |||
				||||||||||||                ||			 ||					      ||||||||||||||||||||||||||||||||||
*/
#include <rtthread.h>
#include <at.h>
#include <at_device_m5311.h>
#include <at_socket.h>
#include <rtdbg.h>
#define THREAD_PRIORITY             10//�߳����ȼ�
#define THREAD_TIMESLICE            5//�߳�ʱ��Ƭ
char arv[]="AT+MQTTCFG=\"124.70.186.199\",1883,\"NB_123\",120,\"NB_123\",\"123123\",1";//�½�MQTT���Ƶ����ò���
static char nb_m5311_stack[1024];//�߳�ջ�ռ��С
static char nb_m5311_send_Stack[1024];//�߳�ջ�ռ��С
static char nb_recv_stack[1024];//�߳�ջ�ռ��С
static struct rt_thread NB_Thread;//NB��ʼ���߳̾��
static struct rt_thread NB_Send_Thread;//NB���������Լ�������Ϣ�߳̾��
static struct rt_thread NB_RECV_Thread;//NB������Ϣ�߳̾��


static void urc_recv_func(struct at_client *client,const char *data, rt_size_t size);

static struct at_urc urc_table[] = {
//{"WIFI CONNECTED", "\r\n", urc_conn_func},
{"+MQTTPUBLISH", ":", urc_recv_func},
//{"RDY", "\r\n", urc_func},
};

//rt_thread_t NB_Thread=RT_NULL;
at_response_t nb_resp = RT_NULL;//AT�������Ӧ�ṹ����
at_client_t nb_client = RT_NULL;//AT����Ŀͻ��˾��
	
//��ʼ��NB�����߳����ʼ���߳�֮����ź���
static rt_sem_t nb_sem = RT_NULL;

//NB��ʼ���̣߳��½�MQTT����+����MQTT������
static void NB_mqtt_thread_entery()
{
	nb_client = at_client_get("uart2");
	nb_resp = at_create_resp(128, 0, rt_tick_from_millisecond(300));
	if(at_obj_exec_cmd(nb_client,nb_resp,arv)!=RT_EOK)
	{
	  LOG_E("The MQTT haven't inited success\r\n");
	}
	else{
	 LOG_E("MQTT HAVE INITED SUCCESS\r\n");
	 if(at_obj_exec_cmd(nb_client,nb_resp,"AT+MQTTOPEN=1,1,0,0,0,'',''")!=RT_EOK)
	 {
	   LOG_E("The MQTT haven't inited success\r\n");
	 }
	 else{//����������½�MQTT���ƺ����ӷ�����
	     LOG_E("The MQTT haven inited success\r\n");
		  rt_sem_release(nb_sem);//�ͷ��ź���
	  }
	}
   rt_thread_mdelay(1000);
}	
//NB���ݷ����̣߳���������+������Ϣ���Ժ����Ϣ���Ϳ�����������+�ڴ�صķ�ʽ������Ϣ���еķ�ʽ��
//��������ݷ��ͽ�����һ��demo�����������͵������ǹ̶����ַ���111
static void NB_mqtt_send_thread_entery()
{ 
   static rt_err_t result;
	result = rt_sem_take(nb_sem, RT_WAITING_FOREVER);
	if(result != RT_EOK)
	{
	  rt_kprintf("NB_mqtt_send_thread take a nb semaphore, failed.\n");
	  return;
	}else{
		for(;;)
		{
			if(at_obj_exec_cmd(nb_client,nb_resp,"AT+MQTTSUB=\"pyr\",1")==RT_EOK)//��������
			{
			  rt_kprintf("The NB-IoT have subscribed the pyr topic\r\n");
			  break;
			}
		}
		while(1)
		{
//			//����MQTT��Ϣ
//			if(at_obj_exec_cmd(nb_client,nb_resp,"  AT+MQTTPUB=\"pyr\",1,1,0,0,\"111\"    ")!=RT_EOK)
//			{
//				LOG_E("Send the MEssage of the MQTT failed\r\n");
//			}
			rt_thread_mdelay(60000);//һ���ӷ�һ����Ϣ
		}
	}
}

char message_mqtt_recv[1024];
static void urc_recv_func(struct at_client *client,const char *data, rt_size_t size)
{
/* �� �� �� �� �� �� �� �� �� �� */
LOG_E("AT Client receive AT Server data!");
at_client_obj_recv(nb_client,message_mqtt_recv,1024,500);
}

//NB���������߳���ں���
static void NB_recv_thread_entery()
{
//   urc_recv_func(message_mqtt_recv,1024);
	while(1)
	{
		rt_kprintf("receive the data:%s",message_mqtt_recv);
		rt_memset(message_mqtt_recv,0,1024);
		rt_thread_mdelay(5000);
	}

}


int main(void)
{
	nb_sem = rt_sem_create("nb_sem", 0, RT_IPC_FLAG_FIFO);
	/* �� �� �� �� URC �� �� �� URC �� �� �У� �� �� �� �� ͬ ʱ ƥ �� URC ǰ ׺ �� �� ׺ �� �� �ݣ� ִ ��
    URC �� �� */
   at_set_urc_table(urc_table, sizeof(urc_table) / sizeof(urc_table[0]));
	//��ʼ��NB�̲߳������߳�
   rt_thread_init(&NB_Thread,   
                  "NB_Thread",
                  NB_mqtt_thread_entery,
                  RT_NULL,
                  &nb_m5311_stack[0],
                  sizeof(nb_m5311_stack),
                  THREAD_PRIORITY - 1, THREAD_TIMESLICE);
	rt_thread_startup(&NB_Thread);
	//NB���ĺͷ����̲߳������߳�
	rt_thread_init(&NB_Send_Thread,   
                  "NB_Send_Thread",
                  NB_mqtt_send_thread_entery,
                  RT_NULL,
                  &nb_m5311_send_Stack[0],
                  sizeof(nb_m5311_send_Stack),
                  THREAD_PRIORITY - 1, THREAD_TIMESLICE);
	rt_thread_startup(&NB_Send_Thread);
	
	//NB���������̲߳������߳�
	rt_thread_init(&NB_RECV_Thread,
	               "NB_RECV_Thread",
						 NB_recv_thread_entery,
						 RT_NULL,
						 &nb_recv_stack[0],
						 sizeof(nb_recv_stack),
					    THREAD_PRIORITY-1,THREAD_TIMESLICE);
   rt_thread_startup(&NB_RECV_Thread);
}

