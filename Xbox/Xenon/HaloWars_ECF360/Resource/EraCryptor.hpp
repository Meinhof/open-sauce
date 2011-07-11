/*
    OpenSauce: HaloWars Data Interop

    Copyright (C)  Kornner Studios (http://kornner.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

namespace Phoenix
{
	class c_era_cryptor
	{
	public:
		static void DecryptMemory(void* buffer, size_t buffer_size);
		static void EncryptMemory(void* buffer, size_t buffer_size);
	};

	class c_campaign_save_cryptor
	{
	public:
		static void DecryptMemory(void* buffer, size_t buffer_size);
		static void EncryptMemory(void* buffer, size_t buffer_size);
	};
};