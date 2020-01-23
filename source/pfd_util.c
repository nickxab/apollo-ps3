#include "saves.h"
#include "backend.h"
#include "config.h"
#include "list.h"
#include "pfd.h"
#include "util.h"

#define LOG dbglogger_log

static backend_t *backend = NULL;

static char *database_path = NULL;
static char *brute_file_path = NULL;
static char *game = NULL;
static int partial_process = 0;
static list_t *file_names = NULL;
static u64 file_offset = 0;
static s64 advance_offset = 1;

u8 *authentication_id = NULL;
u8 *console_id = NULL;
u8 *user_id = NULL;
u8 *syscon_manager_key = NULL;
u8 *keygen_key = NULL;
u8 *trophy_param_sfo_key = NULL;
u8 *savegame_param_sfo_key = NULL;
u8 *tropsys_dat_key = NULL;
u8 *tropusr_dat_key = NULL;
u8 *troptrns_dat_key = NULL;
u8 *tropconf_sfm_key = NULL;
u8 *fallback_disc_hash_key = NULL;
u8 *disc_hash_key = NULL;
list_t *secure_file_ids = NULL;

static pfd_config_t config;

static void show_version(void) {
	LOG("pfdtool " PFDTOOL_VERSION " (c) 2012 by flatz");
	LOG("");
}

/*
static void show_usage(void) {
	show_version();

	LOG("USAGE: pfdtool [options] command");
	LOG("");
	LOG("COMMANDS       Parameters              Explanation");
	LOG(" -h, --help                            Print this help");
	LOG(" -l, --list    dir                     List entries");
	LOG(" -c, --check   dir                     Verify a database");
	LOG(" -u, --update  dir                     Update an existing database");
	LOG(" -e, --encrypt dir files...            Encrypt specified files and update a database");
	LOG(" -d, --decrypt dir files...            Decrypt specified files and update a database");
	LOG(" -b, --brute   dir elf offset files... Bruteforce a secure file IDs for specified files");
	LOG("");
	LOG("OPTIONS        Parameters              Explanation");
	LOG(" -g, --game    product-code            Use a specified game setting set");
	LOG(" -p, --partial                         Don't update/verify all hashes");
	LOG(" -a, --advance                         The offset to advance each time while bruteforcing");

//	exit(1);
}

static void parse_args(int argc, char *argv[]) {
	int option_index;
	int c;
	pfd_cmd cmd;

	static const char* short_options = "hl:c:u:e:d:b:g:a:p";
	static struct option long_options[] = {
		{ "help", ARG_NONE, ARG_NULL, 'h' },

		{ "list", ARG_REQ, ARG_NULL, 'l' },
		{ "check", ARG_REQ, ARG_NULL, 'c' },
		{ "update", ARG_REQ, ARG_NULL, 'u' },
		{ "encrypt", ARG_REQ, ARG_NULL, 'e' },
		{ "decrypt", ARG_REQ, ARG_NULL, 'd' },
		{ "brute", ARG_REQ, ARG_NULL, 'b' },

		{ "game", ARG_REQ, ARG_NULL, 'g' },
		{ "advance", ARG_NONE, ARG_NULL, 'a' },
		{ "partial", ARG_REQ, ARG_NULL, 'p' },

		{ ARG_NULL, ARG_NULL, ARG_NULL, ARG_NULL }
	};

	while ((c = option_index = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch (c) {
			case '?':
				show_usage();
				break;

			case 'h':
				show_usage();
				break;

			case 'l':
//				cmd_list = 1;
				database_path = optarg;
				return;
			case 'c':
//				cmd_check = 1;
				database_path = optarg;
				return;
			case 'u':
//				cmd_update = 1;
				database_path = optarg;
				return;
			case 'e':
//				cmd_encrypt = 1;
				database_path = optarg;
				goto get_args;
			case 'd':
//				cmd_decrypt = 1;
				database_path = optarg;
				goto get_args;
			case 'b':
//				cmd_brute = 1;
				database_path = optarg;
				goto get_args;

			case 'g':
				game = optarg;
				break;
			case 'p':
				partial_process = 1;
				break;
			case 'a':
				advance_offset = strtol(optarg, NULL, 10);
				break;

			default:
				abort();
		}
	}

get_args:;
	if (cmd == PFD_CMD_encrypt) {
		if (argc - optind < 1) {
			LOG("[*] Error: Encrypt command needs input files!");
			show_usage();
		} else {
			if (optind < argc) {
				while (optind < argc) {
					list_append(file_names, argv[optind++]);
				}
			} else {
				LOG("[*] Error: Encrypt command needs file names!");
				show_usage();
			}
		}
		return;
	}

	if (cmd == PFD_CMD_decrypt) {
		if (argc - optind < 1) {
			LOG("[*] Error: Decrypt command needs input files!");
			show_usage();
		} else {
			if (optind < argc) {
				while (optind < argc) {
					list_append(file_names, argv[optind++]);
				}
			} else {
				LOG("[*] Error: Decrypt command needs file names!");
				show_usage();
			}
		}
		return;
	}

	if (cmd == PFD_CMD_brute) {
		if (argc - optind < 3) {
			LOG("[*] Error: Brute command needs additional parameters!");
			show_usage();
		} else {
			brute_file_path = argv[optind++];
			if (sscanf(argv[optind++], "%llu", &file_offset) < 1)
				file_offset = 0;
			if (optind < argc) {
				while (optind < argc) {
					list_append(file_names, argv[optind++]);
				}
			} else {
				LOG("[*] Error: Brute command needs file names!");
				show_usage();
			}
		}
		return;
	}
}
*/


