/*

  This file is distributed under the GNU Public License, version 2 or at
  your option any later version. Read the file gpl.txt for details.

  scandir/access function for wii.
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef GEKKO
#include <errno.h>
#include <dirent.h>

int access(const char *pathname, int mode) {
	struct stat sb;

	if (stat(pathname, &sb) == -1) {
		return -1;
	}

	return 0;
}

int alphasort(const void *d1, const void *d2)
{
	return strcmp((*(struct dirent * const *)d1)->d_name,
		      (*(struct dirent * const *)d2)->d_name);
}

int scandir(const char *dirp, struct dirent ***namelist,
            int (*filter)(struct dirent *),
            int (*compar)(const void *, const void *))
{

    DIR *dp = opendir (dirp);
    struct dirent *current;
    struct dirent **names = NULL;
    size_t names_size = 0, pos;

    if (dp == NULL)
        return -1;

    errno = 0;

    pos = 0;
    while ((current = readdir (dp)) != NULL)
        if (filter == NULL || (*filter) (current))
        {
            struct dirent *vnew;
            size_t dsize;

            if (pos == names_size)
            {
                struct dirent **new;
                if (names_size == 0)
                    names_size = 10;
                else
                    names_size *= 2;
                new = (struct dirent **) realloc (names, names_size * sizeof (struct dirent *));
                if (new == NULL)
                    break;
                names = new;
            }

            dsize = &current->d_name[strlen(current->d_name)+1] - (char *) current;
            vnew = (struct dirent *) malloc (dsize);
            if (vnew == NULL)
                break;

            names[pos++] = (struct dirent *) memcpy (vnew, current, dsize);
        }

    if (errno != 0)
    {
        closedir (dp);
        while (pos > 0)
            free (names[--pos]);
        free (names);

        return -1;
    }

    closedir (dp);


    /* Sort the list if we have a comparison function to sort with.  */
    if (compar != NULL)
        qsort (names, pos, sizeof (struct dirent *), compar);
    *namelist = names;
    return pos;
}


#endif

/*-----------------------------------------------------------------------
 * Here come alphasort and scandir for BeOS and SunOS
 *-----------------------------------------------------------------------*/
#if defined(__BEOS__) || (defined(__sun) && defined(__SVR4))

#undef DIRSIZ

#define DIRSIZ(dp)                                          \
		((sizeof(struct dirent) - sizeof(dp)->d_name) +     \
		(((dp)->d_reclen + 1 + 3) &~ 3))

#if (defined(__sun) && defined(__SVR4)) || defined(__CEGCC__)
# define dirfd(d) ((d)->dd_fd)
#elif defined(__BEOS__)
# define dirfd(d) ((d)->fd)
#endif


/*-----------------------------------------------------------------------*/
/**
 * Alphabetic order comparison routine.
 */
int alphasort(const void *d1, const void *d2)
{
	return strcmp((*(struct dirent * const *)d1)->d_name, (*(struct dirent * const *)d2)->d_name);
}


/*-----------------------------------------------------------------------*/
/**
 * Scan a directory for all its entries
 * Return -1 on error, number of entries on success
 */
int scandir(const char *dirname, struct dirent ***namelist, int (*sdfilter)(struct dirent *), int (*dcomp)(const void *, const void *))
{
	struct dirent *d, *p = NULL, **names = NULL;
	struct stat stb;
	size_t nitems = 0;
	size_t arraysz;
	DIR *dirp;

	if ((dirp = opendir(dirname)) == NULL)
		goto error_out;

	if (fstat(dirfd(dirp), &stb) < 0)
		goto error_out;

	/*
	 * estimate the array size by taking the size of the directory file
	 * and dividing it by a multiple of the minimum size entry.
	 */
	arraysz = (stb.st_size / 24);

	names = (struct dirent **)malloc(arraysz * sizeof(struct dirent *));
	if (names == NULL)
		goto error_out;

	while ((d = readdir(dirp)) != NULL)
	{

		if (sdfilter != NULL && !(*sdfilter)(d))
			continue;       /* just selected names */

		/*
		 * Make a minimum size copy of the data
		 */

		p = (struct dirent *)malloc(DIRSIZ(d));
		if (p == NULL)
			goto error_out;

		p->d_ino = d->d_ino;
		p->d_reclen = d->d_reclen;
		/*p->d_namlen = d->d_namlen;*/
		memcpy(p->d_name, d->d_name, p->d_reclen + 1);

		/*
		 * Check to make sure the array has space left and
		 * realloc the maximum size.
		 */

		if ((nitems+1) >= arraysz)
		{
			struct dirent **tmp;
			
			if (fstat(dirfd(dirp), &stb) < 0)
				goto error_out;   /* just might have grown */

			arraysz = stb.st_size / 12;

			tmp = (struct dirent **)realloc((char *)names, arraysz * sizeof(struct dirent *));
			if (tmp == NULL)
				goto error_out;
			names = tmp;
		}

		names[nitems++] = p;
		p = NULL;
	}

	closedir(dirp);

	if (nitems && dcomp != NULL)
		qsort(names, nitems, sizeof(struct dirent *), dcomp);

	*namelist = names;

	return nitems;

error_out:
	if (names)
	{
		int i;
		for (i = 0; i < nitems; i++)
			free(names[i]);
		free(names);
	}
	if (dirp)
		closedir(dirp);
	return -1;
}


