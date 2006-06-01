/* vim:set ts=2 nowrap: ****************************************************

 libmedio - Medical Data C++ I/O Library
 Copyright (C) 2004 by Jens Langner <Jens.Langner@light-speed.de>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

***************************************************************************/

#ifndef MEDIOUNITS_H
#define MEDIOUNITS_H

// Qt includes
#include <QObject>
#include <QRegExp>

class CWeight: public QObject
{
	Q_OBJECT

public:
	enum WeightUnits { Unknown_units = 0,
										 Gram_units,
										 Ounce_units,
										 Kilogram_units,
										 Pound_units };

	CWeight():m_value(0.0), m_unit(Unknown_units) {};
	CWeight(const CWeight& other):QObject(), m_value(other.m_value), m_unit(other.m_unit) {};
	CWeight(float value, WeightUnits u = Unknown_units):m_value(value), m_unit(u) {};
	
	void setWeight(float value) {m_value = value;};
	float weight() const {return m_value;};

	void setUnit(WeightUnits u) {m_unit = u;};
	WeightUnits unit() const {return m_unit;};

	QString unitAsString() const
	{ 
		QString s;
		switch (m_unit)
		{
			case Gram_units: s = tr("g");break;
			case Ounce_units: s = tr("oz");break;
			case Kilogram_units: s = tr("kg");break;
			case Pound_units: s = tr("pounds");break;
			default: s = tr("unknown unit");break;
		}
		return s;
	};
	
	QString toString() const {return QString::number(m_value, 'f', 2) + " " + unitAsString();};
	CWeight convertTo(WeightUnits u) const
	{
		float r;
		switch (m_unit)
		{
			case Gram_units: 
			{
				switch (u)
				{
					case Gram_units: r = m_value; break;
					case Ounce_units: r = m_value / 28.349; break;
					case Kilogram_units: r = m_value / 1000.0; break;
					case Pound_units: r = m_value / 453.59237; break;
					default: r = 0.0; break;
				}
			};break;
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
			};break;
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
			};break;
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
			};break;
			default: r = 0.0; break;
		}
		return CWeight(r, u);
	};

	CWeight& operator=(const CWeight& other) {m_value = other.m_value; m_unit = other.m_unit; return *this;}

protected:
	// Private member vars
	float m_value;
	WeightUnits m_unit;	
};

class CLength: public QObject
{
	Q_OBJECT

public:
	enum LengthUnits { Unknown_units = 0,
										 Millimeter_units,
										 Centimeter_units,
										 Meter_units,
										 Inch_units };

	CLength():m_value(0.0), m_unit(Unknown_units) {};
	CLength(const CLength& other):QObject(), m_value(other.m_value), m_unit(other.m_unit) {};
	CLength(float value, LengthUnits u = Unknown_units):m_value(value), m_unit(u) {};
	
	void setLength(float value) {m_value = value;};
	float length() const {return m_value;};

	void setUnit(LengthUnits u) {m_unit = u;};
	LengthUnits unit() const {return m_unit;};

	QString unitAsString() const
	{ 
		QString s;
		switch (m_unit)
		{
			case Millimeter_units: s = tr("mm");break;
			case Centimeter_units: s = tr("cm");break;
			case Meter_units: s = tr("m");break;
			case Inch_units: s = tr("in");break;
			default: s = tr("unknown unit");break;
		}
		return s;
	};
	
	QString toString() const {return QString::number(m_value, 'f', 2) + " " + unitAsString();};

	CLength convertTo(LengthUnits u) const
	{
		float r;
		switch (m_unit)
		{
			case Millimeter_units: 
			{
				switch (u)
				{
					case Millimeter_units: r = m_value; break;
					case Centimeter_units: r = m_value / 10; break;
					case Meter_units: r = m_value / 1000; break;
					case Inch_units: r = m_value / 25.4; break;
					default: r = 0.0; break;
				}
			};break;
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
			};break;
			case Meter_units: 
			{
				switch (u)
				{
					case Millimeter_units: r = m_value * 1000; break;
					case Centimeter_units: r = m_value * 100; break;
					case Meter_units: r = m_value; break;
					case Inch_units: r = m_value * 0.0254; break;
					default: r = 0.0; break;
				}
			};break;
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
			};break;
			default: r = 0.0; break;
		}
		return CLength(r, u);
	}

	CLength& operator=(const CLength& other) {m_value = other.m_value; m_unit = other.m_unit; return *this;}

protected:
	// Private member vars
	float m_value;
	LengthUnits m_unit;
};

class CActivityConcentrationUnit: public QObject
{
	Q_OBJECT

public:
	enum ActivityConcentrationUnits { Unknown_units = 0,
																		BequerelsPerCubiccentimeter_units,
																		BequerelsPerMilliliter_units,
																		NanoCuriesPerCubiccentimeter_units,
																	};

	CActivityConcentrationUnit(ActivityConcentrationUnits u = Unknown_units):m_unit(u) {};
	CActivityConcentrationUnit(const CActivityConcentrationUnit& other):QObject(), m_unit(other.m_unit) {};
	CActivityConcentrationUnit(const QString& s):m_unit(Unknown_units) {setUnitFromString(s);};
	
