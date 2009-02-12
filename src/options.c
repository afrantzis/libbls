/**
 * @file options.c
 *
 * Options implementation
 */

#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include "options.h"
#include "util.h"


struct options {
	char **values;
	size_t size;
};

/** 
 * Creates a new options_t.
 * 
 * @param[out] opts the created options_t
 * @param nopts the number of options in the created options_t 
 * 
 * @return the operation error code
 */
int options_new(options_t **opts, size_t nopts)
{
	if (opts == NULL)
		return_error(EINVAL);

	options_t *o = malloc(sizeof *o);
	if (o == NULL)
		return_error(ENOMEM);

	o->values = malloc(nopts * sizeof *(o->values));

	for(size_t i = 0; i < nopts; i++)
		o->values[i] = NULL;

	o->size = nopts;
	*opts = o;

	return 0;
}

/** 
 * Gets the value of an option.
 *
 * The return option value is the char * which is used internally
 * so it must not be altered.
 * 
 * @param opts the options_t to get the option from
 * @param[out] val the value of the specified option
 * @param key the key of the option to get
 * 
 * @return the operation error code
 */
int options_get_option(options_t *opts, char **val, size_t key)
{
	if (opts == NULL || val == NULL || key >= opts->size)
		return_error(EINVAL);

	*val = opts->values[key];
		
	return 0;
}

/** 
 * Sets the value of an option.
 *
 * A copy of the provided char * value is used internally.
 * 
 * @param opts the options_t to set the option in
 * @param key the key of the option to set
 * @param key val the value to set the specified option to
 * 
 * @return the operation error code
 */
int options_set_option(options_t *opts, size_t key, char *val)
{
	if (opts == NULL || key >= opts->size)
		return_error(EINVAL);

	opts->values[key] = strdup(val);
		
	return 0;
}

int options_free(options_t *opts)
{
	if (opts == NULL)
		return_error(EINVAL);

	for(size_t i = 0; i < opts->size; i++)
		free(opts->values[i]);

	free(opts->values);
	free(opts);

	return 0;
}

