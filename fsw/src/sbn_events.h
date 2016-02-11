/************************************************************************
**   sbn_events.h
**   
**   2014/11/21 ejtimmon
**
**   Specification for the Software Bus Network event identifers.
**
*************************************************************************/

#ifndef _sbn_events_h_
#define _sbn_events_h_

/** \brief <tt> 'SBN Initialized.' </tt>
**  \event <tt> 'SBN Initialized.' </tt>
**
**  \par Type: INFORMATIONAL
**
**  \par Cause:
**
**  This event message is issued when the Software Bus Network has
**  completed initialization.
*/
#define SBN_INIT_EID 1

/** \brief <tt> 'Application Terminating, err = 0x\%08X' </tt>
**  \event <tt> 'Application Terminating, err = 0x\%08X' </tt>
**
**  \par Type: CRITICAL
**
**  \par Cause:
**
**  This event message is issued when the Software Bus Network
**  exits due to a fatal error condition.
**
**  The \c err field contains the return status from the
**  cFE call that caused the app to terminate
*/
#define SBN_APP_EXIT_EID 2

/** \brief <tt> 'Error Creating SB Command Pipe,RC=0x\%08X' </tt>
**  \event <tt> 'Error Creating SB Command Pipe,RC=0x\%08X' </tt>
**
**  \par Type: ERROR
**
**  \par Cause:
**
**  This event message is issued when the Software Bus Network
**  is unable to create its command pipe via the #CFE_SB_CreatePipe
**  API.
**
**  The \c RC field contains the return status from the
**  #CFE_SB_CreatePipe call that generated the error
*/
#define SBN_CR_CMD_PIPE_ERR_EID 3

/** \brief <tt> 'Error Subscribing to HK Request,RC=0x\%08X' </tt>
**  \event <tt> 'Error Subscribing to HK Request,RC=0x\%08X' </tt>
**
**  \par Type: ERROR
**
**  \par Cause:
**
**  This event message is issued when the call to #CFE_SB_Subscribe
**  for the #SBN_SEND_HK_MID, during initialization returns
**  a value other than CFE_SUCCESS
*/
#define SBN_SUB_REQ_ERR_EID 4

/** \brief <tt> 'Error Subscribing to Gnd Cmds,RC=0x\%08X' </tt>
**  \event <tt> 'Error Subscribing to Gnd Cmds,RC=0x\%08X' </tt>
**
**  \par Type: ERROR
**
**  \par Cause:
**
**  This event message is issued when the call to #CFE_SB_Subscribe
**  for the #SBN_CMD_MID, during initialization returns a value
**  other than CFE_SUCCESS
*/
#define SBN_SUB_CMD_ERR_EID 5

/** \brief <tt> 'Invalid command code: ID = 0x\%04X, CC = \%d' </tt>
**  \event <tt> 'Invalid command code: ID = 0x\%04X, CC = \%d' </tt>
**
**  \par Type: ERROR
**
**  \par Cause:
**
**  This event message is issued when a software bus message is received
**  with an invalid command code.
**
**  The \c ID field contains the message ID, the \c CC field contains
**  the command code that generated the error.
*/
#define SBN_CC_ERR_EID 6

/** \brief <tt> 'Invalid command pipe message ID: 0x\%04X' </tt>
**  \event <tt> 'Invalid command pipe message ID: 0x\%04X' </tt>
**
**  \par Type: ERROR
**
**  \par Cause:
**
**  This event message is issued when a software bus message is received
**  with an invalid message ID.
**
**  The \c message \c ID field contains the message ID that generated
**  the error.
*/
#define SBN_MID_ERR_EID 7

/** \brief <tt> 'Invalid HK request msg length: ID = 0x\%04X, CC = \%d, Len = \%d, Expected = \%d' </tt>
**  \event <tt> 'Invalid HK request msg length: ID = 0x\%04X, CC = \%d, Len = \%d, Expected = \%d' </tt>
**
**  \par Type: ERROR
**
**  \par Cause:
**
**  This event message is issued when a housekeeping request is received
**  with a message length that doesn't match the expected value.
**
**  The \c ID field contains the message ID, the \c CC field contains the
**  command code, the \c Len field is the actual length returned by the
**  #CFE_SB_GetTotalMsgLength call, and the \c Expected field is the expected
**  length for the message.
*/
#define SBN_HKREQ_LEN_ERR_EID 8

