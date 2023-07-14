﻿#include "Panel.h"

AFile_Descriptor::AFile_Descriptor(unsigned int attributes, unsigned int size_low, unsigned int size_high, wchar_t* file_name)
	:Attributes(attributes), File_Name(file_name)
{
	File_Size = (unsigned long long)size_high << 32 | (unsigned long long)size_low;
}

APanel::APanel(unsigned short x_pos, unsigned short y_pos, unsigned short width, unsigned short height, CHAR_INFO* screen_buffer, unsigned short screen_width)
	: X_Pos(x_pos), Y_Pos(y_pos), Width(width), Height(height), Screen_Buffer(screen_buffer), Screen_Width(screen_width)
{
}

void APanel::Draw()
{
	Draw_Panels();
	Draw_Files();
	Draw_Highlight();
}

void APanel::Get_Directory_Files(const std::wstring& root_dir)
{
	HANDLE search_handle;
	WIN32_FIND_DATAW find_data{};

	for (auto* file : Files)
		delete file;

	Files.erase(Files.begin(), Files.end());

	Current_Directory = root_dir;

	std::wstring file_path = root_dir + L"\\*.*";
	search_handle = FindFirstFileW(file_path.c_str(), &find_data);

	while (FindNextFileW(search_handle, &find_data))
	{
		AFile_Descriptor* file_descriptor = new AFile_Descriptor(find_data.dwFileAttributes, find_data.nFileSizeLow, find_data.nFileSizeHigh, find_data.cFileName);
		Files.push_back(file_descriptor);
	}

	Curr_File_Index = 0;
	Highlight_X_Offset = 0;
	Highlight_Y_Offset = 0;
}

void APanel::Move_Highlight(bool move_up)
{
	if (move_up) {
		if (Curr_File_Index > 0)
		{
			--Curr_File_Index;
			--Highlight_Y_Offset;
		}
		else
		{
			// Переход на предыдущую колонку
			if (Highlight_X_Offset > 0)
			{
				Highlight_X_Offset -= Width / 2;
				Highlight_Y_Offset = Height - 6; // Высота колонки минус 1
				Curr_File_Index = Files.size() - 1;
			}
		}
	}
	else {
		if (Curr_File_Index + 1 < Files.size())
		{
			++Curr_File_Index;
			++Highlight_Y_Offset;

			if (Highlight_Y_Offset >= Height - 5)
			{
				// Переход на следующую колонку
				if (Highlight_X_Offset + Width / 2 < Width)
				{
					Highlight_X_Offset += Width / 2;
					Highlight_Y_Offset = 0;
				}
			}
		}
	}
}


void APanel::On_Enter()
{
	AFile_Descriptor* file_descriptor = Files[Curr_File_Index];

	if (file_descriptor->Attributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		if (file_descriptor->File_Name == L"..")
		{
			// возврат в предыдущий каталог
			size_t pos = Current_Directory.find_last_of(L"\\");
			if (pos != std::wstring::npos)
			{
				std::wstring new_curr_dir = Current_Directory.substr(0, pos);
				Get_Directory_Files(new_curr_dir);
			}
		}
		else
		{// вход в текущий каталог

			std::wstring new_curr_dir = Current_Directory + L"\\" + file_descriptor->File_Name;

			Get_Directory_Files(new_curr_dir);
		}
	}
}

void APanel::Draw_Panels()
{
	ASymbol symbol(L' ', 0x1b, L' ', L' ');
	SArea_Pos area_pos(X_Pos + 1, Y_Pos + 1, Screen_Width, Width - 2, Height - 2);
	Clear_Area(Screen_Buffer, area_pos, symbol);

	//Horisontal
	{
		//up
		ASymbol symbol(L'═', 0x1b, L'╔', L'╗');
		SPos pos(X_Pos, Y_Pos, Screen_Width, Width - 2);
		Draw_Line_Horizontal(Screen_Buffer, pos, symbol);
	}
	{
		//down
		ASymbol symbol(L'═', 0x1b, L'╚', L'╝');
		SPos pos(X_Pos, Y_Pos + Height - 1, Screen_Width, Width - 2);
		Draw_Line_Horizontal(Screen_Buffer, pos, symbol);
	}

	//Vertical
	{
		//left
		ASymbol symbol(L'║', 0x1b, L'║', L'║');
		SPos pos(X_Pos, Y_Pos + 1, Screen_Width, Height - 4);
		Draw_Line_Vertical(Screen_Buffer, pos, symbol);
	}
	{
		//right
		ASymbol symbol(L'║', 0x1b, L'║', L'║');
		SPos pos(X_Pos + Width - 1, Y_Pos + 1, Screen_Width, Height - 4);
		Draw_Line_Vertical(Screen_Buffer, pos, symbol);
	}
	{
		//middle_hor
		ASymbol symbol(L'─', 0x1b, L'╟', L'╢');
		SPos pos(X_Pos, Y_Pos + Height - 3, Screen_Width, Width - 2);
		Draw_Line_Horizontal(Screen_Buffer, pos, symbol);
	}
	{
		//center_vert
		ASymbol symbol(L'║', 0x1b, L'╦', L'╨');
		SPos pos(X_Pos + Width / 2, Y_Pos, Screen_Width, Height - 4);
		Draw_Line_Vertical(Screen_Buffer, pos, symbol);
	}

}
//Show_Colors(screen_buffer, pos, symbol);
void APanel::Draw_Files()
{
	int x_offset = 0;
	int y_offset = 0;

	for (auto* file : Files)
	{
		Draw_One_File(file, x_offset, y_offset, 0x10);
		++y_offset;

		if (y_offset >= Height - 5)
		{// конец колонки

			if (x_offset == 0)
			{
				x_offset += Width / 2;
				y_offset = 0;
			}
			else
				break;  // вывод 2х колонок
		}
	}
}
void APanel::Draw_One_File(AFile_Descriptor* file_descriptor, int x_offset, int y_offset, unsigned short bg_attribute) {
	unsigned short attributes;

	if (file_descriptor->Attributes & FILE_ATTRIBUTE_DIRECTORY)
		attributes = bg_attribute | 0x0f;
	else
		attributes = bg_attribute | 0x0b;

	SText_Pos pos(X_Pos + x_offset + 1, Y_Pos + y_offset + 2, Screen_Width, attributes);
	Draw_Limited_Text(Screen_Buffer, pos, file_descriptor->File_Name.c_str(), Width / 2 - 1);

}
void APanel::Draw_Highlight()
{
	AFile_Descriptor* file_descriptor;
	if (Curr_File_Index >= Files.size())
		return;
	file_descriptor = Files[Curr_File_Index];
	Draw_One_File(file_descriptor, Highlight_X_Offset, Highlight_Y_Offset, 0x0D);
}