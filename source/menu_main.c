#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <io/pad.h>

#include "sfo.h"
#include "saves.h"
#include "common.h"
#include "menu.h"
#include "menu_gui.h"
#include "ttf_render.h"

#include <tiny3d.h>
#include <libfont.h>

extern save_list_t hdd_saves;
extern save_list_t usb_saves;
extern save_list_t trophies;
extern save_list_t online_saves;
extern save_list_t user_backup;

extern int close_app;
extern padData paddata[];

int menu_options_maxopt = 0;
int * menu_options_maxsel;

int menu_id = 0;												// Menu currently in
int menu_sel = 0;												// Index of selected item (use varies per menu)
int menu_old_sel[TOTAL_MENU_IDS] = { 0 };						// Previous menu_sel for each menu
int last_menu_id[TOTAL_MENU_IDS] = { 0 };						// Last menu id called (for returning)

save_entry_t* selected_entry;
code_entry_t* selected_centry;
int option_index = 0;

void initMenuOptions()
{
	menu_options_maxopt = 0;
	while (menu_options[menu_options_maxopt].name)
		menu_options_maxopt++;

	menu_options_maxsel = (int *)calloc(menu_options_maxopt, sizeof(int));

	for (int i = 0; i < menu_options_maxopt; i++)
	{
		if (menu_options[i].type == APP_OPTION_LIST)
		{
			while (menu_options[i].options[menu_options_maxsel[i]])
				menu_options_maxsel[i]++;
		}
	}

	// default account owner
	*menu_options[8].value = menu_options_maxsel[8] - 1;
}

static void LoadFileTexture(const char* fname, int idx)
{
	if (!menu_textures[idx].buffer)
		menu_textures[idx].buffer = free_mem;

	pngLoadFromFile(fname, &menu_textures[idx].texture);
	copyTexture(idx);

	menu_textures[idx].size = 1;
	free_mem = (u32*) menu_textures[idx].buffer;
}

static int ReloadUserSaves(save_list_t* save_list)
{
	init_loading_screen("Loading save games...");

	if (save_list->list)
	{
		UnloadGameList(save_list->list);
		save_list->list = NULL;
	}

	if (save_list->UpdatePath)
		save_list->UpdatePath(save_list->path);

	save_list->list = save_list->ReadList(save_list->path);
	if (apollo_config.doSort == SORT_BY_NAME)
		list_bubbleSort(save_list->list, &sortSaveList_Compare);
	else if (apollo_config.doSort == SORT_BY_TITLE_ID)
		list_bubbleSort(save_list->list, &sortSaveList_Compare_TitleID);

	stop_loading_screen();

	if (!save_list->list)
	{
		show_message("No save-games found");
		return 0;
	}

	return list_count(save_list->list);
}

static code_entry_t* LoadRawPatch()
{
	char patchPath[256];
	code_entry_t* centry = calloc(1, sizeof(code_entry_t));

	centry->name = strdup(selected_entry->title_id);
	snprintf(patchPath, sizeof(patchPath), APOLLO_DATA_PATH "%s.ps3savepatch", selected_entry->title_id);
	centry->codes = readTextFile(patchPath, NULL);

	return centry;
}

static code_entry_t* LoadSaveDetails()
{
	char sfoPath[256];
	code_entry_t* centry = calloc(1, sizeof(code_entry_t));

	centry->name = strdup(selected_entry->title_id);

	if (!(selected_entry->flags & SAVE_FLAG_PS3))
	{
		asprintf(&centry->codes, "%s\n\nTitle: %s\n", selected_entry->path, selected_entry->name);
		return(centry);
	}

	snprintf(sfoPath, sizeof(sfoPath), "%s" "PARAM.SFO", selected_entry->path);
	LOG("Save Details :: Reading %s...", sfoPath);

	sfo_context_t* sfo = sfo_alloc();
	if (sfo_read(sfo, sfoPath) < 0) {
		LOG("Unable to read from '%s'", sfoPath);
		sfo_free(sfo);
		return centry;
	}

	if (selected_entry->flags & SAVE_FLAG_TROPHY)
	{
		char* account = (char*) sfo_get_param_value(sfo, "ACCOUNTID");
		asprintf(&centry->codes, "%s\n\n"
			"Title: %s\n"
			"NP Comm ID: %s\n"
			"Account ID: %.16s\n", selected_entry->path, selected_entry->name, selected_entry->title_id, account);
		LOG(centry->codes);

		sfo_free(sfo);
		return(centry);
	}

	char* subtitle = (char*) sfo_get_param_value(sfo, "SUB_TITLE");
	sfo_params_ids_t* param_ids = (sfo_params_ids_t*)(sfo_get_param_value(sfo, "PARAMS") + 0x1C);
	param_ids->user_id = ES32(param_ids->user_id);

	asprintf(&centry->codes, "%s\n\n"
		"Title: %s\n"
		"Sub-Title: %s\n"
		"Lock: %s\n\n"
		"User ID: %08d\n"
		"Account ID: %.16s (%s)\n"
		"PSID: %016lX %016lX\n", selected_entry->path, selected_entry->name, subtitle, 
		(selected_entry->flags & SAVE_FLAG_LOCKED ? "Copying Prohibited" : "Unlocked"),
		param_ids->user_id, param_ids->account_id, 
		(selected_entry->flags & SAVE_FLAG_OWNER ? "Owner" : "Not Owner"),
		param_ids->psid[0], param_ids->psid[1]);
	LOG(centry->codes);

	sfo_free(sfo);
	return (centry);
}