/** \brief <tt> 'Invalid msg length: ID = 0x\%04X, CC = \%d, Len = \%d, Expected = \%d' </tt>
**  \event <tt> 'Invalid msg length: ID = 0x\%04X, CC = \%d, Len = \%d, Expected = \%d' </tt>
**
**  \par Type: ERROR
**
**  \par Cause:
**
**  This event message is issued when a ground command message is received
**  with a message length that doesn't match the expected value.
**
**  The \c ID field contains the message ID, the \c CC field contains the
**  command code, the \c Len field is the actual length returned by the
**  #CFE_SB_GetTotalMsgLength call, and the \c Expected field is the expected
**  length for a message with that command code.
*/
#define SBN_LEN_ERR_EID 9

/** \brief <tt> 'No-op command' </tt>
**  \event <tt> 'No-op command' </tt>
**
**  \par Type: INFORMATIONAL
**
**  \par Cause:
**
**  This event message is issued when a NOOP command has been received.
*/
#define SBN_NOOP_INF_EID 10

/** \brief <tt> 'Reset counters command' </tt>
**  \event <tt> 'Reset counters command' </tt>
**
**  \par Type: DEBUG
**
**  \par Cause:
**
**  This event message is issued when a reset counters command has
**  been received.
*/
#define SBN_RESET_DBG_EID 11

/** \brief <tt> 'Get peer list command' </tt>
**  \event <tt> 'Get peer list command' </tt>
**
**  \par Type: DEBUG
**
**  \par Cause:
**
**  This event message is issued when a get peer list command has 
**  been received.
*/
#define SBN_PEER_LIST_DBG_EID 12

/** \brief <tt> 'Get peer status command' </tt>
**  \event <tt> 'Get peer status command' </tt>
**
**  \par Type: DEBUG
**
**  \par Cause:
**
**  This event message is issued when a get peer status command has 
**  been received.
*/
#define SBN_PEER_STATUS_DBG_EID 13

/** \brief <tt> 'Reset peer command' </tt>
**  \event <tt> 'Reset peer command' </tt>
**
**  \par Type: DEBUG
**
**  \par Cause:
**
**  This event message is issued when a reset peer command has 
**  been received.
*/
#define SBN_RESET_PEER_DBG_EID 14

/** \brief <tt> 'Command not implemented' </tt>
**  \event <tt> 'Command not implemented' </tt>
**
**  \par Type: INFORMATIONAL
**
**  \par Cause:
**
**  This event message is issued when a command is sent that is 
**  not implemented by a particular interface module.
*/
#define SBN_CMD_NOT_IMPLEMENTED_INFO_EID 15

/** \brief <tt> 'Get subscriptions command' </tt>
**  \event <tt> 'Get subscriptions command' </tt>
**
**  \par Type: DEBUG
**
**  \par Cause:
**
**  This event message is issued when the Software Bus Network 
**  receives a get local subscriptions command.
*/
#define SBN_GET_SUBS_DBG_EID 16

/** \brief <tt> 'Get peer subscriptions command' </tt>
**  \event <tt> 'Get peer subscriptions command' </tt>
**
**  \par Type: DEBUG
**
**  \par Cause:
**
**  This event message is issued when the Software Bus Network 
**  receives a get peer subscriptions command.
*/
#define SBN_GET_PEER_SUBS_DBG_EID 17

/*
** Event IDs
*/
#define SBN_MSGID_ERR_EID             2
#define SBN_FILE_ERR_EID              3
#define SBN_PROTO_INIT_ERR_EID        4
#define SBN_VOL_FAILED_EID            5
#define SBN_NONVOL_FAILED_EID         6
#define SBN_FILE_OPENED_EID           8
#define SBN_PEERPIPE_CR_ERR_EID       10

/*#define SBN_SOCK_FAIL_EID             12*/
/*#define SBN_BIND_FAIL_EID             13*/
#define SBN_PEERPIPE_CR_EID           14

#define SBN_MSG_TRUNCATED_EID         21
#define SBN_LSC_ERR1_EID              22
#define SBN_SUBTYPE_ERR_EID           23
#define SBN_PEERIDX_ERR1_EID          32
#define SBN_SUB_ERR_EID               33
#define SBN_DUP_SUB_EID               34
#define SBN_PEERIDX_ERR2_EID          35
#define SBN_NO_SUBS_EID               36
#define SBN_SUB_NOT_FOUND_EID         37
#define SBN_SB_SEND_ERR_EID           39
#define SBN_MSGTYPE_ERR_EID           40
#define SBN_LSC_ERR2_EID              41
#define SBN_ENTRY_ERR_EID             42
#define SBN_PEERIDX_ERR3_EID          43
#define SBN_UNSUB_CNT_EID             44
#define SBN_PROTO_SEND_ERR_EID        46

#define SBN_PIPE_ERR_EID              47
#define SBN_PEER_LIMIT_EID            48

#endif /* _sbn_events_h_ */
