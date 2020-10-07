/*
 * UNG's Not GNU
 *
 * Copyright (c) 2020, Jakob Kaivo <jkk@ung.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define _XOPEN_SOURCE 700
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef LINE_MAX
#define LINE_MAX _POSIX_LINE_MAX
#endif

static int sort_check(const char *input, char sep, const char *key, unsigned int flags)
{
	(void)sep;
	(void)key;
	(void)flags;

	char prev[LINE_MAX] = "";
	char cur[LINE_MAX] = "";
	uintmax_t line = 1;

	FILE *in = stdin;
	if (strcmp(input, "-")) {
		in = fopen(input, "r");
	}
	if (in == NULL) {
		fprintf(stderr, "sort: %s: %s\n", input, strerror(errno));
	}

	while (fgets(cur, sizeof(cur), in) != NULL) {
		if (strcoll(cur, prev) < 0) {
			fprintf(stderr, "sort: %s: disorder at line %ju\n",
				input, line);
			return 1;
		}

		strcpy(prev, cur);
		line++;
	}

	fclose(in);

	return 0;
}

static int sort_read(const char *path, char ***lines, size_t *nlines)
{
	FILE *f = stdin;
	if (strcmp(path, "-")) {
		f = fopen(path, "r");
	}
	if (f == NULL) {
		fprintf(stderr, "sort: %s: %s\n", path, strerror(errno));
		return 1;
	}

	char cur[LINE_MAX] = "";

	while (fgets(cur, sizeof(cur), f) != NULL) {
		char **tmp = realloc(*lines, sizeof(*lines) * (*nlines + 1));
		if (tmp == NULL) {
			abort();
		}

		*lines = tmp;
		(*lines)[*nlines] = strdup(cur);
		if ((*lines)[*nlines] == NULL) {
			abort();
		}

		(*nlines)++;
	}

	if (f != stdin) {
		fclose(f);
	}

	return 0;
}

static int sort_compar(const void *a, const void *b)
{
	const char *l1 = a;
	const char *l2 = b;

	return strcoll(l1, l2);
}

static int sort_sort(char *files[], int nfiles, char sep, const char *key, unsigned int flags)
{
	(void)sep;
	(void)key;
	(void)flags;

	char **lines = NULL;
	size_t nlines = 0;

	for (int i = 0; i < nfiles; i++) {
		if (sort_read(files[i], &lines, &nlines) != 0) {
			return 1;
		}
	}

	qsort(lines, nlines, sizeof(*lines), sort_compar);

	for (size_t i = 0; i < nlines; i++) {
		printf("%s", lines[i]);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");

	char *output = "-";
	enum { SORT, MERGE, CHECK } mode = SORT;
	char separator = '\0';
	char *key = NULL;
	unsigned int flags = 0;

	/* TODO: handle + prefixed options */

	int c;
	while ((c = getopt(argc, argv, "mcCo:t:k:bdfinru")) != -1) {
		switch (c) {
		case 'm':
			mode = MERGE;
			break;

		case 'C':
			/* disable warning */
			/* FALLTHROUGH */
		case 'c':
			mode = CHECK;
			break;

		case 'o':
			output = optarg;
			break;

		case 't':
			separator = *optarg;
			break;

		case 'k':
			key = optarg;
			break;

		case 'b':
		case 'd':
		case 'f':
		case 'i':
		case 'n':
		case 'r':
		case 'u':
			break;

		default:
			return 1;
		}
	}

	(void)output;

	if (mode == CHECK) {
		return sort_check(argv[optind], separator, key, flags);
	}

	#if 0
	if (mode == MERGE) {
		/* optimize to assume sorted input */
		return sort_merge(argv + optind, argc - optind, separator, key, flags);
	}
	#endif

	return sort_sort(argv + optind, argc - optind, separator, key, flags);
}