static void SetMenu(int id)
{   
	switch (menu_id) //Leaving menu
	{
		case MENU_MAIN_SCREEN: //Main Menu
		case MENU_TROPHIES:
		case MENU_USB_SAVES: //USB Saves Menu
		case MENU_HDD_SAVES: //HHD Saves Menu
		case MENU_ONLINE_DB: //Cheats Online Menu
		case MENU_USER_BACKUP: //Backup Menu
			menu_textures[icon_png_file_index].size = 0;
			break;

		case MENU_SETTINGS: //Options Menu
		case MENU_CREDITS: //About Menu
		case MENU_PATCHES: //Cheat Selection Menu
			break;

		case MENU_SAVE_DETAILS:
		case MENU_PATCH_VIEW: //Cheat View Menu
			if (apollo_config.doAni)
				Draw_CheatsMenu_View_Ani_Exit();
			break;

		case MENU_CODE_OPTIONS: //Cheat Option Menu
			if (apollo_config.doAni)
				Draw_CheatsMenu_Options_Ani_Exit();
			break;
	}
	
	switch (id) //going to menu
	{
		case MENU_MAIN_SCREEN: //Main Menu
			if (apollo_config.doAni || menu_id == MENU_MAIN_SCREEN) //if load animation
				Draw_MainMenu_Ani();
			break;

		case MENU_TROPHIES: //Trophies Menu
			if (!trophies.list && !ReloadUserSaves(&trophies))
				return;

			if (apollo_config.doAni)
				Draw_UserCheatsMenu_Ani(&trophies);
			break;

		case MENU_USB_SAVES: //USB saves Menu
			if (!usb_saves.list && !ReloadUserSaves(&usb_saves))
				return;
			
			if (apollo_config.doAni)
				Draw_UserCheatsMenu_Ani(&usb_saves);
			break;

		case MENU_HDD_SAVES: //HDD saves Menu
			if (!hdd_saves.list)
				ReloadUserSaves(&hdd_saves);
			
			if (apollo_config.doAni)
				Draw_UserCheatsMenu_Ani(&hdd_saves);
			break;

		case MENU_ONLINE_DB: //Cheats Online Menu
			if (!online_saves.list && !ReloadUserSaves(&online_saves))
				return;

			if (apollo_config.doAni)
				Draw_UserCheatsMenu_Ani(&online_saves);
			break;

		case MENU_CREDITS: //About Menu
			if (apollo_config.doAni)
				Draw_AboutMenu_Ani();
			break;

		case MENU_SETTINGS: //Options Menu
			if (apollo_config.doAni)
				Draw_OptionsMenu_Ani();
			break;

		case MENU_USER_BACKUP: //User Backup Menu
			if (!user_backup.list)
				ReloadUserSaves(&user_backup);

			if (apollo_config.doAni)
				Draw_UserCheatsMenu_Ani(&user_backup);
			break;

		case MENU_PATCHES: //Cheat Selection Menu
			//if entering from game list, don't keep index, otherwise keep
			if (menu_id == MENU_USB_SAVES || menu_id == MENU_HDD_SAVES || menu_id == MENU_ONLINE_DB || menu_id == MENU_TROPHIES)
				menu_old_sel[MENU_PATCHES] = 0;

			char iconfile[256];
			snprintf(iconfile, sizeof(iconfile), "%s" "ICON0.PNG", selected_entry->path);

			if (selected_entry->flags & SAVE_FLAG_ONLINE)
			{
				snprintf(iconfile, sizeof(iconfile), APOLLO_TMP_PATH "%s.PNG", selected_entry->title_id);

				if (file_exists(iconfile) != SUCCESS)
					http_download(selected_entry->path, "ICON0.PNG", iconfile, 0);
			}

			if (file_exists(iconfile) == SUCCESS)
				LoadFileTexture(iconfile, icon_png_file_index);
			else
				menu_textures[icon_png_file_index].size = 0;

			if (apollo_config.doAni && menu_id != MENU_PATCH_VIEW && menu_id != MENU_CODE_OPTIONS)
				Draw_CheatsMenu_Selection_Ani();
			break;

		case MENU_PATCH_VIEW: //Cheat View Menu
			menu_old_sel[MENU_PATCH_VIEW] = 0;
			if (apollo_config.doAni)
				Draw_CheatsMenu_View_Ani("Patch view");
			break;

		case MENU_SAVE_DETAILS: //Save Detail View Menu
			if (apollo_config.doAni)
				Draw_CheatsMenu_View_Ani(selected_entry->name);
			break;

		case MENU_CODE_OPTIONS: //Cheat Option Menu
			menu_old_sel[MENU_CODE_OPTIONS] = 0;
			if (apollo_config.doAni)
				Draw_CheatsMenu_Options_Ani();
			break;
	}
	
	menu_old_sel[menu_id] = menu_sel;
	if (last_menu_id[menu_id] != id)
		last_menu_id[id] = menu_id;
	menu_id = id;
	
	menu_sel = menu_old_sel[menu_id];
}

