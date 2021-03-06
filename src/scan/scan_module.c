/*
 * scan_module.c
 *
 *  Created on: 2014年6月17日
 *      Author: baby
 */

#include "scan_control_module.h"

void scan_module_init()
{
	scan_init();

	struct module_stru temp;
	temp.module_head = MOD_SCAN_HEAD;
	temp.count_tasks = 1;
	temp.dispatch = scan_dispatch;
	temp.taks_init[0] = scan_task_init;
	module_addtolist(temp, MOD_SCAN_HEAD);
}

void scan_task_init()
{
	OS_ERR err;
	OSQCreate(&ScanQ, "ScanQ", 10, &err);

	OSTaskCreate(
				(OS_TCB	*)&Scan_TCB,
				(CPU_CHAR	*)"Scan Task",
				(OS_TASK_PTR)task_scan,
				(void	*)0,
				(OS_PRIO	)2,
				(CPU_STK	*)&Scan_Stk[0],
				(CPU_STK_SIZE)16,
				(CPU_STK_SIZE)256,
				(OS_MSG_QTY	)0,
				(OS_TICK	)0,
				(void	*)0,
				(OS_OPT)(OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR),
				(OS_ERR *)&err
				);
}

void scan_dispatch(unsigned short *msg)
{
	OS_ERR err;
//	unsigned short temp_word = *(msg+1)&0x00FF;
	unsigned short temp_task = *(msg+1)>>8;
	switch (temp_task)
	{
	case MOD_SCAN_TASK:
		OSQPost(&ScanQ, msg+1, sizeof(unsigned short), OS_OPT_POST_FIFO, &err);
		break;
	}
}

void scan_render(unsigned short *data, unsigned short des_head, unsigned short des_word, unsigned short ori_task_interface, unsigned short *msg)
{
	*msg = des_head;
	*(msg+1) = des_word;
	*(msg+2) = *data;
	*(msg+3) = *(data+1);
	*(msg+4) = MOD_MOTOR_HEAD;
	*(msg+5) = ori_task_interface;
}

void task_scan(void *p_arg)
{
	OS_ERR err;
	OS_MSG_SIZE size;
	CPU_TS ts;
	unsigned short *msg;
	unsigned short msg_send[6];
	unsigned short data[2];

	while (1)
	{
		msg = OSQPend(&ScanQ, 0, OS_OPT_PEND_BLOCKING, &size, &ts, &err);
		switch (*msg)
		{
		case MOD_MOTOR_CMD_SET_ORIGIN:
			motor_origin_set();

			data[0]=0x0000;
			data[1]=0x0000;

			comm_render(data, MOD_COMM_HEAD, (MOD_COMM_TASK_SEND<<8) + MOD_COMM_CMD_SEND_INT,
						(MOD_MOTOR_TASK_MOVE<<8) + MOD_MOTOR_REPORT_ORIGINATE, msg_send);
			break;

		case MOD_MOTOR_CMD_STEP_FORWARD:
			motor_step_forward(*(msg+1));
			data[0] = 0x0000;
			data[1] = *(msg+1);
			comm_render(data, MOD_COMM_HEAD, (MOD_COMM_TASK_SEND<<8) + MOD_COMM_CMD_SEND_INT,
						(MOD_MOTOR_TASK_MOVE<<8) + MOD_MOTOR_REPORT_STEPS, msg_send);
			break;

		case MOD_MOTOR_CMD_STEP_BACKWARD:
			motor_step_backward(*(msg+1));
			data[0] = *(msg+1);
			data[1] = 0x0000;
			comm_render(data, MOD_COMM_HEAD, (MOD_COMM_TASK_SEND<<8) + MOD_COMM_CMD_SEND_INT,
						(MOD_MOTOR_TASK_MOVE<<8) + MOD_MOTOR_REPORT_STEPS, msg_send);
			break;

		case MOD_MOTOR_CMD_AUTO_FORWARD:
			data[0] = 0x0000;
			data[1] = MOTOR_SINGLE_STEP;
			while (motor_continue_check() == MOTOR_GOON)
			{
				motor_step_forward(MOTOR_SINGLE_STEP);
				comm_render(data, MOD_COMM_HEAD, (MOD_COMM_TASK_SEND<<8) + MOD_COMM_CMD_SEND_INT,
							(MOD_MOTOR_TASK_MOVE<<8) + MOD_MOTOR_REPORT_STEPS, msg_send);
				module_msg_dispatch(msg_send);
			}
			comm_render(data, MOD_COMM_HEAD, (MOD_COMM_TASK_SEND<<8) + MOD_COMM_CMD_SEND_INT,
						(MOD_MOTOR_TASK_MOVE<<8) + MOD_MOTOR_REPORT_STOP, msg_send);
			break;

		case MOD_MOTOR_CMD_AUTO_BACKWARD:
			motor_auto_backward();
			data[0] = MOTOR_SINGLE_STEP;
			data[1] = 0x0000;
			while (motor_continue_check() == MOTOR_GOON)
			{
				motor_step_backward(MOTOR_SINGLE_STEP);
				comm_render(data, MOD_COMM_HEAD, (MOD_COMM_TASK_SEND<<8) + MOD_COMM_CMD_SEND_INT,
							(MOD_MOTOR_TASK_MOVE<<8) + MOD_MOTOR_REPORT_STEPS, msg_send);
				module_msg_dispatch(msg_send);
			}
			comm_render(data, MOD_COMM_HEAD, (MOD_COMM_TASK_SEND<<8) + MOD_COMM_CMD_SEND_INT,
						(MOD_MOTOR_TASK_MOVE<<8) + MOD_MOTOR_REPORT_STOP, msg_send);
			break;

		case MOD_MOTOR_CMD_STOP:
			motor_stop();
			comm_render(data, MOD_COMM_HEAD, (MOD_COMM_TASK_SEND<<8) + MOD_COMM_CMD_SEND_INT,
						(MOD_MOTOR_TASK_MOVE<<8) + MOD_MOTOR_REPORT_STOP, msg_send);
			break;

		case MOD_MOTOR_CMD_ORIGINATE:
			motor_originate();
			int origin_point = motor_getorigin();
			if (origin_point>0)
			{
				data[0] = origin_point;
			}
			else
			{
				data[1] = origin_point;
			}
			comm_render(data, MOD_COMM_HEAD, (MOD_COMM_TASK_SEND<<8) + MOD_COMM_CMD_SEND_INT,
						(MOD_MOTOR_TASK_MOVE<<8) + MOD_MOTOR_REPORT_ORIGINATE, msg_send);
			break;
		}
		module_msg_dispatch(msg_send);
	}

}
