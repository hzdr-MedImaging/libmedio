/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio
 
  Copyright (C) 2004-2026 hzdr.de and contributors
 
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
 
    http://www.apache.org/licenses/LICENSE-2.0
 
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "MedIOUnits.h"

#include <QRegExp>

#include <rtdebug.h>

CWeight::CWeight()
  : m_value(0.0), 
    m_unit(Unknown_units)
{ 
  ENTER();
  LEAVE();
}

CWeight::CWeight(const CWeight& other)
  : m_value(other.m_value), 
    m_unit(other.m_unit)
{
  ENTER();
  LEAVE();
}

CWeight::CWeight(const float value, const WeightUnits u)
  : m_value(value), 
    m_unit(u)
{
  ENTER();
  LEAVE();
}
  
void CWeight::setWeight(const float value)
{
  m_value = value;
}

float CWeight::weight() const
{
  return m_value;
}

void CWeight::setUnit(const WeightUnits u)
{
  m_unit = u;
}

CWeight::WeightUnits CWeight::unit() const
{
  return m_unit;
}

QString CWeight::unitAsString() const
{ 
  ENTER();
  QString s;
  
  switch (m_unit)
  {
    case Gram_units: s = "g";break;
    case Ounce_units: s = "oz";break;
    case Kilogram_units: s = "kg";break;
    case Pound_units: s = "pounds";break;
    default: s = "unknown unit";break;
  }

  LEAVE();
  return s;
}
  
QString CWeight::toString() const
{
  return QString::number(m_value, 'f', 2) + " " + unitAsString();
}

CWeight CWeight::convertTo(const WeightUnits u) const
{
  ENTER();
  float r;
  
  switch(m_unit)
  {
    case Gram_units: 
    {
      switch(u)
      {
        case Gram_units: r = m_value; break;
        case Ounce_units: r = m_value / 28.349; break;
        case Kilogram_units: r = m_value / 1000.0; break;
        case Pound_units: r = m_value / 453.59237; break;
        default: r = 0.0; break;
      }
    }
    break;
    
    case Ounce_units:
    {
      switch (u)
      {
        case Gram_units: r = m_value * 28.349; break;
        case Ounce_units: r = m_value; break;
        case Kilogram_units: r = m_value * 0.028349; break;
        case Pound_units: r = m_value / 16; break;
        default: r = 0.0; break;
      }
    }
    break;
    
    case Kilogram_units:
    {
      switch (u)
      {
        case Gram_units: r = m_value * 1000.0; break;
        case Ounce_units: r = m_value / 0.028349; break;
        case Kilogram_units: r = m_value; break;
        case Pound_units: r = m_value / 0.45359237; break;
        default: r = 0.0; break;
      }
    }
    break;
    
    case Pound_units: 
    {
      switch (u)
      {
        case Gram_units: r = m_value * 453.59237; break;
        case Ounce_units: r = m_value / 0.028349; break;
        case Kilogram_units: r = m_value * 0.45359237; break;
        case Pound_units: r = m_value; break;
        default: r = 0.0; break;
      }
    }
    break;
    
    default: r = 0.0; break;
  }

  LEAVE();
  return CWeight(r, u);
}

CWeight& CWeight::operator=(const CWeight& other)
{
  m_value = other.m_value;
  m_unit = other.m_unit;
  return *this;
}

CLength::CLength()
  : m_value(0.0), 
    m_unit(Unknown_units)
{
  ENTER();
  LEAVE();
}

CLength::CLength(const CLength& other)
  : m_value(other.m_value), 
    m_unit(other.m_unit)
{
  ENTER();
  LEAVE();
}

CLength::CLength(const float value, const LengthUnits u)
  : m_value(value), 
    m_unit(u)
{
  ENTER();
  LEAVE();
}
  
void CLength::setLength(const float value)
{
  m_value = value;
}

float CLength::length() const
{
  return m_value;
}

void CLength::setUnit(const LengthUnits u)
{
  m_unit = u;
}

CLength::LengthUnits CLength::unit() const 
{
  return m_unit;
}

QString CLength::unitAsString() const
{ 
  ENTER();
  QString s;
  
  switch (m_unit)
  {
    case Millimeter_units: s = "mm";break;
    case Centimeter_units: s = "cm";break;
    case Meter_units: s = "m";break;
    case Inch_units: s = "in";break;
    default: s = "unknown unit";break;
  }

  LEAVE();
  return s;
}
  
QString CLength::toString() const
{
  return QString::number(m_value, 'f', 2) + " " + unitAsString();
}

