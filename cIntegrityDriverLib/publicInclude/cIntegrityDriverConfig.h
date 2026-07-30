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

#endif // CUSTOM_INTEGRITY_DRIVER_CONFIG

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* C_INTEGRITY_DRIVER_CONFIG_H */