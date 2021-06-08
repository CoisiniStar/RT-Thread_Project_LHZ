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
#define THREAD_PRIORITY             10//线程优先级
#define THREAD_TIMESLICE            5//线程时间片
char arv[]="AT+MQTTCFG=\"124.70.186.199\",1883,\"NB_123\",120,\"NB_123\",\"123123\",1";//新建MQTT机制的配置参数
static char nb_m5311_stack[1024];//线程栈空间大小
static char nb_m5311_send_Stack[1024];//线程栈空间大小
static char nb_recv_stack[1024];//线程栈空间大小
static struct rt_thread NB_Thread;//NB初始化线程句柄
static struct rt_thread NB_Send_Thread;//NB订阅主题以及发送消息线程句柄
static struct rt_thread NB_RECV_Thread;//NB接收消息线程句柄


static void urc_recv_func(struct at_client *client,const char *data, rt_size_t size);

static struct at_urc urc_table[] = {
//{"WIFI CONNECTED", "\r\n", urc_conn_func},
{"+MQTTPUBLISH", ":", urc_recv_func},
//{"RDY", "\r\n", urc_func},
};

//rt_thread_t NB_Thread=RT_NULL;
at_response_t nb_resp = RT_NULL;//AT组件的响应结构体句柄
at_client_t nb_client = RT_NULL;//AT组件的客户端句柄
	
//初始化NB发送线程与初始化线程之间的信号量
static rt_sem_t nb_sem = RT_NULL;

//NB初始化线程：新建MQTT机制+连接MQTT服务器
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
	 else{//真正完成了新建MQTT机制和连接服务器
	     LOG_E("The MQTT haven inited success\r\n");
		  rt_sem_release(nb_sem);//释放信号量
	  }
	}
   rt_thread_mdelay(1000);
}	
//NB数据发送线程：订阅主题+发送消息；以后的消息发送可以利用邮箱+内存池的方式或者消息队列的方式；
//这里的数据发送仅仅是一个demo，所以所发送的数据是固定的字符串111
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
			if(at_obj_exec_cmd(nb_client,nb_resp,"AT+MQTTSUB=\"pyr\",1")==RT_EOK)//订阅主题
			{
			  rt_kprintf("The NB-IoT have subscribed the pyr topic\r\n");
			  break;
			}
		}
		while(1)
		{
//			//发送MQTT消息
//			if(at_obj_exec_cmd(nb_client,nb_resp,"  AT+MQTTPUB=\"pyr\",1,1,0,0,\"111\"    ")!=RT_EOK)
//			{
//				LOG_E("Send the MEssage of the MQTT failed\r\n");
//			}
			rt_thread_mdelay(60000);//一分钟发一次消息
		}
	}
}

char message_mqtt_recv[1024];
static void urc_recv_func(struct at_client *client,const char *data, rt_size_t size)
{
/* 接 收 到 服 务 器 发 送 数 据 */
LOG_E("AT Client receive AT Server data!");
at_client_obj_recv(nb_client,message_mqtt_recv,1024,500);
}

//NB接收数据线程入口函数
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
	/* 添 加 多 种 URC 数 据 至 URC 列 表 中， 当 接 收 到 同 时 匹 配 URC 前 缀 和 后 缀 的 数 据， 执 行
    URC 函 数 */
   at_set_urc_table(urc_table, sizeof(urc_table) / sizeof(urc_table[0]));
	//初始化NB线程并启动线程
   rt_thread_init(&NB_Thread,   
                  "NB_Thread",
                  NB_mqtt_thread_entery,
                  RT_NULL,
                  &nb_m5311_stack[0],
                  sizeof(nb_m5311_stack),
                  THREAD_PRIORITY - 1, THREAD_TIMESLICE);
	rt_thread_startup(&NB_Thread);
	//NB订阅和发送线程并启动线程
	rt_thread_init(&NB_Send_Thread,   
                  "NB_Send_Thread",
                  NB_mqtt_send_thread_entery,
                  RT_NULL,
                  &nb_m5311_send_Stack[0],
                  sizeof(nb_m5311_send_Stack),
                  THREAD_PRIORITY - 1, THREAD_TIMESLICE);
	rt_thread_startup(&NB_Send_Thread);
	
	//NB接收数据线程并启动线程
	rt_thread_init(&NB_RECV_Thread,
	               "NB_RECV_Thread",
						 NB_recv_thread_entery,
						 RT_NULL,
						 &nb_recv_stack[0],
						 sizeof(nb_recv_stack),
					    THREAD_PRIORITY-1,THREAD_TIMESLICE);
   rt_thread_startup(&NB_RECV_Thread);
}

