/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
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

#define Uses_SCIM_TRANSACTION
#define Uses_ISF_IMCONTROL_CLIENT


#include <string.h>
#include "scim.h"


namespace scim
{

int isf_control_set_active_ise_by_uuid (const char *uuid)
{
    IMControlClient imcontrol_client;
    imcontrol_client.open_connection ();
    imcontrol_client.prepare ();
    imcontrol_client.set_active_ise_by_uuid (uuid);
    imcontrol_client.close_connection ();
    return 0;
}

int isf_control_get_ise_list (char ***iselist)
{
    int count;
    IMControlClient imcontrol_client;
    imcontrol_client.open_connection ();
    imcontrol_client.prepare ();
    imcontrol_client.get_ise_list (&count, iselist);
    imcontrol_client.close_connection ();
    return count;
}

int isf_control_reset_ise_option (void)
{
    IMControlClient imcontrol_client;
    imcontrol_client.open_connection ();
    imcontrol_client.prepare ();
    imcontrol_client.reset_ise_option ();
    imcontrol_client.close_connection ();
    return 0;
}

};

/*
vi:ts=4:nowrap:ai:expandtab
*/
