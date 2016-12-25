/*
 * This file is part of Log Writer Library (LWL).
 * Copyright (C) 2002, 2003, 2004, 2005 Nicolas Darnis <ndarnis@free.fr>
 *
 * The LWL library is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * The LWL library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the LWL library; see the file COPYING.
 * If not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 *
 * $RCSfile: lwl.h,v $
 * $Revision: 1.15 $
 * $Date: 2005/01/05 16:05:22 $
 */

#ifndef _LWL_H_
#define _LWL_H_

#include <stdio.h>


#if defined (__cplusplus)
extern "C" {
#endif /* __cplusplus */


/**
 * @defgroup LWL Log Writer Library
 * @{
 */


/**
 * @brief LWL Tags
 *
 * This is the list of available lwl_tag_t tags for use with lwl_set_attributes().
 *
 * @see lwl_set_attributes()
 */
typedef enum 
{
  /** Special tag to end the tag list */ 
  LWL_TAG_DONE = 0,

  /** Tag to modify the level of a LWL handle */ 
  LWL_TAG_LEVEL,

  /** Tag to set the prefix of the logged messages */
  LWL_TAG_PREFIX,

  /** Tag to set options to use in a LWL handme */
  LWL_TAG_OPTIONS,  

  /** Tag to set the default file used to write messages in */
  LWL_TAG_FILE,

  /** Tag to set the file for EMERG level */
  LWL_TAG_FILE_EMERG,

  /** Tag to set the file for ALERT level */
  LWL_TAG_FILE_ALERT,

  /** Tag to set the file for CRIT level */
  LWL_TAG_FILE_CRIT,

  /** Tag to set the file for ERR level */
  LWL_TAG_FILE_ERR,

  /** Tag to set the file for WARNING level */
  LWL_TAG_FILE_WARNING,

  /** Tag to set the file for NOTICE level */
  LWL_TAG_FILE_NOTICE,

  /** Tag to set the file for INFO level */
  LWL_TAG_FILE_INFO,

  /** Tag to set the file for DEBUG level */
  LWL_TAG_FILE_DEBUG

} lwl_tag_t;

/**
 * @brief LWL options
 *
 * This is the list of available LWL options for use with lwl_set_attributes().
 * The options could be XOR-ed to use more than one option at the same time.
 *
 * Example: LWL_OPT_ADR | LWL_OPT_PID | LWL_OPT_DATE | LWL_OPT_TIME means
 *          you want to use all these options for each logged message.
 *
 * @see lwl_set_attributes()
 */
typedef enum 
{
  /** No option (default) */
  LWL_OPT_NONE=0,

  /** The message is also logged to stderr */
  LWL_OPT_PERROR=1,   

  /** Write the pid of the process */
  LWL_OPT_PID=2,  

  /** Write the date */
  LWL_OPT_DATE=4, 

  /** Write the time */                
  LWL_OPT_TIME=8, 

  /** Write host's IP address */                   
  LWL_OPT_IP=16, 

  /** Write host's FQDN */   
  LWL_OPT_ADR=32,  

  /** Write the LWL_PRIORITY of the message */   
  LWL_OPT_PRIORITY=64,  

  /** Write prefix specified by LWL_TAG_PREFIX */
  LWL_OPT_PREFIX=128, 

  /** Don't flush the file after log writing */
  LWL_OPT_NO_FLUSH=256,

  /** Use locale environement settings to display date and time */
  LWL_OPT_USE_LOCALE=512

} lwl_option_t;

/**
 * @brief LWL priority
 *
 * This is the list of available LWL priorities for use with lwl_write_log()
 *
 * @see lwl_write_log()
 */
typedef enum 
{
  /** EMERGENCY level */
  LWL_PRI_EMERG=0,

  /** ALERT level */
  LWL_PRI_ALERT=1,

  /** CRITICAL level */
  LWL_PRI_CRIT=2,

  /** ERROR level */
  LWL_PRI_ERR=3,

  /** WARNING level */
  LWL_PRI_WARNING=4,

  /** NOTICE level */
  LWL_PRI_NOTICE=5,

  /** INFORMATION level */
  LWL_PRI_INFO=6,

  /** DEBUG level */
  LWL_PRI_DEBUG=7

} lwl_priority_t;

/**
 * @brief LWL handle type.
 *
 * This type is voluntary opaque. Variables of this kind could'nt be directly
 * used, but by the functions of this module.
 */
typedef struct _lwl_handle *lwlh_t;

/**
 * @brief Create a new LWL handle.
 *
 * Allocate a new LWL handle data structure.
 * The intial log file used is stdout and the initial level is LWL_PRI_DEBUG.
 * To change them and other attributes, uses lwl_set_attributes() function.
 *
 * Default values are: 
 * - LWL_PRI_DEBUG for LWL_TAG_LEVEL
 * - LWL_OPT_NONE for LWL_TAG_OPTIONS
 * - NULL for LWL_TAG_PREFIX
 * - stdout for LWL_TAG_FILE
 * - NULL for all LWL_TAG_FILE_xxx
 *
 * @pre nothing
 * @return the newly allocated LWL handle in case of success.
 * @return NULL in case of insufficient memory.
 * @see lwl_free()
 */
extern lwlh_t 
lwl_alloc (void);

/**
 * @brief Destroy a LWL handle.
 *
 * Deallocate and destroy the lwl HANDLE.
 *
 * @pre HANDLE must be a valid lwlh_t.
 * @param HANDLE The lwl handle to deallocate.
 * @see lwl_alloc()
 */
extern void
lwl_free (lwlh_t HANDLE
	  );

/**
 * @brief Log a message to the files associated to a LWL handle.
 *
 * Write the given formated message of priority PRI to the HANDLE's files.
 * To change the HANDLE's files, or log level, you can use lwl_set_attributes()
 * function. If no files were specified by LWL_TAG_FILE_xxx tags, then all logs
 * are written to the file specified by LWL_TAG_FILE (or to the default one if 
 * no LWL_TAG_FILE was specified). If a file was specified for a given level, by
 * use of tags like LWL_TAG_FILE_xxx, then the specified file is used instead 
 * of the default one.
 * The message FORMAT must be used like printf().
 * The message will be logged only if PRI is <= HANDLE's log level (wich you
 * can consult with lwl_get_log_level()).
 *
 * Example: 
 *
 * lwl_write_log (my_handle, 
 *                LWL_PRI_DEBUG, 
 *                "%s:%d-%s ERROR", 
 *                __FILE__, __LINE__, __FUNCTION__);
 *
 * @pre HANDLE must be a valid lwlh_t.
 * @param HANDLE The LWL handle to use.
 * @param PRI The priority of the message to log.
 * @param FORMAT The message to log.
 * @param ... Variables to include into the message.
 * @see lwl_get_log_level
 * @see lwl_set_attributes
 */
extern void
lwl_write_log (lwlh_t HANDLE,
	       lwl_priority_t PRI,
	       char *FORMAT,
	       ...
	       );

/**
 * @brief Modify the attributes of a LWL handle.
 *
 * To use this function, you must use LWL_TAGs to specify each tag you want
 * to set. The list of tags is fully determinated by the lwl_tag_t type.
 * Each tag must be followed by its new value, and the tag list must be 
 * terminated by LWL_TAG_DONE. If the tag list is not terminated by LWL_TAG_DONE 
 * the result of this function is not determined.
 *
 * Example: 
 *
 * lwl_set_attributes (my_handle, 
 *                     LWL_TAG_PREFIX, "my example app",
 *                     LWL_TAG_FILE, stderr,
 *                     LWL_TAG_OPTIONS, LWL_OPT_PRIORITY | LWL_OPT_DATE,
 *                     LWL_TAG_DONE);
 *
 * @pre HANDLE must be a valid lwlh_t.
 * @param HANDLE The LWL handle to modify.
 * @param TAGS, ... A coma separated list of (LWL_TAG_xxx, value) couples, 
 * terminated by the special LWL_TAG_DONE lwl_tag_t.
 * @return 0 in case of success.
 * @return -1 in case of failure.
 * @see lwl_tag_t
 */
extern int
lwl_set_attributes (lwlh_t HANDLE,
		    lwl_tag_t TAGS,
		    ...
		    );

/**
 * @brief Get the actual log level of a LWL handle.
 * @pre HANDLE must be a valid lwlh_t.
 * @return the actual log level of HANDLE.
 * @param HANDLE The LWL handle to use.
 */
extern lwl_priority_t
lwl_get_log_level (const lwlh_t HANDLE
		   );

/**
 * @brief Get the actual default file where a LWL handle is logging into.
 * @pre HANDLE must be a valid lwlh_t.
 * @param HANDLE The LWL handle to use.
 * @return the default file where HANDLE is actually logging.
 */
extern FILE*
lwl_get_default_log_file (const lwlh_t HANDLE
			  );

/**
 * @brief Alias for lwl_get_default_log_file()
 * @see lwl_get_default_log_file()
 */
#define lwl_get_log_file(h) lwl_get_default_log_file(h)

/**
 * @brief Get the hostname of a LWL handle.
 * @pre HANDLE must be a valid lwlh_t.
 * @param HANDLE The LWL handle to use.
 * @return the hostname if HANDLE's options includes LWL_OPT_IP and/or 
 * LWL_OPT_ADR.
 * @return NULL if HANDLE's options does not include LWL_OPT_IP and/or 
 * LWL_OPT_ADR.
 */
extern const char*
lwl_get_hostname (lwlh_t HANDLE
		  );


/*
 * @}
 */


#ifdef __cplusplus
}
#endif/* __cplusplus */


#endif /* _LWL_H_ */

/** EMACS **
 * Local variables:
 * mode: c
 * c-basic-offset: 4
 * End:
 */
