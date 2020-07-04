#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include "menu.h"
#include "saves.h"
#include "common.h"
#include "audioplayer.h"

#define show_message(msg)	show_dialog(0, msg)


const menu_option_t menu_options[] = {
	{ .name = "Background Music", 
		.options = NULL, 
		.type = APP_OPTION_BOOL, 
		.value = &apollo_config.music, 
		.callback = music_callback 
	},
	{ .name = "Sort Saves", 
		.options = NULL, 
		.type = APP_OPTION_BOOL, 
		.value = &apollo_config.doSort, 
		.callback = sort_callback 
	},
	{ .name = "Menu Animations", 
		.options = NULL, 
		.type = APP_OPTION_BOOL, 
		.value = &apollo_config.doAni, 
		.callback = ani_callback 
	},
	{ .name = "Screen Horizontal Margin", 
		.options = NULL, 
		.type = APP_OPTION_INC, 
		.value = &apollo_config.marginH, 
		.callback = horm_callback 
	},
	{ .name = "Screen Vertical Margin", 
		.options = NULL, 
		.type = APP_OPTION_INC, 
		.value = &apollo_config.marginV, 
		.callback = verm_callback 
	},
	{ .name = "Version Update Check", 
		.options = NULL, 
		.type = APP_OPTION_BOOL, 
		.value = &apollo_config.update, 
		.callback = update_callback 
	},
	{ .name = "Clear Local Cache", 
		.options = NULL, 
		.type = APP_OPTION_CALL, 
		.value = NULL, 
		.callback = clearcache_callback 
	},
	{ .name = "Update Application Data", 
		.options = NULL, 
		.type = APP_OPTION_CALL, 
		.value = NULL, 
		.callback = upd_appdata_callback 
	},
	{ .name = NULL }
};


void music_callback(int sel)
{
	apollo_config.music = !sel;
	SND_PauseVoice(2, sel);
}

void sort_callback(int sel)
{
	apollo_config.doSort = !sel;
}

void ani_callback(int sel)
{
	apollo_config.doAni = !sel;
}

void horm_callback(int sel)
{
	if (sel == 255)
		sel = 0;
	if (sel > 100)
		sel = 100;
	apollo_config.marginH = sel;
}

void verm_callback(int sel)
{
	if (sel == 255)
		sel = 0;
	if (sel > 100)
		sel = 100;
	apollo_config.marginV = sel;
}

void clearcache_callback(int sel)
{
	DIR *d;
	struct dirent *dir;
	char dataPath[256];

	d = opendir(APOLLO_LOCAL_CACHE);
	if (!d)
		return;

	LOG("Cleaning folder '%s'...", APOLLO_LOCAL_CACHE);

	while ((dir = readdir(d)) != NULL)
	{
		if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0)
		{
			snprintf(dataPath, sizeof(dataPath), "%s" "%s", APOLLO_LOCAL_CACHE, dir->d_name);
			LOG("Removing %s", dataPath);
			unlink_secure(dataPath);
		}
	}
	closedir(d);

	show_message("Local cache folder cleaned");
}

void unzip_app_data(const char* zip_file)
{
	if (extract_zip(zip_file, APOLLO_DATA_PATH))
		show_message("Successfully installed local application data");

	unlink_secure(zip_file);
}

void upd_appdata_callback(int sel)
{
	if (http_download(ONLINE_URL, "appdata.zip", APOLLO_LOCAL_CACHE "appdata.zip", 1))
		unzip_app_data(APOLLO_LOCAL_CACHE "appdata.zip");
}

void update_callback(int sel)
{
    apollo_config.update = !sel;

    if (!apollo_config.update)
        return;

	LOG("checking latest Apollo version at %s", APOLLO_UPDATE_URL);

	if (!http_download(APOLLO_UPDATE_URL, "", APOLLO_LOCAL_CACHE "ver.check", 0))
	{
		LOG("http request to %s failed", APOLLO_UPDATE_URL);
		return;
	}

	char *buffer;
	long size = 0;

	buffer = readFile(APOLLO_LOCAL_CACHE "ver.check", &size);

	if (!buffer)
		return;

	LOG("received %u bytes", size);
	buffer[size-1] = 0;

	static const char find[] = "\"name\":\"Apollo Save Tool v";
	const char* start = strstr(buffer, find);
	if (!start)
	{
		LOG("no name found");
		return;
	}

	LOG("found name");
	start += sizeof(find) - 1;

	char* end = strstr(start, "\"");
	if (!end)
	{
		LOG("no end of name found");
		return;
	}
	*end = 0;
	LOG("latest version is %s", start);

	if (stricmp(APOLLO_VERSION, start) == 0)
	{
		return;
	}

	start = strstr(end+1, "\"browser_download_url\":\"");
	if (!start)
		return;

	start += 24;
	end = strstr(start, "\"");
	if (!end)
	{
		LOG("no download URL found");
		return;
	}

	*end = 0;
	LOG("download URL is %s", start);

	if (show_dialog(1, "New version available! Download update?"))
	{
		if (http_download(start, "", "/dev_hdd0/packages/apollo-ps3.pkg", 1))
			show_message("Update downloaded to /dev_hdd0/packages/");
		else
			show_message("Download error!");
	}

}