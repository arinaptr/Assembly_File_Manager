#include "Commander.h"

AMenu_Item::AMenu_Item(unsigned short x_pos, unsigned short y_pos, unsigned short len, const wchar_t* key, const wchar_t* name)
	: X_Pos(x_pos), Y_Pos(y_pos), Len(len), Key(key), Name(name)
{
}

void AMenu_Item::Draw(CHAR_INFO* screen_buffer, unsigned short screen_width)
{
	int key_str_len;

	SText_Pos key_pos(X_Pos, Y_Pos, screen_width, 0x07);
	key_str_len = Draw_Text(screen_buffer, key_pos, Key);

	SText_Pos name_pos(X_Pos + key_str_len, Y_Pos, screen_width, 0xb0);
	Draw_Limited_Text(screen_buffer, name_pos, Name, Len);
}

AsCommander:: ~AsCommander()
{
	if (!SetConsoleActiveScreenBuffer(Std_Output_Handle))
		printf("SetConsoleActiveScreenBuffer failed - (%d)\n", GetLastError());
	delete Left_Panel;
	delete Right_Panel;
	delete Screen_Buffer;

}
bool AsCommander::Init()
{
	SMALL_RECT srctWriteRect;
	int screen_buffer_size;
	std::wstring root_dir = L"C:\\";

	Std_Input_Handle = GetStdHandle(STD_INPUT_HANDLE);
	Std_Output_Handle = GetStdHandle(STD_OUTPUT_HANDLE);

	Screen_Buffer_Handle = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, CONSOLE_TEXTMODE_BUFFER, 0);
	if (Std_Input_Handle == INVALID_HANDLE_VALUE || Std_Output_Handle == INVALID_HANDLE_VALUE || Screen_Buffer_Handle == INVALID_HANDLE_VALUE)
	{
		printf("CreateConsoleScreenBuffer failed - (%d)\n", GetLastError());
		return false;
	}

	// Make the new screen buffer the active screen buffer.

	if (!SetConsoleActiveScreenBuffer(Screen_Buffer_Handle))
	{
		printf("SetConsoleActiveScreenBuffer failed - (%d)\n", GetLastError());
		return false;
	}

	if (!GetConsoleScreenBufferInfo(Screen_Buffer_Handle, &Screen_Buffer_Info))
	{
		printf("GetConsoleScreenBufferInfo failed - (%d)\n", GetLastError());
		return false;
	}

	screen_buffer_size = (int)Screen_Buffer_Info.dwSize.X * (int)Screen_Buffer_Info.dwSize.Y;
	Screen_Buffer = new CHAR_INFO[screen_buffer_size];
	memset(Screen_Buffer, 0, screen_buffer_size * sizeof(CHAR_INFO));

	// Set the destination rectangle.

	srctWriteRect.Top = 10;    // top lt: row 10, col 0
	srctWriteRect.Left = 0;
	srctWriteRect.Bottom = 11; // bot. rt: row 11, col 79
	srctWriteRect.Right = 79;

	int half_width = Screen_Buffer_Info.dwSize.X / 2;
	Left_Panel = new APanel(0, 0, half_width, Screen_Buffer_Info.dwSize.Y - 2, Screen_Buffer, Screen_Buffer_Info.dwSize.X);
	Right_Panel = new APanel(half_width, 0, half_width, Screen_Buffer_Info.dwSize.Y - 2, Screen_Buffer, Screen_Buffer_Info.dwSize.X);
	Build_Menu();

	Left_Panel->Get_Directory_Files(root_dir);
	return true;
}


void AsCommander::Run()
{
	unsigned long records_count;
	INPUT_RECORD input_record[128];
	Can_Run = true;
	Need_Redraw = true;
	while (Can_Run)
	{
		if (PeekConsoleInput(Std_Input_Handle, input_record, 128, &records_count))
		{
			if (ReadConsoleInput(Std_Input_Handle, input_record, 1, &records_count))
			{
				if (records_count != 0)
				{
					if (input_record[0].EventType == KEY_EVENT && input_record[0].Event.KeyEvent.bKeyDown)
					{
						switch (input_record[0].Event.KeyEvent.wVirtualKeyCode)
						{
						case VK_F10:
							Can_Run = false;
							break;
						case VK_UP:
							Left_Panel->Move_Highlight(true);
							Need_Redraw = true;
							break;
						case VK_DOWN:
							Left_Panel->Move_Highlight(false);
							Need_Redraw = true;
							break;
						case VK_RETURN:
							Left_Panel->On_Enter();
							Need_Redraw = true;
							break;
						}
					}
				}

			}
		}
		if (Need_Redraw)
		{
			if (!Draw())
				return;

			Need_Redraw = false;
		}
		Sleep(10);
	}
}

bool AsCommander::Draw()
{
	COORD screen_buffer_pos{};

	Left_Panel->Draw();
	Right_Panel->Draw();

	for (int i = 0; i < 10; i++)
	{
		if (Menu_Items[i] != 0) {
			Menu_Items[i]->Draw(Screen_Buffer, Screen_Buffer_Info.dwSize.X);
		}
	}

	if (!WriteConsoleOutput(Screen_Buffer_Handle, Screen_Buffer, Screen_Buffer_Info.dwSize, screen_buffer_pos, &Screen_Buffer_Info.srWindow))
	{
		printf("WriteConsoleOutput failed - (%d)\n", GetLastError());
		return false;
	}
	return true;
}

void AsCommander::Add_Next_Menu_Item(int& index, int& x_pos, int x_step, const wchar_t* key, const wchar_t* name)
{
	Menu_Items[index++] = new AMenu_Item(x_pos, Screen_Buffer_Info.dwSize.Y - 1, 8, key, name);
	x_pos += x_step;
	if (index == 2)
	{
		--x_pos;
	}
}

void AsCommander::Build_Menu()
{
	int index = 0;
	int x_pos = 0;
	int x_step = Screen_Buffer_Info.dwSize.X / 10;

	Add_Next_Menu_Item(index, x_pos, x_step, L"1", L"Help");
	Add_Next_Menu_Item(index, x_pos, x_step, L"2", L"UserMenu");
	Add_Next_Menu_Item(index, x_pos, x_step, L"3", L"View");
	Add_Next_Menu_Item(index, x_pos, x_step, L"4", L"Edit");
	Add_Next_Menu_Item(index, x_pos, x_step, L"5", L"Copy");
	Add_Next_Menu_Item(index, x_pos, x_step, L"6", L"RenMov");
	Add_Next_Menu_Item(index, x_pos, x_step, L"7", L"MakeDir");
	Add_Next_Menu_Item(index, x_pos, x_step, L"8", L"Delete");
	Add_Next_Menu_Item(index, x_pos, x_step, L"9", L"Config");
	Add_Next_Menu_Item(index, x_pos, x_step, L"10", L"Quit");
	//Add_Next_Menu_Item(index, x_pos, x_step, L"11", L"Plugin");
	//Add_Next_Menu_Item(index, x_pos, x_step, L"12", L"Screen");
}