void pfd_util_end(void) {

	LOG("clean up...");

	if (authentication_id)
		free(authentication_id);
	if (console_id)
		free(console_id);
	if (user_id)
		free(user_id);
	if (syscon_manager_key)
		free(syscon_manager_key);
	if (keygen_key)
		free(keygen_key);
	if (trophy_param_sfo_key)
		free(trophy_param_sfo_key);
	if (savegame_param_sfo_key)
		free(savegame_param_sfo_key);
	if (tropsys_dat_key)
		free(tropsys_dat_key);
	if (tropusr_dat_key)
		free(tropusr_dat_key);
	if (troptrns_dat_key)
		free(troptrns_dat_key);
	if (tropconf_sfm_key)
		free(tropconf_sfm_key);
	if (fallback_disc_hash_key)
		free(fallback_disc_hash_key);
	if (disc_hash_key)
		free(disc_hash_key);

	LOG("clean up 2");

	if (file_names)
		list_free(file_names);

	LOG("clean up 3");

	if (secure_file_ids) {
		list_node_t *node;
		node = list_head(secure_file_ids);
		while (node) {
			if (node->value)
				free(node->value);
			node = node->next;
		}
		list_free(secure_file_ids);
	}

	LOG("clean up complete");
}

static int global_config_handler(void *user, const char *section, const char *name, const char *value) {
	if (strcmp(section, "global") == 0) {
		if (strcmp(name, "authentication_id") == 0) {
			if (strlen(value) != PFD_AUTHENTICATION_ID_SIZE * 2) {
				LOG("[*] Error: 'Authentication ID' needs to be 8 bytes.");
				return -1;
			}
			authentication_id = x_to_u8_buffer(value);
			return 0;
		} else if (strcmp(name, "console_id") == 0) {
			if (strlen(value) != PFD_CONSOLE_ID_SIZE * 2) {
				LOG("[*] Error: Console ID needs to be 16 bytes.");
				return -1;
			}
			console_id = x_to_u8_buffer(value);
			return 0;
		} else if (strcmp(name, "user_id") == 0) {
			if (strlen(value) != PFD_USER_ID_SIZE) {
				LOG("[*] Error: User ID needs to be 8 bytes.");
				return -1;
			}
			user_id = (u8 *)strdup(value);
			return 0;
		} else if (strcmp(name, "syscon_manager_key") == 0) {
			if (strlen(value) != PFD_SYSCON_MANAGER_KEY_SIZE * 2) {
				LOG("[*] Error: Syscon Manager Key needs to be 16 bytes.");
				return -1;
			}
			syscon_manager_key = x_to_u8_buffer(value);
			return 0;
		} else if (strcmp(name, "keygen_key") == 0) {
			if (strlen(value) != PFD_KEYGEN_KEY_SIZE * 2) {
				LOG("[*] Error: Keygen Key needs to be 20 bytes.");
				return -1;
			}
			keygen_key = x_to_u8_buffer(value);
			return 0;
		} else if (strcmp(name, "trophy_param_sfo_key") == 0) {
			if (strlen(value) != PFD_PARAM_SFO_KEY_SIZE * 2) {
				LOG("[*] Error: 'Trophy PARAM.SFO Key' needs to be 20 bytes.");
				return -1;
			}
			trophy_param_sfo_key = x_to_u8_buffer(value);
			return 0;
		} else if (strcmp(name, "savegame_param_sfo_key") == 0) {
			if (strlen(value) != PFD_PARAM_SFO_KEY_SIZE * 2) {
				LOG("[*] Error: 'Save Game PARAM.SFO Key' needs to be 20 bytes.");
				return -1;
			}
			savegame_param_sfo_key = x_to_u8_buffer(value);
			return 0;
		} else if (strcmp(name, "tropsys_dat_key") == 0) {
			if (strlen(value) != PFD_TROPSYS_DAT_KEY_SIZE * 2) {
				LOG("[*] Error: 'TROPSYS.DAT Key' needs to be 20 bytes.");
				return -1;
			}
			tropsys_dat_key = x_to_u8_buffer(value);
			return 0;
		} else if (strcmp(name, "tropusr_dat_key") == 0) {
			if (strlen(value) != PFD_TROPUSR_DAT_KEY_SIZE * 2) {
				LOG("[*] Error: 'TROPUSR.DAT Key' needs to be 20 bytes.");
				return -1;
			}
			tropusr_dat_key = x_to_u8_buffer(value);
			return 0;
		} else if (strcmp(name, "troptrns_dat_key") == 0) {
			if (strlen(value) != PFD_TROPTRNS_DAT_KEY_SIZE * 2) {
				LOG("[*] Error: 'TROPTRNS.DAT Key' needs to be 20 bytes.");
				return -1;
			}
			troptrns_dat_key = x_to_u8_buffer(value);
			return 0;
		} else if (strcmp(name, "tropconf_sfm_key") == 0) {
			if (strlen(value) != PFD_TROPCONF_SFM_KEY_SIZE * 2) {
				LOG("[*] Error: 'TROPCONF.SFM Key' needs to be 20 bytes.");
				return -1;
			}
			tropconf_sfm_key = x_to_u8_buffer(value);
			return 0;
		} else if (strcmp(name, "fallback_disc_hash_key") == 0) {
			if (strlen(value) != PFD_DISC_HASH_KEY_SIZE * 2) {
				LOG("[*] Error: 'Fallback Disc Hash Key' needs to be 16 bytes.");
				return -1;
			}
			fallback_disc_hash_key = x_to_u8_buffer(value);
			return 0;
		}
	}
	return 0;
}

