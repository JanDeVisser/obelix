/* 
 * File:   types.h
 * Author: jan
 *
 * Created on February 24, 2015, 4:54 PM
 */

#ifndef __TYPES_H__
#define	__TYPES_H__

#include <stdarg.h>
#include <data.h>

#ifdef	__cplusplus
extern "C" {
#endif

extern data_t * execute(data_t *, const char *, int, ...);

#ifdef	__cplusplus
}
#endif

#endif	/* __TYPES_H__ */