static void move_selection_back(int game_count, int steps)
{
	menu_sel -= steps;
	if ((menu_sel == -1) && (steps == 1))
		menu_sel = game_count - 1;
	else if (menu_sel < 0)
		menu_sel = 0;
}

static void move_selection_fwd(int game_count, int steps)
{
	menu_sel += steps;
	if ((menu_sel == game_count) && (steps == 1))
		menu_sel = 0;
	else if (menu_sel >= game_count)
		menu_sel = game_count - 1;
}

static int updatePadSelection(int total)
{
	if(paddata[0].BTN_UP)
		move_selection_back(total, 1);

	else if(paddata[0].BTN_DOWN)
		move_selection_fwd(total, 1);

	else if (paddata[0].BTN_LEFT)
		move_selection_back(total, 5);

	else if (paddata[0].BTN_L1)
		move_selection_back(total, 25);

	else if (paddata[0].BTN_L2)
		menu_sel = 0;

	else if (paddata[0].BTN_RIGHT)
		move_selection_fwd(total, 5);

	else if (paddata[0].BTN_R1)
		move_selection_fwd(total, 25);

	else if (paddata[0].BTN_R2)
		menu_sel = total - 1;

	else return 0;

	return 1;
}

static void doSaveMenu(save_list_t * save_list)
{
	if(readPad(0))
	{
		if (updatePadSelection(list_count(save_list->list)))
			(void)0;
	
		else if (paddata[0].BTN_CIRCLE)
		{
			SetMenu(MENU_MAIN_SCREEN);
			return;
		}
		else if (paddata[0].BTN_CROSS)
		{
			selected_entry = list_get_item(save_list->list, menu_sel);

			if (!selected_entry->codes && !save_list->ReadCodes(selected_entry))
			{
				show_message("No data found in folder:\n%s", selected_entry->path);
				return;
			}

			if (apollo_config.doSort && 
				((save_list->icon_id == cat_bup_png_index) || (save_list->icon_id == cat_db_png_index)))
				list_bubbleSort(selected_entry->codes, &sortCodeList_Compare);

			SetMenu(MENU_PATCHES);
			return;
		}
		else if (paddata[0].BTN_TRIANGLE && save_list->UpdatePath)
		{
			selected_entry = list_get_item(save_list->list, menu_sel);
			if (selected_entry->type != FILE_TYPE_MENU)
			{
				selected_centry = LoadSaveDetails();
				SetMenu(MENU_SAVE_DETAILS);
				return;
			}
		}
		else if (paddata[0].BTN_SELECT)
		{
			selected_entry = list_get_item(save_list->list, menu_sel);
			if ((save_list->icon_id == cat_hdd_png_index || save_list->icon_id == cat_usb_png_index) &&
				selected_entry->type != FILE_TYPE_MENU && (selected_entry->flags & SAVE_FLAG_PS3))
				selected_entry->flags ^= SAVE_FLAG_SELECTED;
		}
		else if (paddata[0].BTN_SQUARE)
		{
			ReloadUserSaves(save_list);
		}
	}

	Draw_UserCheatsMenu(save_list, menu_sel, 0xFF);
}