CLength CLength::convertTo(const LengthUnits u) const
{
  ENTER();
  float r;

  switch(m_unit)
  {
    case Millimeter_units: 
    {
      switch(u)
      {
        case Millimeter_units: r = m_value; break;
        case Centimeter_units: r = m_value / 10; break;
        case Meter_units: r = m_value / 1000; break;
        case Inch_units: r = m_value / 25.4; break;
        default: r = 0.0; break;
      }
    }
    break;
    
    case Centimeter_units: 
    {
      switch (u)
      {
        case Millimeter_units: r = m_value * 10; break;
        case Centimeter_units: r = m_value; break;
        case Meter_units: r = m_value / 100; break;
        case Inch_units: r = m_value / 2.54; break;
        default: r = 0.0; break;
      }
    }
    break;
    
    case Meter_units: 
    {
      switch (u)
      {
        case Millimeter_units: r = m_value * 1000; break;
        case Centimeter_units: r = m_value * 100; break;
        case Meter_units: r = m_value; break;
        case Inch_units: r = m_value / 0.0254; break;
        default: r = 0.0; break;
      }
    }
    break;
    
    case Inch_units: 
    {
      switch (u)
      {
        case Millimeter_units: r = m_value * 25.4; break;
        case Centimeter_units: r = m_value * 2.54; break;
        case Meter_units: r = m_value * 0.0254; break;
        case Inch_units: r = m_value; break;
        default: r = 0.0; break;
      }
    }
    break;
    
    default: r = 0.0; break;
  }

  LEAVE();
  return CLength(r, u);
}

CLength& CLength::operator=(const CLength& other)
{
  m_value = other.m_value;
  m_unit = other.m_unit;
  return *this;
}

CActivityConcentrationUnit::CActivityConcentrationUnit(const ActivityConcentrationUnits u)
  : m_unit(u),
    m_unknownUnitString("unknown unit")
{
  ENTER();
  LEAVE();
}

CActivityConcentrationUnit::CActivityConcentrationUnit(const CActivityConcentrationUnit& other)
  : m_unit(other.m_unit),
    m_unknownUnitString(other.m_unknownUnitString)
{
  ENTER();
  LEAVE();
}

CActivityConcentrationUnit::CActivityConcentrationUnit(const QString& s)
  : m_unit(Unknown_units)
{
  ENTER();
  setUnitFromString(s);
  LEAVE();
}
  

void CActivityConcentrationUnit::setUnknownUnitString(const QString & S)
{

  ENTER();
  m_unknownUnitString = S;
  LEAVE();
}

QString CActivityConcentrationUnit::unknownUnitString() const
{
  ENTER();
  LEAVE();
  return m_unknownUnitString;
}

void CActivityConcentrationUnit::setUnit(const ActivityConcentrationUnits unit)
{
  ENTER();
  m_unit = unit;
  LEAVE();
}

void CActivityConcentrationUnit::setUnitFromString(const QString& s)
{
  ENTER();

  m_unit = Unknown_units;
  QRegExp t;
  t.setCaseSensitivity(Qt::CaseInsensitive);

  t.setPattern("\\s*Bq\\/?cc\\s*");
  if(t.exactMatch(s))
    m_unit = BequerelsPerCubiccentimeter_units;
  else
  {
    t.setPattern("\\s*Bq\\/?ml\\s*");
    if(t.exactMatch(s))
      m_unit = BequerelsPerMilliliter_units;
    else
    {
      t.setPattern("\\s*nCi\\/cc\\s*");
      if(t.exactMatch(s))
        m_unit = NanoCuriesPerCubiccentimeter_units;
    }
  }

  if(!s.isEmpty())
  {
    QString testString(s);
    if(!testString.remove(" ").isEmpty())
      m_unknownUnitString = s;
  }
  LEAVE();
}

CActivityConcentrationUnit::ActivityConcentrationUnits CActivityConcentrationUnit::unit() const
{
  return m_unit;
}

QString CActivityConcentrationUnit::unitAsString() const
{ 
  ENTER();
  QString s;

  switch(m_unit)
  {
    case BequerelsPerCubiccentimeter_units: s = "Bq/cc";break;
    case BequerelsPerMilliliter_units: s = "Bq/ml";break;
    case NanoCuriesPerCubiccentimeter_units: s = "nCi/cc";break;
    default: s = m_unknownUnitString;break;
  }

  LEAVE();
  return s;
}

QString CActivityConcentrationUnit::activityUnitAsString() const
{
  ENTER();
  QString s;

  switch (m_unit)
  {
    case BequerelsPerCubiccentimeter_units:
    case BequerelsPerMilliliter_units: s = "Bq";break;
    case NanoCuriesPerCubiccentimeter_units: s = "nCi";break;
    default: s = m_unknownUnitString;break;
  }

  LEAVE();
  return s;
}

QString CActivityConcentrationUnit::toString() const
{
  return unitAsString();
}

