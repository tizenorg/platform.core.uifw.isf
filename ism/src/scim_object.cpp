/*
 * Most code of this file are came from Inti project.
 */

/*
 * Smart Common Input Method
 *
 * Copyright (c) 2002-2005 James Su <suzhe@tsinghua.org.cn>
 * Copyright (c) 2002 The Inti Development Team.
 *
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 *
 * $Id: scim_object.cpp,v 1.9 2005/01/10 08:30:54 suzhe Exp $
 */

#define Uses_SCIM_OBJECT
#include "scim_private.h"
#include "scim.h"

namespace scim {

/*  ReferencedObject
 */

ReferencedObject::ReferencedObject ()
    : m_referenced (true), m_ref_count (1)
{
}

ReferencedObject::~ReferencedObject ()
{
}

void
ReferencedObject::set_referenced (bool reference)
{
    m_referenced = reference;
}

bool
ReferencedObject::is_referenced() const
{
    return m_referenced;
}

void
ReferencedObject::ref()
{
    ++m_ref_count;
}

void
ReferencedObject::unref()
{
    if (--m_ref_count == 0)
        delete this;
}

} // namespace scim

/*
vi:ts=4:nowrap:ai:expandtab
*/
