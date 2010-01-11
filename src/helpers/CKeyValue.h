/* -*- mode: c++; tab-width: 2; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 * vim:set ts=2 sw=2 expandtab: *********************************************
 *
 * libmedio - C++ I/O Library for loading/saving medical data formats
 * Copyright (C) 2006-2010 by Jens Langner <Jens.Langner@light-speed.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id$
 *
 **************************************************************************/

#ifndef CKEYVALUE_H
#define CKEYVALUE_H

#include <QString>

#include "CKeyType.h"

class CKeyValue
{
  public:
    //constructors
    CKeyValue(CKeyType::Type kType, void* value);
    CKeyValue(CKeyType::Type kType, void* p2Object, void (*p2Function)(void* p2Object, QString KeyValue));
    
    //detructors
    ~CKeyValue(){Value = 0;};
    
    //methods
    CKeyType::Type getType(){return KType;};
    void* getPToValue(){return Value;};
    void callBack(QString KeyValue);

  private:
    CKeyType::Type KType;
    void* Value;
    void (*P2Function)(void* p2Object, QString KeyValue);
};

#endif
