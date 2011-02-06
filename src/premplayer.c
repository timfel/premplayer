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

// The starting size for the directory entries vector
#define MIN_DIR_ENTRY_LENGTH 2048

#define DEBUG_LOG(x) { FILE* dbg = fopen("/tmp/premplayer.txt", "a"); fprintf(dbg, x); fclose(dbg); }


pid_t mplayer_pid;
char mplayer_path[1024] = {'\0'}; // Be nasty - pre-allocate enough space for a path
char* entries = NULL;
int entry_size = MIN_DIR_ENTRY_LENGTH; // The current size of the return buffer

PDL_bool prem_list(PDL_JSParameters *parms, unsigned char type) {
	int argc = PDL_GetNumJSParams(parms);
	if (argc != 1) {
		return EMPTY_STRING(parms);
	}
	const char* argv = PDL_GetJSParamString(parms, 0);
	if (!argv) {
		return EMPTY_STRING(parms);
	}

	DEBUG_LOG("Argument for list: ");
	DEBUG_LOG(argv);
	DEBUG_LOG("\n");

	DIR* dir = opendir(argv);
	DEBUG_LOG("Has dir\n");
	if (dir) {
		DEBUG_LOG("Opened dir\n");
		int entry_p = 0; // The position we're at in the return buffer
		entries[entry_p] = '\0';
		DEBUG_LOG("Entries reset\n");

		struct dirent *ent;
		while (ent = readdir(dir)) {
			DEBUG_LOG("Read entry\n");
			if (ent->d_type == type) {
				DEBUG_LOG("Use entry\n");
				// The given entry matches the requested type
				int name_length = strlen(ent->d_name) + 1; // The names are separated by "/", so +1

				// Resize the buffer, if the new string wouldn't fit
				if (entry_p + name_length >= entry_size) {
					DEBUG_LOG("Grow entries\n");
					entry_size += MAX(MIN_DIR_ENTRY_LENGTH, name_length + 1);
					char* entries_ng = (char*)realloc(entries, sizeof(char) * entry_size);
					DEBUG_LOG("Done growing\n");
					if (!entries_ng) break; // We cannot grow, return what we have
					entries = entries_ng; // Grown
					DEBUG_LOG("Settling growth\n");
				}

				// Add a separator
				strcat(entries, "/");
				// Add the new entry to the buffer
				strcat(entries, ent->d_name);
				DEBUG_LOG("Name: ");
				DEBUG_LOG(ent->d_name);
				DEBUG_LOG("\n");
				entry_p += name_length;
			}
		}
		closedir (dir);
		DEBUG_LOG("Returning ");
		DEBUG_LOG(entries);
		DEBUG_LOG("\n");
		PDL_JSReply(parms, entries); // Return the "/" separated list of entries
		return PDL_TRUE;
	} else {
		DEBUG_LOG("Open failed\n");
		return EMPTY_STRING(parms);
	}
}

// Return the list of file names under a path
PDL_bool prem_list_files(PDL_JSParameters *parms) {
	DEBUG_LOG("Listing files");
	return prem_list(parms, DT_REG);
}

// Return the list of directory names under a path
PDL_bool prem_list_directories(PDL_JSParameters *parms) {
	DEBUG_LOG("Listing directories");
	return prem_list(parms, DT_DIR);
}

void prem_kill_bang() {
	if (mplayer_pid != 0) {
		(void)kill(mplayer_pid, SIGKILL);
		mplayer_pid = 0;
	}
}

// Kill the running mplayer instance, if any
PDL_bool prem_kill(PDL_JSParameters *parms) {
	DEBUG_LOG("Killing from JS\n");
	prem_kill_bang();
	return RETVAL_TRUE(parms);
}

// Run mplayer with the passed files qualifier
PDL_bool prem_run(PDL_JSParameters *parms) {
	DEBUG_LOG("RUN");
	// Kill any running instance
	prem_kill_bang();
	DEBUG_LOG("Killed\n");

	// Get the arguments - should be a list file paths - wildcards allowed
	int argc = PDL_GetNumJSParams(parms);
	if (argc < 1) {
		return RETVAL_FALSE(parms);
	}

	const char* argument = PDL_GetJSParamString(parms, 0);
	DEBUG_LOG("Argument for run: ");
	DEBUG_LOG(argument);
	DEBUG_LOG("\n");

	mplayer_pid = fork();
	if (mplayer_pid == -1) {
		DEBUG_LOG("fork failed!!!\n");
		mplayer_pid = 0;
		return RETVAL_FALSE(parms);
	}
	if (mplayer_pid) {
		// parent
		return RETVAL_TRUE(parms);
	} else {
		// child
		execl(mplayer_path, argument);
	}
	return RETVAL_TRUE(parms);
}

void termination_handler(int signum) {
	prem_kill_bang();
	exit(0);
}

int main(int argc, char** argv) {
	strcpy(mplayer_path, argv[0]);
	char *path_end = rindex(mplayer_path, '/');
	if (!path_end) return 1; // Error
	path_end[1] = '\0'; // cutoff after the last path delimiter
	strcat(mplayer_path, "mplayer");
	printf("Registered mplayer path: %s.\n", mplayer_path);

	// Pre-allocate the run-buffer
	entries = (char*)calloc(sizeof(char), MIN_DIR_ENTRY_LENGTH + 1);

	if (signal(SIGINT, termination_handler) == SIG_IGN)
		signal(SIGINT, SIG_IGN);
	if (signal(SIGHUP, termination_handler) == SIG_IGN)
		signal(SIGHUP, SIG_IGN);
	if (signal(SIGTERM, termination_handler) == SIG_IGN)
		signal(SIGTERM, SIG_IGN);

	int error = SDL_Init(SDL_INIT_VIDEO);
	PDL_Init(0);

	PDL_Err reg;
	reg = PDL_RegisterJSHandler("list_files", prem_list_files);
	reg += PDL_RegisterJSHandler("list_directories", prem_list_directories);
	reg += PDL_RegisterJSHandler("run", prem_run);
	reg += PDL_RegisterJSHandler("kill", prem_kill);

	if (reg == PDL_NOERROR) {
		DEBUG_LOG("Registration of signals done\n");
	}

	reg = PDL_JSRegistrationComplete();
	if (reg == PDL_NOERROR) {
		DEBUG_LOG("Registration complete\n");
	}

	SDL_Event event;
	do {
		SDL_WaitEvent(&event);
	} while (event.type != SDL_QUIT);

	return 0;
}