CActivityConcentrationUnit& CActivityConcentrationUnit::operator=(const CActivityConcentrationUnit& other)
{
  m_unit = other.m_unit;
  return *this;
}

CDosage::CDosage()
  : m_value(0.0),
    m_unit(Unknown_units)
{
  ENTER();
  LEAVE();
}

CDosage::CDosage(const CDosage& other)
  : m_value(other.m_value), 
    m_unit(other.m_unit)
{
  ENTER();
  LEAVE();
}

CDosage::CDosage(const float value, const DosageUnits u)
  : m_value(value), 
    m_unit(u)
{
  ENTER();
  LEAVE();
}
  
void CDosage::setDosage(const float value)
{
  m_value = value;
}

float CDosage::dosage() const
{
  return m_value;
}

void CDosage::setUnit(const DosageUnits unit)
{
  m_unit = unit;
}

void CDosage::setUnitFromString(const QString& s)
{
  ENTER();

  m_unit = Unknown_units;
  QRegExp t;
  t.setCaseSensitivity(Qt::CaseInsensitive);

  t.setPattern("\\s*MBq\\s*");
  if(t.exactMatch(s))
    m_unit = Megabequerels_units;
  else
  {
    t.setPattern("\\s*Bq\\s*");
    if(t.exactMatch(s))
      m_unit = Bequerels_units;
    else
    {
      t.setPattern("\\s*nCi\\s*");
      if(t.exactMatch(s))
        m_unit = Nanocurie_units;
      else
      {
        t.setPattern("\\s*mCi\\s*");
        if(t.exactMatch(s))
          m_unit = Millicurie_units;
      }
    }
  }

  LEAVE();
}

CDosage::DosageUnits CDosage::unit() const
{
  return m_unit;
}

QString CDosage::unitAsString() const
{ 
  ENTER();
  QString s;

  switch (m_unit)
  {
    case Bequerels_units: s = "Bq";break;
    case Megabequerels_units: s = "MBq";break;
    case Nanocurie_units: s = "nCi";break;
    case Millicurie_units: s = "mCi";break;
    default: s = "unknown unit";break;
  }

  LEAVE();
  return s;
}

QString CDosage::toString() const
{
  return QString::number(m_value, 'f', 2) + " " + unitAsString();
}

CDosage& CDosage::operator=(const CDosage& other)
{
  m_value = other.m_value;
  m_unit = other.m_unit;
  return *this;
}

CDosage CDosage::convertTo(DosageUnits u) const
{
  ENTER();
  float r;

  switch (m_unit)
  {
    case Bequerels_units: 
    {
      switch (u)
      {
        case Bequerels_units: r = m_value; break;
        case Megabequerels_units: r = m_value / 1e6; break;
        case Nanocurie_units: r = m_value / 3.7e1; break;
        case Millicurie_units: r = m_value / 3.7e7; break;
        default: r = 0.0; break;
      }
    }
    break;
    
    case Megabequerels_units: 
    {
      switch (u)
      {
        case Bequerels_units: r = m_value * 1e6; break;
        case Megabequerels_units: r = m_value; break;
        case Nanocurie_units: r = m_value  / 3.7e-5; break;
        case Millicurie_units: r = m_value / 3.7e1; break;
        default: r = 0.0; break;
      }
    }
    break;
    
    case Nanocurie_units: 
    {
      switch (u)
      {
        case Bequerels_units: r = m_value * 3.7e1; break;
        case Megabequerels_units: r = m_value * 3.7e-5; break;
        case Nanocurie_units: r = m_value; break;
        case Millicurie_units: r = m_value * 1e-6; break;
        default: r = 0.0; break;
      }
    }
    break;
    
    case Millicurie_units: 
    {
      switch (u)
      {
        case Bequerels_units: r = m_value * 3.7e7; break;
        case Megabequerels_units: r = m_value * 3.7e1; break;
        case Nanocurie_units: r = m_value * 1e6; break;
        case Millicurie_units: r = m_value; break;
        default: r = 0.0; break;
      }
    }
    break;
    
    default: r = 0.0; break;
  }

  LEAVE();
  return CDosage(r, u);
}

CDosage CDosage::convertTo(const CActivityConcentrationUnit& a) const
{
  ENTER();

  switch(a.unit())
  {
    case CActivityConcentrationUnit::BequerelsPerCubiccentimeter_units: return convertTo(Bequerels_units); break;
    case CActivityConcentrationUnit::BequerelsPerMilliliter_units: return convertTo(Bequerels_units); break;
    case CActivityConcentrationUnit::NanoCuriesPerCubiccentimeter_units: return convertTo(Nanocurie_units); break;
    default: return CDosage(0.0, Unknown_units);
  }

  LEAVE();
}
