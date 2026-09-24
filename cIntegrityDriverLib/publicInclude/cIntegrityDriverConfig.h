/** ***********************************************
 * @file cIntegrityDriverConfig.h
 * @brief Overridable configuration of the integrity driver.
 * @author Anthony Garza
 * @copyright All rights reserved 2026
*************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "commonMacros.h"
#ifndef C_INTEGRITY_DRIVER_CONFIG_H
#define C_INTEGRITY_DRIVER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CUSTOM_INTEGRITY_DRIVER_CONFIG


#if ( EMBEDDED_OPTIMIZED_BUILD == DEF_TRUE )
#define CRC_16_EMBEDDED_OPTIMIZED_CRC16                                 DEF_TRUE
#else
#define CRC_16_EMBEDDED_OPTIMIZED_CRC16                                 DEF_FALSE
#endif

#ifndef INTEGRITY_LOGGING_ENABLED
#define INTEGRITY_LOGGING_ENABLED                                                DEF_TRUE
#endif

#ifndef LOG_FULL_ERROR_MESSAGE
#define LOG_FULL_ERROR_MESSAGE                                               DEF_TRUE
#endif

#ifndef ERROR_MESSAGE_FULL
#define ERROR_MESSAGE_FULL                                                   DEF_TRUE
#endif

#endif // CUSTOM_INTEGRITY_DRIVER_CONFIG

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* C_INTEGRITY_DRIVER_CONFIG_H */