#endif /* __BEOS__ || __sun */


/*-----------------------------------------------------------------------
 * Here come alphasort and scandir for Windows
 *-----------------------------------------------------------------------*/
#if defined(WIN32) || defined(__CEGCC__)

#include <windows.h>
#include <wchar.h>

/*-----------------------------------------------------------------------*/
/**
 * Alphabetic order comparison routine.
 */
int alphasort(const void *d1, const void *d2)
{
	return stricmp((*(struct dirent * const *)d1)->d_name, (*(struct dirent * const *)d2)->d_name);
}

/*-----------------------------------------------------------------------*/
/**
 * Scan a directory for all its entries
 */
int scandir(const char *dirname, struct dirent ***namelist, int (*sdfilter)(struct dirent *), int (*dcomp)(const void *, const void *))
{
	int len;
	char *findIn, *d;
	WIN32_FIND_DATA find;
	HANDLE h;
	int nDir = 0, NDir = 0;
	struct dirent **dir = 0, *selectDir;
	unsigned long ret;

	len    = strlen(dirname);
	findIn = (char *)malloc(len+5);
	if (!findIn)
		return -1;

	strcpy(findIn, dirname);
	Log_Printf(LOG_DEBUG, "scandir : findIn orign='%s'\n", findIn);

	for (d = findIn; *d; d++)
		if (*d=='/')
			*d='\\';
	if ((len==0))
	{
		strcpy(findIn, ".\\*");
	}
	if ((len==1)&& (d[-1]=='.'))
	{
		strcpy(findIn, ".\\*");
	}
	if ((len>0) && (d[-1]=='\\'))
	{
		*d++ = '*';
		*d = 0;
	}
	if ((len>1) && (d[-1]=='.') && (d[-2]=='\\'))
	{
		d[-1] = '*';
	}
	if ((len>1) && !(d[-2]=='\\' && d[-1]=='*') )
	{
		*d++ = '\\';
		*d++ = '*';
		*d = 0;
	}

	Log_Printf(LOG_DEBUG, "scandir : findIn processed='%s'\n", findIn);

#if defined(__CEGCC__)
	void *findInW = NULL;
	findInW = malloc((len+6)*2);
	if (!findInW)
		return -1;
	mbstowcs(findInW, findIn, len+6);
	h = FindFirstFileW(findInW, &find);
#else
	h = FindFirstFile(findIn, &find);
#endif

	if (h == INVALID_HANDLE_VALUE)
	{
		Log_Printf(LOG_DEBUG, "scandir : FindFirstFile error\n");
		ret = GetLastError();
		if (ret != ERROR_NO_MORE_FILES)
		{
			// TODO: return some error code
		}
		*namelist = dir;
		return nDir;
	}

	do
	{
		selectDir=(struct dirent*)malloc(sizeof(struct dirent)+lstrlen(find.cFileName)+1);
#if defined(__CEGCC__)
		wcstombs(selectDir->d_name, find.cFileName, lstrlen(find.cFileName)+1);
#else
		strcpy(selectDir->d_name, find.cFileName);
#endif
		//Log_Printf(LOG_DEBUG, "scandir : findFile='%s'\n", selectDir->d_name);
		if (!sdfilter || (*sdfilter)(selectDir))
		{
			if (nDir==NDir)
			{
				struct dirent **tempDir = (struct dirent **)calloc(sizeof(struct dirent*), NDir+33);
				if (NDir)
					memcpy(tempDir, dir, sizeof(struct dirent*)*NDir);
				if (dir)
					free(dir);
				dir = tempDir;
				NDir += 32;
			}
			dir[nDir] = selectDir;
			nDir++;
			dir[nDir] = 0;
		}
		else
		{
			free(selectDir);
		}

#if defined(__CEGCC__)
		ret = FindNextFileW(h, &find);
#else
		ret = FindNextFile(h, &find);
#endif
	}
	while (ret);

	ret = GetLastError();
	if (ret != ERROR_NO_MORE_FILES)
	{
		// TODO: return some error code
		Log_Printf(LOG_DEBUG, "scandir: last error = %ld\n", ret);
	}

	FindClose(h);

	free(findIn);

#if defined(__CEGCC__)
	free(findInW);
#endif

	if (dcomp)
		qsort (dir, nDir, sizeof(*dir),dcomp);

	*namelist = dir;
	return nDir;
}

#endif /* WIN32 */
