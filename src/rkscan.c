/*******************************************************************************
 * Copyright(C)		: Rockontrol Industrial Park.
 * Version			: V1.0
 * Author			: KangQi
 * Tel				: 18636181581/6581
 * Email			: goodfuturemail@gmail.com
 * File Name		: rkscan.c
 * Created At		: 2013-09-23 14:29
 * Last Modified	: 2013-11-11 14:56
 * Description		: 
 * History			: 
*******************************************************************************/

#include "rkscan.h"
#include <dirent.h>
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

static struct libMap libmap;

int rkLibScan(const char *path)
{
	DIR *dirp;
	void *lib;
	struct dirent *dir;
	char tmp[256], *ptr;
	char *(*func)();

	memset(&libmap, 0, sizeof(struct libMap));

	dirp = opendir(path);
	if (!dirp) {
		fprintf(stderr, "%s : \"%s\" %s.\n", __func__, path, strerror(errno));
		return -1; 
	}

	while((dir = readdir(dirp)) != NULL) {
		if (!strstr(dir->d_name, ".so")) {
			continue;
		}   

		if (libmap.size >= MAX_LIB_NUM) {
			fprintf(stderr, "%s : Only Support Maximum %d Librarys.\n", __func__, MAX_LIB_NUM);
			break;
		}

		if (path[strlen(path)] == '/') {
			sprintf(tmp, "%s%s", path, dir->d_name);
		} else {
			sprintf(tmp, "%s/%s", path, dir->d_name);
		}  

		lib = dlopen(tmp, RTLD_LAZY);
		if (!lib) {
			fprintf(stderr, "%s.\n", dlerror());
			continue;
		}   

		func = dlsym(lib, "name");
		if (!func) {
			fprintf(stderr, "%s.\n", dlerror());
			dlclose(lib);
			continue;
		}

		if ((ptr = func()) != NULL) {
			libmap.lname[libmap.size] = strdup(ptr);
			libmap.fpath[libmap.size] = strdup(tmp);
			libmap.size++;
		}
		dlclose(lib);
	}
	closedir(dirp);

	return 0;
}

const char *rkLibGetPath(const char *lname)
{
	int i;

	if (!lname || !strcmp(lname, "none")) {
		return NULL;
	}

	for (i = 0; i < libmap.size; i++) {
		if (!strcmp(libmap.lname[i], lname)) {
			return libmap.fpath[i];
		}
	}

	return NULL;
}
