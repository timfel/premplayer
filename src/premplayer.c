#include "PDL.h"
#include "SDL/SDL.h"
#include "json.h"

#include "dirent.h"
#include "signal.h"
#include "unistd.h"
#include "strings.h"
#include "string.h"

#define EMPTY_STRING(x) PDL_JSReply(x, ""); return PDL_TRUE;
#define RETVAL_TRUE(x) PDL_JSReply(x, "true"); return PDL_TRUE;
#define RETVAL_FALSE(x) PDL_JSReply(x, "false"); return PDL_TRUE;
#define MAX(x,y) (x > y ? x : y)

// The maximum number of parameters we will allow to be passed to mplayer
#define MAX_PARAMETERS 42
// The starting size for the directory entries vector
#define MIN_DIR_ENTRY_LENGTH 2048


pid_t mplayer_pid;
char mplayer_path[1024] = {'\0'}; // Be nasty - pre-allocate enough space for a path
char* entries = NULL;

PDL_bool prem_list(PDL_JSParameters *parms, unsigned char type) {
	int argc = PDL_GetNumJSParams(parms);
	if (argc != 1) {
		return EMPTY_STRING(parms);
	}
	const char* argv = PDL_GetJSParamString(parms, 0);
	if (!argv) {
		return EMPTY_STRING(parms);
	}

	DIR* dir = opendir(argv);
	if (dir) {
		int entry_p = 0; // The position we're at in the return buffer
		int entry_size = MIN_DIR_ENTRY_LENGTH; // The current size of the return buffer
		if (entries) {
			// There are to be entries from a previous run, free those
			free(entries);
		}
		entries = (char*)calloc(sizeof(char), entry_size + 1);
		if (!entries) return EMPTY_STRING(parms);

		struct dirent *ent;
		while (ent = readdir(dir)) {
			if (ent->d_type == type) {
				// The given entry matches the requested type
				int name_length = strlen(ent->d_name) + 1; // The names are separated by "/", so +1

				// Resize the buffer, if the new string wouldn't fit
				if (entry_p + name_length > entry_size) {
					entry_size += MAX(MIN_DIR_ENTRY_LENGTH, name_length);
					char* entries_ng = (char*)realloc(entries, sizeof(char) * entry_size);
					if (!entries_ng) break; // We cannot grow, return what we have
				}

				// Add a separator
				strcat(entries, "/");
				// Add the new entry to the buffer
				strcat(entries, ent->d_name);
				entry_p += name_length;
			}
		}
		closedir (dir);
		PDL_JSReply(parms, entries); // Return the "/" separated list of entries
		return PDL_TRUE;
	} else {
		return EMPTY_STRING(parms);
	}
}

// Return the list of file names under a path
PDL_bool prem_list_files(PDL_JSParameters *parms) {
	return prem_list(parms, DT_REG);
}

// Return the list of directory names under a path
PDL_bool prem_list_directories(PDL_JSParameters *parms) {
	return prem_list(parms, DT_DIR);
}

void prem_kill_bang() {
	(void)kill(mplayer_pid, SIGKILL);
	mplayer_pid = 0;
}

// Kill the running mplayer instance, if any
PDL_bool prem_kill(PDL_JSParameters *parms) {
	if (mplayer_pid != 0) {
		prem_kill_bang();
	}
	return RETVAL_TRUE(parms);
}

// Run mplayer with the passed files qualifier
PDL_bool prem_run(PDL_JSParameters *parms) {
	// Kill any running instance
	prem_kill_bang();

	// Get the arguments - should be a list file paths - wildcards allowed
	int argc = PDL_GetNumJSParams(parms);
	if (argc < 1) {
		return RETVAL_FALSE(parms);
	}
	if (argc > MAX_PARAMETERS) {
		argc = MAX_PARAMETERS;
	}

	char** argv = (char**)calloc(sizeof(char*), argc);
	if (!argv) {
		return RETVAL_FALSE(parms); // Unable to allocate argument list
	}

	for (int i = 0; i < argc; i++) {
		const char* argument = PDL_GetJSParamString(parms, i);
		argv[i] = (char*)calloc(sizeof(char), strlen(argument));
		if (!argv[i]) {
			break; // If we cannot allocate anymore, ignore the remaining args
		}
		strcpy(argv[i], argument);
	}

	mplayer_pid = fork();
	if (mplayer_pid == -1) {
		mplayer_pid = 0;
		return RETVAL_FALSE(parms);
	}

	if (mplayer_pid) {
		// parent
		for (int i = 0; i < argc; i++) {
			if (argv[i]) {
				free(argv[i]);
			}
			free(argv);
		}
		return RETVAL_TRUE(parms);
	} else {
		// child
		execv(mplayer_path, argv);
	}
}

void termination_handler(int signum) {
	prem_kill_bang();
}

int main(int argc, char** argv) {
	strcpy(mplayer_path, argv[0]);
	char *path_end = rindex(mplayer_path, '/');
	if (!path_end) return 1; // Error
	path_end[1] = '\0'; // cutoff after the last path delimiter
	strcat(mplayer_path, "mplayer");
	printf("Registered mplayer path: %s.\n", mplayer_path);

	int error = SDL_Init(SDL_INIT_VIDEO);

	PDL_RegisterJSHandler("list_files", prem_list_files);
	PDL_RegisterJSHandler("list_directories", prem_list_directories);

	PDL_RegisterJSHandler("run", prem_run);
	PDL_RegisterJSHandler("kill", prem_run);

	if (signal(SIGINT, termination_handler) == SIG_IGN)
		signal(SIGINT, SIG_IGN);
	if (signal(SIGHUP, termination_handler) == SIG_IGN)
		signal(SIGHUP, SIG_IGN);
	if (signal(SIGTERM, termination_handler) == SIG_IGN)
		signal(SIGTERM, SIG_IGN);

	SDL_Event event;
	while (SDL_WaitEvent(&event)) {
		// sleep indefinitely
	}
	return 0;
}