static void doMainMenu()
{
	// Check the pads.
	if (readPad(0))
	{
		if(paddata[0].BTN_LEFT)
			move_selection_back(MENU_CREDITS, 1);

		else if(paddata[0].BTN_RIGHT)
			move_selection_fwd(MENU_CREDITS, 1);

		else if (paddata[0].BTN_CROSS)
			SetMenu(menu_sel+1);

		else if(paddata[0].BTN_CIRCLE && show_dialog(1, "Exit to XMB?"))
			close_app = 1;
	}
	
	Draw_MainMenu();
}

void doAboutMenu()
{
	// Check the pads.
	if (readPad(0))
	{
		if (paddata[0].BTN_CIRCLE)
		{
			SetMenu(MENU_MAIN_SCREEN);
			return;
		}
	}

	Draw_AboutMenu();
}

static void doOptionsMenu()
{
	// Check the pads.
	if (readPad(0))
	{
		if(paddata[0].BTN_UP)
			move_selection_back(menu_options_maxopt, 1);

		else if(paddata[0].BTN_DOWN)
			move_selection_fwd(menu_options_maxopt, 1);

		else if (paddata[0].BTN_CIRCLE)
		{
			save_app_settings(&apollo_config);
			set_ttf_window(0, 0, 848 + apollo_config.marginH, 512 + apollo_config.marginV, WIN_SKIP_LF);
			SetMenu(MENU_MAIN_SCREEN);
			return;
		}
		else if (paddata[0].BTN_LEFT)
		{
			if (menu_options[menu_sel].type == APP_OPTION_LIST)
			{
				if (*menu_options[menu_sel].value > 0)
					(*menu_options[menu_sel].value)--;
				else
					*menu_options[menu_sel].value = menu_options_maxsel[menu_sel] - 1;
			}
			else if (menu_options[menu_sel].type == APP_OPTION_INC)
				(*menu_options[menu_sel].value)--;
			
			if (menu_options[menu_sel].type != APP_OPTION_CALL)
				menu_options[menu_sel].callback(*menu_options[menu_sel].value);
		}
		else if (paddata[0].BTN_RIGHT)
		{
			if (menu_options[menu_sel].type == APP_OPTION_LIST)
			{
				if (*menu_options[menu_sel].value < (menu_options_maxsel[menu_sel] - 1))
					*menu_options[menu_sel].value += 1;
				else
					*menu_options[menu_sel].value = 0;
			}
			else if (menu_options[menu_sel].type == APP_OPTION_INC)
				*menu_options[menu_sel].value += 1;

			if (menu_options[menu_sel].type != APP_OPTION_CALL)
				menu_options[menu_sel].callback(*menu_options[menu_sel].value);
		}
		else if (paddata[0].BTN_CROSS)
		{
			if (menu_options[menu_sel].type == APP_OPTION_BOOL)
				menu_options[menu_sel].callback(*menu_options[menu_sel].value);

			else if (menu_options[menu_sel].type == APP_OPTION_CALL)
				menu_options[menu_sel].callback(0);
		}
	}
	
	Draw_OptionsMenu();
}

static int count_code_lines()
{
	//Calc max
	int max = 0;
	const char * str;

	for(str = selected_centry->codes; *str; ++str)
		max += (*str == '\n');

	if (max <= 0)
		max = 1;

	return max;
}

static void doPatchViewMenu()
{
	// Check the pads.
	if (readPad(0))
	{
		if (updatePadSelection(count_code_lines()))
			(void)0;

		else if (paddata[0].BTN_CIRCLE)
		{
			SetMenu(last_menu_id[MENU_PATCH_VIEW]);
			return;
		}
	}
	
	Draw_CheatsMenu_View("Patch view");
}

static void doCodeOptionsMenu()
{
	code_entry_t* code = list_get_item(selected_entry->codes, menu_old_sel[last_menu_id[MENU_CODE_OPTIONS]]);
	// Check the pads.
	if (readPad(0))
	{
		if(paddata[0].BTN_UP)
			move_selection_back(selected_centry->options[option_index].size, 1);

		else if(paddata[0].BTN_DOWN)
			move_selection_fwd(selected_centry->options[option_index].size, 1);

		else if (paddata[0].BTN_CIRCLE)
		{
			code->activated = 0;
			SetMenu(last_menu_id[MENU_CODE_OPTIONS]);
			return;
		}
		else if (paddata[0].BTN_CROSS)
		{
			code->options[option_index].sel = menu_sel;

			if (code->type == PATCH_COMMAND)
				execCodeCommand(code, code->options[option_index].value[menu_sel]);

			option_index++;
			
			if (option_index >= code->options_count)
			{
				SetMenu(last_menu_id[MENU_CODE_OPTIONS]);
				return;
			}
			else
				menu_sel = 0;
		}
	}
	
	Draw_CheatsMenu_Options();
}