static int games_config_handler(void *user, const char *section, const char *name, const char *value) {
	u8 *secure_file_id;
	char *tokens;
	char *token;
	if (!game)
		return 0;
	tokens = strdup(section);
	section = NULL;
	token = strtok(tokens, "/");
	while (token) {
		if (strcmp(token, game) == 0) {
			section = token;
			break;
		}
		token = strtok(NULL, "/");
	}
	if (section) {
		free(tokens);
		if (strcmp(name, "disc_hash_key") == 0) {
			if (strlen(value) != PFD_DISC_HASH_KEY_SIZE * 2) {
				LOG("[*] Error: Disc hash key needs to be 16 bytes.");
				return -1;
			}
			disc_hash_key = x_to_u8_buffer(value);
			return 0;
		} else if (strncmp(name, "secure_file_id:", 15) == 0) {
			secure_file_id_t *tmp;
			const char *file_name = name + 15;
			if (strlen(value) != PFD_SECURE_FILE_ID_SIZE * 2) {
				LOG("[*] Error: Secure file ID needs to be 16 bytes.");
				return -1;
			}
			secure_file_id = x_to_u8_buffer(value);
			if (file_name && secure_file_id) {
				tmp = (secure_file_id_t *)malloc(sizeof(secure_file_id_t));
				memset(tmp, 0, sizeof(secure_file_id_t));
				strncpy(tmp->file_name, file_name, PFD_ENTRY_NAME_SIZE);
				memcpy(tmp->secure_file_id, secure_file_id, PFD_SECURE_FILE_ID_SIZE);
				list_append(secure_file_ids, tmp);
			}
			if (secure_file_id)
				free(secure_file_id);
			return 0;
		}
	} else {
		/* skipping unneeded title id */
		return 0;
	}
	return 0;
}

