/* vim:set ts=2 nowrap: ****************************************************

 LMMC - ListMode Movement Correction
 Copyright (C) 2003 by Jens Langner <Jens.Langner@light-speed.de>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 $Id$

***************************************************************************/

#include "CGetOpt.h"
#include <qstring.h>

#include <string>
#include <iostream>
#include <iomanip>

#include <rtdebug.h>

bool CGetOpt::parse(int argc, char* argv[])
{
	ENTER();
	
	bool result = true;
	int iLastArgc = 1;

	while(iLastArgc < argc && result == true)
	{
		char* curOption = argv[iLastArgc++];
		if(curOption[0] == '-')
		{
			// now we need to analyze which option this could be
			bool found = false;
			CGetOptOption* curArgOption = 0;
			if(curOption[1] == '-')
			{
		    Q3PtrListIterator<CGetOptCategory> it(*this);
				for(; !found && it.current(); ++it)
				{
					Q3PtrListIterator<CGetOptOption> it2(*it.current());
					while((curArgOption = it2.current()) != 0)
					{
						++it2;

						char* curArgLongOpt = curArgOption->getLongOption();

						if(curArgLongOpt &&
							 strncmp(&curOption[2], curArgLongOpt, strlen(curArgLongOpt)) == 0)
						{
							D("long option --%s found", curArgLongOpt);
							found = true;

							curOption += strlen(curArgLongOpt)+2;
							break;
						}
					}
				}
			}
			else if(isalnum(curOption[1]) &&
							(curOption[2] == '\0' || curOption[2] == '=')
						 )
			{
		    Q3PtrListIterator<CGetOptCategory> it(*this);
				for(; !found && it.current(); ++it)
				{	
					Q3PtrListIterator<CGetOptOption> it2(*it.current());
					while((curArgOption = it2.current()) != 0)
					{
						++it2;

						if(curOption[1] == curArgOption->getShortOption())
						{
							D("option -%c found", curOption[1]);
						
							found = true;
							curOption += 2;
							break;
						}
					}
				}
			}
			else
			{
				result = false;
				std::cerr << argv[0] << ": invalid or unknown option '" << curOption << "'";
				std::cerr << std::endl;
			}

			if(found)
			{
				// now that we found the option within our argument
				// vector we need to analyze what to do with it
				if(curOption[0] == '=')
				{
					if(curOption[1] != '\0')
					{
						curArgOption->setFoundValue(&curOption[1]);
					}
					else
					{
						// if this isn't a flag option we have to drop an error
						E("value for option -%c is missing!", curArgOption->getShortOption());
						result = false;
					}						
				}
				else if(curOption[0] == '\0')
				{
					// we check the next argument in the vector because
					// it can be the value for the found option, but only
					// if it doesn't start with a "-"
					if(argv[iLastArgc] && argv[iLastArgc][0] != '-')
					{
						D("value for option -%c found", curArgOption->getShortOption());

						curArgOption->setFoundValue(argv[iLastArgc]);
						iLastArgc++; // increase it so that the next iteration ignores it.
					}
					else
					{
						if(curArgOption->getType() == CGetOptOption::Flag)
						{
							curArgOption->setFoundValue("1");
						}
						else
						{
							// if this isn't a flag option we have to drop an error
							std::cerr << argv[0] << ": value for option '-" << curArgOption->getShortOption() << "' is missing.";
							std::cerr << std::endl;
							result = false;
						}
					}
				}
				else
				{
					std::cerr << argv[0] << ": error in specified option '-" << curArgOption->getShortOption() << "'.";
					std::cerr << std::endl;
					result = false;
				}

				// now we need to check the type of the foundValue
				if(curArgOption->getFoundValue())
				{
					QString str(curArgOption->getFoundValue());
					bool validType = false;

					switch(curArgOption->getType())
					{
						case CGetOptOption::String: { validType = true;					} break;
						case CGetOptOption::Int:		{ str.toInt(&validType);		} break;
						case CGetOptOption::Short:	{ str.toShort(&validType);	} break;
						case CGetOptOption::Long:		{ str.toLong(&validType);		} break;
						case CGetOptOption::Float:	{ str.toFloat(&validType);	} break;
						case CGetOptOption::Flag:
						{
							if((str.toShort(&validType) == 0 || str.toShort(&validType) == 1) && validType) { }
							else if(str.lower() == "yes")
							{
								curArgOption->setFoundValue("1");
								validType = true;
							}
							else if(str.lower() == "no")
							{
								curArgOption->setFoundValue("0");
								validType = true;
							}
							else validType = false;
						}
						break;
					}

					if(!validType)
					{
						std::cerr << argv[0] << ": invalid value type for option '-" << curArgOption->getShortOption() << "'.";
						std::cerr << std::endl;
						result = false;
					}
				}
			}
			else
			{
				result = false;
				std::cerr << argv[0] << ": unknown option '" << curOption << "'";
				std::cerr << std::endl;
			}				
		}
	}

	RETURN(result);
	return result;
}

void CGetOpt::showAllOptions(void)
{
	// format the Options output
  Q3PtrListIterator<CGetOptCategory> it(*this);
	for(; it.current(); ++it)
	{	
		it.current()->showOptions();
	}
}

void CGetOpt::showCategoryOptions(QString category)
{
  Q3PtrListIterator<CGetOptCategory> it(*this);
	CGetOptCategory* curCategory;
	while((curCategory = it.current()))
	{	
		++it;

		if(curCategory->getName() == category)
		{
			curCategory->showOptions();
			break;
		}
	}
}

void CGetOptOption::show(void)
{
	QString outputString;

	outputString += "  -";
	outputString += m_ShortOption;
	if(m_pLongOption)
	{
		outputString += ", --";
		outputString += m_pLongOption;
	}

	if(m_pDefaultValue)
	{
		if(m_Type == CGetOptOption::Flag) outputString += "[";
		outputString += "=";
		outputString += m_pDefaultValue;
		if(m_Type == CGetOptOption::Flag) outputString += "]";
	}
	else
	{
		switch(m_Type)
		{
			case CGetOptOption::String: { outputString += "=STRING";		} break;
			case CGetOptOption::Int:		{ outputString += "=INT";				} break;
			case CGetOptOption::Short:	{ outputString += "=SHORT";			} break;
			case CGetOptOption::Long:		{ outputString += "=LONG";			} break;
			case CGetOptOption::Float:	{ outputString += "=FLOAT";			} break;
			case CGetOptOption::Flag:		{ outputString += "[=yes/no]";	} break;
		}
	}
	
	// now we have to analyse how many spaces we have to put after
	// the longoption so that we have all descriptions aligned
	int spaces = 25 - outputString.length();
	while(spaces-- > 0)
	{
		outputString += " ";
	}
	outputString += m_pDescription;

	std::cout << outputString.toAscii().data() << std::endl;
}

void CGetOptCategory::showOptions(void)
{
	std::cout << m_Name.toAscii().data() << ":" << std::endl;
		
	Q3PtrListIterator<CGetOptOption> it(*this);
	CGetOptOption* curArgOption;
	while((curArgOption = it.current()) != 0)
	{
		++it;
		curArgOption->show();
	}

	std::cout << std::endl;
}