	void setUnit(ActivityConcentrationUnits unit) {m_unit = unit;};
	void setUnitFromString(const QString& s)
	{
		m_unit = Unknown_units;
		QRegExp t;
		t.setCaseSensitivity(Qt::CaseInsensitive);
		t.setPattern("\\s*Bq\\/?cc\\s*");
		if(t.exactMatch(s))
			m_unit = BequerelsPerCubiccentimeter_units;
		t.setPattern("\\s*Bq\\/?ml\\s*");
		if(t.exactMatch(s))
			m_unit = BequerelsPerMilliliter_units;
		t.setPattern("\\s*nCi\\/cc\\s*");
		if(t.exactMatch(s))
			m_unit = NanoCuriesPerCubiccentimeter_units;
	};
	ActivityConcentrationUnits unit() const {return m_unit;};

	QString unitAsString() const
	{ 
		QString s;
		switch (m_unit)
		{
			case BequerelsPerCubiccentimeter_units: s = tr("Bq/cc");break;
			case BequerelsPerMilliliter_units: s = tr("Bq/ml");break;
			case NanoCuriesPerCubiccentimeter_units: s = tr("nCi/cc");break;
			default: s = tr("unknown unit");break;
		}
		return s;
	};
	QString activityUnitAsString() const
	{
		QString s;
		switch (m_unit)
		{
			case BequerelsPerCubiccentimeter_units:
			case BequerelsPerMilliliter_units: s = tr("Bq");break;
			case NanoCuriesPerCubiccentimeter_units: s = tr("nCi");break;
			default: s = tr("unknown unit");break;
		}
		return s;
	};

	QString toString() const {return unitAsString();};

	CActivityConcentrationUnit& operator=(const CActivityConcentrationUnit& other) {m_unit = other.m_unit; return *this;}

protected:
	// Private member vars
	ActivityConcentrationUnits m_unit;
};

class CDosage: public QObject
{
	Q_OBJECT

public:
	enum DosageUnits { Unknown_units = 0,
										 Bequerels_units,
										 Megabequerels_units,
										 Nanocurie_units,
										 Millicurie_units };

	CDosage():m_value(0.0), m_unit(Unknown_units) {};
	CDosage(const CDosage& other):QObject(), m_value(other.m_value), m_unit(other.m_unit) {};
	CDosage(float value, DosageUnits u = Unknown_units):m_value(value), m_unit(u) {};
	
	void setDosage(float value) {m_value = value;};
	float dosage() const {return m_value;};

	void setUnit(DosageUnits unit) {m_unit = unit;};
	DosageUnits unit() const {return m_unit;};

	QString unitAsString() const
	{ 
		QString s;
		switch (m_unit)
		{
			case Bequerels_units: s = tr("Bq");break;
			case Megabequerels_units: s = tr("MBq");break;
 			case Nanocurie_units: s = tr("nCi");break;
			case Millicurie_units: s = tr("mCi");break;
			default: s = tr("unknown unit");break;
		}
		return s;
	};

	QString toString() const {return QString::number(m_value, 'f', 2) + " " + unitAsString();};

	CDosage& operator=(const CDosage& other) {m_value = other.m_value; m_unit = other.m_unit; return *this;}
	CDosage convertTo(DosageUnits u) const
	{
		float r;
		switch (m_unit)
		{
			case Bequerels_units: 
			{
				switch (u)
				{
					case Bequerels_units: r = m_value; break;
					case Megabequerels_units: r = m_value / 1e6; break;
					case Nanocurie_units: r = m_value * 3.7e10 * 1e-9; break;
					case Millicurie_units: r = m_value * 3.7e10 * 1e-3; break;
					default: r = 0.0; break;
				}
			};break;
			case Megabequerels_units: 
			{
				switch (u)
				{
					case Bequerels_units: r = m_value * 1e6; break;
					case Megabequerels_units: r = m_value; break;
					case Nanocurie_units: r = m_value * 1e6 * 3.7e10 * 1e-9; break;
					case Millicurie_units: r = m_value * 1e6 * 3.7e10 * 1e-3; break;
					default: r = 0.0; break;
				}
			};break;
			case Nanocurie_units: 
			{
				switch (u)
				{
					case Bequerels_units: r = m_value * 1e-9 / 3.7e10; break;
					case Megabequerels_units: r = m_value * 1e-9 / 3.7e10 / 1e6; break;
					case Nanocurie_units: r = m_value; break;
					case Millicurie_units: r = m_value * 1e-9 / 1e-3; break;
					default: r = 0.0; break;
				}
			};break;
			case Millicurie_units: 
			{
				switch (u)
				{
					case Bequerels_units: r = m_value * 1e-3 / 3.7e10; break;
					case Megabequerels_units: r = m_value * 1e-3 / 3.7e10 * 1e-6; break;
					case Nanocurie_units: r = m_value * 1e3 / 1e-9; break;
					case Millicurie_units: r = m_value; break;
					default: r = 0.0; break;
				}
			};break;
			default: r = 0.0; break;
		}
		return CDosage(r, u);
	};
	CDosage convertTo(const CActivityConcentrationUnit& a) const
	{
		switch(a.unit())
		{
			case CActivityConcentrationUnit::BequerelsPerCubiccentimeter_units: return convertTo(Bequerels_units); break;
			case CActivityConcentrationUnit::BequerelsPerMilliliter_units: return convertTo(Bequerels_units); break;
			case CActivityConcentrationUnit::NanoCuriesPerCubiccentimeter_units: return convertTo(Nanocurie_units); break;
			default: return CDosage(0.0, Unknown_units);
		}
	};
protected:
	// Private member vars
	float m_value;
	DosageUnits m_unit;
};

#endif // MEDIOUNITS_H