int pfd_util_init(char* game_id, char* db_path, int partial) {
	int result = 0;

	file_names = list_alloc();
	secure_file_ids = list_alloc();

//	parse_args(argc, argv);
	show_version();

	game = game_id;
	database_path = db_path;
	partial_process = partial;

	if ((result = parse_config_file(APOLLO_PATH PFDTOOL_CONFIG_GLOBAL, &global_config_handler, NULL)) != 0) {
		if (result < 0)
			LOG("[*] Error: Unable to read a global config file.");
		else
			LOG("[*] Error: Could not parse a global config file (error at line: %d).\n", result);
		return result;
	}

	if ((result = parse_config_file(APOLLO_DATA_PATH PFDTOOL_CONFIG_GAMES, &games_config_handler, NULL)) != 0) {
		if (result < 0)
			LOG("[*] Error: Unable to read a games config file.");
		else
			LOG("[*] Error: Could not parse a games config file (error at line: %d).\n", result);
		return result;
	}

	if (game) {
		if (!disc_hash_key && fallback_disc_hash_key) {
			disc_hash_key = (u8 *)malloc(PFD_DISC_HASH_KEY_SIZE);
			memcpy(disc_hash_key, fallback_disc_hash_key, PFD_DISC_HASH_KEY_SIZE);
			LOG("[*] Warning: A disc hash key was not found. A fallback disc hash key will be used.");
		}
	}

	memset(&config, 0, sizeof(config));
	if (syscon_manager_key)
		memcpy(config.syscon_manager_key, syscon_manager_key, PFD_SYSCON_MANAGER_KEY_SIZE);
	if (keygen_key)
		memcpy(config.keygen_key, keygen_key, PFD_KEYGEN_KEY_SIZE);
	if (console_id)
		memcpy(config.console_id, console_id, PFD_CONSOLE_ID_SIZE);
	if (console_id)
		memcpy(config.user_id, user_id, PFD_USER_ID_SIZE);
	if (savegame_param_sfo_key)
		memcpy(config.savegame_param_sfo_key, savegame_param_sfo_key, PFD_PARAM_SFO_KEY_SIZE);
	if (trophy_param_sfo_key)
		memcpy(config.trophy_param_sfo_key, trophy_param_sfo_key, PFD_PARAM_SFO_KEY_SIZE);
	if (tropsys_dat_key)
		memcpy(config.tropsys_dat_key, tropsys_dat_key, PFD_TROPSYS_DAT_KEY_SIZE);
	if (tropsys_dat_key)
		memcpy(config.tropusr_dat_key, tropusr_dat_key, PFD_TROPUSR_DAT_KEY_SIZE);
	if (troptrns_dat_key)
		memcpy(config.troptrns_dat_key, troptrns_dat_key, PFD_TROPTRNS_DAT_KEY_SIZE);
	if (tropconf_sfm_key)
		memcpy(config.tropconf_sfm_key, tropconf_sfm_key, PFD_TROPCONF_SFM_KEY_SIZE);
	if (fallback_disc_hash_key)
		memcpy(config.fallback_disc_hash_key, fallback_disc_hash_key, PFD_DISC_HASH_KEY_SIZE);
	if (disc_hash_key)
		memcpy(config.disc_hash_key, disc_hash_key, PFD_DISC_HASH_KEY_SIZE);
	if (authentication_id)
		memcpy(config.authentication_id, authentication_id, PFD_AUTHENTICATION_ID_SIZE);

//	backend = backend_initialize(&config, secure_file_ids, database_path);

	return result;
}

int pfd_util_process(pfd_cmd_t cmd) {
	int ret = 0;
	u32 flag = (partial_process ? BACKEND_VALIDATE_FLAG_PARTIAL : BACKEND_VALIDATE_FLAG_NONE);

	LOG("db %s (%d)", database_path, flag);
	LOG("game %s", game);

	backend = backend_initialize(&config, secure_file_ids, database_path);
	if (!backend) {
		return -1;
	}

	switch (cmd) {
		case PFD_CMD_LIST:
			ret = backend_cmd_list(backend);
			break;

		case PFD_CMD_CHECK:
			ret = backend_cmd_check(backend, (partial_process ? BACKEND_VALIDATE_FLAG_PARTIAL : BACKEND_VALIDATE_FLAG_NONE));
			break;

		case PFD_CMD_UPDATE:
			ret = backend_cmd_update(backend, (partial_process ? BACKEND_VALIDATE_FLAG_PARTIAL : BACKEND_VALIDATE_FLAG_NONE));
			break;

		case PFD_CMD_ENCRYPT:
			ret = backend_cmd_encrypt(backend, file_names);
			break;

		case PFD_CMD_DECRYPT:
			ret = backend_cmd_decrypt(backend, file_names);
			break;

		case PFD_CMD_BRUTE:
			ret = backend_cmd_brute(backend, brute_file_path, file_offset, advance_offset, file_names);
			break;
	}

	backend_shutdown(backend);

	return ret;
}

