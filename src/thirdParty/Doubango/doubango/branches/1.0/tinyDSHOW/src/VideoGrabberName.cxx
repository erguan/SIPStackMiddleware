/*
* Copyright (C) 2009-2010 Mamadou Diop.
*
* Contact: Mamadou Diop <diopmamadou(at)doubango.org>
*	
* This file is part of Open Source Doubango Framework.
*
* DOUBANGO is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*	
* DOUBANGO is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*	
* You should have received a copy of the GNU General Public License
* along with DOUBANGO.
*
*/
#include "tinydshow/VideoGrabberName.h"

VideoGrabberName::VideoGrabberName(std::string name_, std::string descr) : name(name_), description(descr)
{
}

std::string VideoGrabberName::getName() const
{
	return this->name;
}

std::string VideoGrabberName::getDescription() const
{
	return this->description;
}

int VideoGrabberName::operator==(const VideoGrabberName &dev) const
{
	return this->name == dev.name;
}