static void doSaveDetailsMenu()
{
	// Check the pads.
	if (readPad(0))
	{
		if (updatePadSelection(count_code_lines()))
			(void)0;

		if (paddata[0].BTN_CIRCLE)
		{
			if (selected_centry->name)
				free(selected_centry->name);
			if (selected_centry->codes)
				free(selected_centry->codes);
			free(selected_centry);

			SetMenu(last_menu_id[MENU_SAVE_DETAILS]);
			return;
		}
	}
	
	Draw_CheatsMenu_View(selected_entry->name);
}

static void doPatchMenu()
{
	// Check the pads.
	if (readPad(0))
	{
		if (updatePadSelection(list_count(selected_entry->codes)))
			(void)0;

		else if (paddata[0].BTN_CIRCLE)
		{
			SetMenu(last_menu_id[MENU_PATCHES]);
			return;
		}
		else if (paddata[0].BTN_CROSS)
		{
			selected_centry = list_get_item(selected_entry->codes, menu_sel);

			if (selected_centry->type != PATCH_NULL)
				selected_centry->activated = !selected_centry->activated;

			if (selected_centry->type == PATCH_COMMAND)
				execCodeCommand(selected_centry, selected_centry->codes);

			if (selected_centry->activated)
			{
				// Only activate Required codes if a cheat is selected
				if (selected_centry->type == PATCH_GAMEGENIE || selected_centry->type == PATCH_BSD)
				{
					code_entry_t* code;
					list_node_t* node;

					for (node = list_head(selected_entry->codes); (code = list_get(node)); node = list_next(node))
						if (wildcard_match_icase(code->name, "*(REQUIRED)*"))
							code->activated = 1;
				}
				/*
				if (!selected_centry->options)
				{
					int size;
					selected_entry->codes[menu_sel].options = ReadOptions(selected_entry->codes[menu_sel], &size);
					selected_entry->codes[menu_sel].options_count = size;
				}
				*/
				
				if (selected_centry->options)
				{
					option_index = 0;
					SetMenu(MENU_CODE_OPTIONS);
					return;
				}

				if (selected_centry->codes[0] == CMD_VIEW_RAW_PATCH)
				{
					selected_centry->activated = 0;
					selected_centry = LoadRawPatch();
					SetMenu(MENU_SAVE_DETAILS);
					return;
				}

				if (selected_centry->codes[0] == CMD_VIEW_DETAILS)
				{
					selected_centry->activated = 0;
					selected_centry = LoadSaveDetails();
					SetMenu(MENU_SAVE_DETAILS);
					return;
				}
			}
		}
		else if (paddata[0].BTN_TRIANGLE)
		{
			selected_centry = list_get_item(selected_entry->codes, menu_sel);

			if (selected_centry->type == PATCH_GAMEGENIE || selected_centry->type == PATCH_BSD ||
				selected_centry->type == PATCH_TROP_LOCK || selected_centry->type == PATCH_TROP_UNLOCK)
			{
				SetMenu(MENU_PATCH_VIEW);
				return;
			}
		}
	}
	
	Draw_CheatsMenu_Selection(menu_sel, 0xFFFFFFFF);
}

// Resets new frame
void drawScene()
{
	switch (menu_id)
	{
		case MENU_MAIN_SCREEN:
			doMainMenu();
			break;

		case MENU_TROPHIES: //Trophies Menu
			doSaveMenu(&trophies);
			break;

		case MENU_USB_SAVES: //USB Saves Menu
			doSaveMenu(&usb_saves);
			break;

		case MENU_HDD_SAVES: //HDD Saves Menu
			doSaveMenu(&hdd_saves);
			break;

		case MENU_ONLINE_DB: //Online Cheats Menu
			doSaveMenu(&online_saves);
			break;

		case MENU_CREDITS: //About Menu
			doAboutMenu();
			break;

		case MENU_SETTINGS: //Options Menu
			doOptionsMenu();
			break;

		case MENU_USER_BACKUP: //User Backup Menu
			doSaveMenu(&user_backup);
			break;

		case MENU_PATCHES: //Cheats Selection Menu
			doPatchMenu();
			break;

		case MENU_PATCH_VIEW: //Cheat View Menu
			doPatchViewMenu();
			break;

		case MENU_CODE_OPTIONS: //Cheat Option Menu
			doCodeOptionsMenu();
			break;

		case MENU_SAVE_DETAILS: //Save Details Menu
			doSaveDetailsMenu();
			break;
	}
}