/*
int pfd_main(int argc, char *argv[], pfd_cmd cmd) {
	int result;

	if (argc <= 1)
		show_usage();

	atexit(&pfd_util_end);

	file_names = list_alloc();
	secure_file_ids = list_alloc();

	parse_args(argc, argv);
	show_version();

	if ((result = parse_config_file(APOLLO_PATH PFDTOOL_CONFIG_GLOBAL, &global_config_handler, NULL)) != 0) {
		if (result < 0)
			LOG("[*] Error: Unable to read a global config file.");
		else
			LOG("[*] Error: Could not parse a global config file (error at line: %d).\n", result);
		return 0;
	}

	if ((result = parse_config_file(APOLLO_PATH PFDTOOL_CONFIG_GAMES, &games_config_handler, NULL)) != 0) {
		if (result < 0)
			LOG("[*] Error: Unable to read a games config file.");
		else
			LOG("[*] Error: Could not parse a games config file (error at line: %d).\n", result);
	}

	if (cmd == PFD_CMD_list || cmd == PFD_CMD_check || cmd == PFD_CMD_update || cmd == PFD_CMD_encrypt || cmd == PFD_CMD_decrypt || cmd == PFD_CMD_brute) {
		if (game) {
			if (!disc_hash_key && fallback_disc_hash_key) {
				disc_hash_key = (u8 *)malloc(PFD_DISC_HASH_KEY_SIZE);
				memcpy(disc_hash_key, fallback_disc_hash_key, PFD_DISC_HASH_KEY_SIZE);
				LOG("[*] Warning: A disc hash key was not found. A fallback disc hash key will be used.");
			}
		}

		memset(&config, 0, sizeof(config));
		if (syscon_manager_key)
			memcpy(config.syscon_manager_key, syscon_manager_key, PFD_SYSCON_MANAGER_KEY_SIZE);
		if (keygen_key)
			memcpy(config.keygen_key, keygen_key, PFD_KEYGEN_KEY_SIZE);
		if (console_id)
			memcpy(config.console_id, console_id, PFD_CONSOLE_ID_SIZE);
		if (console_id)
			memcpy(config.user_id, user_id, PFD_USER_ID_SIZE);
		if (savegame_param_sfo_key)
			memcpy(config.savegame_param_sfo_key, savegame_param_sfo_key, PFD_PARAM_SFO_KEY_SIZE);
		if (trophy_param_sfo_key)
			memcpy(config.trophy_param_sfo_key, trophy_param_sfo_key, PFD_PARAM_SFO_KEY_SIZE);
		if (tropsys_dat_key)
			memcpy(config.tropsys_dat_key, tropsys_dat_key, PFD_TROPSYS_DAT_KEY_SIZE);
		if (tropsys_dat_key)
			memcpy(config.tropusr_dat_key, tropusr_dat_key, PFD_TROPUSR_DAT_KEY_SIZE);
		if (troptrns_dat_key)
			memcpy(config.troptrns_dat_key, troptrns_dat_key, PFD_TROPTRNS_DAT_KEY_SIZE);
		if (tropconf_sfm_key)
			memcpy(config.tropconf_sfm_key, tropconf_sfm_key, PFD_TROPCONF_SFM_KEY_SIZE);
		if (fallback_disc_hash_key)
			memcpy(config.fallback_disc_hash_key, fallback_disc_hash_key, PFD_DISC_HASH_KEY_SIZE);
		if (disc_hash_key)
			memcpy(config.disc_hash_key, disc_hash_key, PFD_DISC_HASH_KEY_SIZE);
		if (authentication_id)
			memcpy(config.authentication_id, authentication_id, PFD_AUTHENTICATION_ID_SIZE);

		backend = backend_initialize(&config, secure_file_ids, database_path);
		if (backend) {
			if (cmd == PFD_CMD_list) {
				backend_cmd_list(backend);
			} else if (cmd == PFD_CMD_check) {
				if (partial_process)
					backend_cmd_check(backend, BACKEND_VALIDATE_FLAG_PARTIAL);
				else
					backend_cmd_check(backend, BACKEND_VALIDATE_FLAG_NONE);
			} else if (cmd == PFD_CMD_update) {
				if (partial_process)
					backend_cmd_update(backend, BACKEND_UPDATE_FLAG_PARTIAL);
				else
					backend_cmd_update(backend, BACKEND_UPDATE_FLAG_NONE);
			} else if (cmd == PFD_CMD_encrypt) {
				backend_cmd_encrypt(backend, file_names);
			} else if (cmd == PFD_CMD_decrypt) {
				backend_cmd_decrypt(backend, file_names);
			} else if (cmd == PFD_CMD_brute) {
				backend_cmd_brute(backend, brute_file_path, file_offset, advance_offset, file_names);
			}
			backend_shutdown(backend);
		}
	} else {
		show_usage();
	}

	return 0;
}
*/
