#ifndef __CONFIG_H__
#define __CONFIG_H__


//#define DEBUG0


#define SCH_REPORT_ERRORS


#ifndef TRUE
#define FALSE 0
#define TRUE (!FALSE)
#endif

#define RETURN_NORMAL (uint8_t) 0

#define RETURN_ERROR (uint8_t)  1

#define ERROR_SCH_TOO_MANY_TASKS (1)

#define ERROR_SCH_CANNOT_DELETE_TASK (2)

#define ERROR_SCH_WAITING_FOR_SLAVE_TO_ACK (3)

#define ERROR_SCH_WAITING_FOR_START_COMMAND_FROM_MASTER (4)

#define ERROR_SCH_ONE_OR_MORE_SLAVES_DID_NOT_START (5)

#define ERROR_SCH_LOST_SLAVE (6)

#define ERROR_USART_WRITE_CHAR (7);

#define ERROR_USART_TI (8);



#endif
