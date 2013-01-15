/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Haifeng Deng <haifeng.deng@samsung.com>, Hengliang Luo <hl.luo@samsung.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef __ISF_CONTROL_H
#define __ISF_CONTROL_H

namespace scim
{
/////////////////////////////////////////////////////////////////////////////
// Declaration of global data types.
/////////////////////////////////////////////////////////////////////////////
typedef enum
{
    HARDWARE_KEYBOARD_ISE = 0,  /* Hardware keyboard ISE */
    SOFTWARE_KEYBOARD_ISE       /* Software keyboard ISE */
} ISE_TYPE_T;


/////////////////////////////////////////////////////////////////////////////
// Declaration of global functions.
/////////////////////////////////////////////////////////////////////////////
/**
 * @brief Set active ISE by UUID.
 *
 * @param uuid The active ISE's UUID.
 *
 * @return 0 if successfully, otherwise return -1;
 */
int isf_control_set_active_ise_by_uuid (const char *uuid);

/**
 * @brief Get active ISE's UUID.
 *
 * @param uuid The parameter is used to store active ISE's UUID.
 *             Applcation need free *uuid if it is not used.
 *
 * @return the length of UUID if successfully, otherwise return -1;
 */
int isf_control_get_active_ise (char **uuid);

/**
 * @brief Get the list of all ISEs' UUID.
 *
 * @param uuid_list The list is used to store all ISEs' UUID.
 *                  Applcation need free **uuid_list if it is not used.
 *
 * @return the count of UUID list if successfully, otherwise return -1;
 */
int isf_control_get_ise_list (char ***uuid_list);

/**
 * @brief Get ISE's information according to ISE's UUID.
 *
 * @param uuid The ISE's UUID.
 * @param name     The parameter is used to store ISE's name. Applcation need free *name if it is not used.
 * @param language The parameter is used to store ISE's language. Applcation need free *language if it is not used.
 * @param type     The parameter is used to store ISE's type.
 * @param option   The parameter is used to store ISE's option.
 *
 * @return 0 if successfully, otherwise return -1;
 */
int isf_control_get_ise_info (const char *uuid, char** name, char** language, ISE_TYPE_T &type, int &option);

/**
 * @brief Reset all ISEs' options.
 *
 * @return 0 if successfully, otherwise return -1;
 */
int isf_control_reset_ise_option (void);

}

#endif /* __ISF_CONTROL_H */

/*
vi:ts=4:nowrap:ai:expandtab
*/
