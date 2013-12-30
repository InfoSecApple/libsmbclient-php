/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2002 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Matthew Sachs <matthewg@zevils.com>                         |
   |          Eduardo Bacchi Kienetz <eduardo@kienetz.com>                |
   +----------------------------------------------------------------------+
 */

#define IS_EXT_MODULE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_libsmbclient.h"

#include <libsmbclient.h>

#define LIBSMBCLIENT_VERSION	"0.2"

#ifdef ZTS
int libsmbclient_globals_id;
#else
#define php_libsmbclient_globals libsmbclient_globals;
#endif

#ifdef ZTS
static void php_libsmbclient_init_globals(php_libsmbclient_globals *libsmbclient_globals_p TSRMLS_DC)
{
}
#endif

static char *
find_char (char *start, char *last, char q)
{
	char *c;
	for (c = start; c <= last; c++) {
		if (*c == q) {
			return c;
		}
	}
	return NULL;
}

static char *
find_second_char (char *start, char *last, char q)
{
	char *c;
	if ((c = find_char(start, last, q)) == NULL) {
		return NULL;
	}
	return find_char(c + 1, last, q);
}

static void
astfill (char *start, char *last)
{
	char *c;
	for (c = start; c <= last; c++) {
		*c = '*';
	}
}

static void
hide_password (char *url, int len)
{
	/* URL should have the form:
	 *   smb://[[[domain;]user[:password@]]server[/share[/path[/file]]]]
	 * Replace everything after the second colon and before the next @
	 * with asterisks. */
	char *last = (url + len) - 1;
	char *second_colon;
	char *slash;
	char *at_sign;

	if (len <= 0) {
		return;
	}
	if ((second_colon = find_second_char(url, last, ':')) == NULL) {
		return;
	}
	if ((slash = find_char(second_colon + 1, last, '/')) == NULL) {
		slash = last + 1;
	}
	if ((at_sign = find_char(second_colon + 1, last, '@')) == NULL) {
		astfill(second_colon + 1, slash - 1);
		return;
	}
	if (at_sign > slash) {
		at_sign = slash;
	}
	astfill(second_colon + 1, at_sign - 1);
}

static zend_function_entry libsmbclient_functions[] =
{
	PHP_FE(smbclient_opendir, NULL)
	PHP_FE(smbclient_readdir, NULL)
	PHP_FE(smbclient_closedir, NULL)
	PHP_FE(smbclient_stat, NULL)
	PHP_FE(smbclient_open, NULL)
	PHP_FE(smbclient_creat, NULL)
	PHP_FE(smbclient_read, NULL)
	PHP_FE(smbclient_close, NULL)
	PHP_FE(smbclient_mkdir, NULL)
	PHP_FE(smbclient_rmdir, NULL)
	PHP_FE(smbclient_rename, NULL)
	PHP_FE(smbclient_write, NULL)
	PHP_FE(smbclient_unlink, NULL)
	{NULL, NULL, NULL}
};

zend_module_entry libsmbclient_module_entry =
	{ STANDARD_MODULE_HEADER
	, "libsmbclient"		/* name                  */
	, libsmbclient_functions	/* functions             */
	, PHP_MINIT(smbclient)		/* module_startup_func   */
	, PHP_MSHUTDOWN(smbclient)	/* module_shutdown_func  */
	, PHP_RINIT(smbclient)		/* request_startup_func  */
	, NULL				/* request_shutdown_func */
	, PHP_MINFO(smbclient)		/* info_func             */
	, LIBSMBCLIENT_VERSION		/* version               */
	, STANDARD_MODULE_PROPERTIES
	} ;

#ifdef COMPILE_DL_LIBSMBCLIENT
ZEND_GET_MODULE(libsmbclient)
#endif


static void
smbclient_auth_func (const char *server, const char *share, char *workgroup, int wglen, char *username, int userlen, char *password, int passlen)
{
}

PHP_MINIT_FUNCTION(smbclient)
{
	#ifdef ZTS
	ts_allocate_id(&libsmbclient_globals_id, sizeof(php_libsmbclient_globals), (ts_allocate_ctor) php_libsmbclient_init_globals, NULL);
	#endif

	if (smbc_init(smbclient_auth_func, 0) == 0) {
		return SUCCESS;
	}
	switch (errno) {
		case ENOMEM: php_error(E_WARNING, "Couldn't initialize libsmbclient: Out of memory."); break;
		case ENOENT: php_error(E_WARNING, "Couldn't initialize libsmbclient: The smb.conf file would not load.  It must be located in ~/.smb/ (probably /root/.smb) ."); break;
		case EINVAL: php_error(E_WARNING, "Couldn't initialize libsmbclient: Invalid parameter."); break;
		default: php_error(E_WARNING, "Couldn't initialize libsmbclient: Unknown error (%d).", errno); break;
	}
	return FAILURE;	
}

PHP_RINIT_FUNCTION(smbclient)
{
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(smbclient)
{
	return SUCCESS;
}

PHP_MINFO_FUNCTION(smbclient)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "libsmbclient Support", "enabled");
	php_info_print_table_row(2, "libsmbclient Version", LIBSMBCLIENT_VERSION);
	php_info_print_table_end();
}

PHP_FUNCTION(smbclient_opendir)
{
	char *path;
	int path_len, dirhandle;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	if ((dirhandle = smbc_opendir(path)) >= 0) {
		RETURN_LONG(dirhandle);
	}
	hide_password(path, path_len);
	switch (errno) {
		case EACCES: php_error(E_WARNING, "Couldn't open SMB directory %s: Permission denied", path); break;
		case EINVAL: php_error(E_WARNING, "Couldn't open SMB directory %s: Invalid URL", path); break;
		case ENOENT: php_error(E_WARNING, "Couldn't open SMB directory %s: Path does not exist", path); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't open SMB directory %s: Insufficient memory", path); break;
		case ENOTDIR: php_error(E_WARNING, "Couldn't open SMB directory %s: Not a directory", path); break;
		case EPERM: php_error(E_WARNING, "Couldn't open SMB directory %s: Workgroup not found", path); break;
		case ENODEV: php_error(E_WARNING, "Couldn't open SMB directory %s: Workgroup or server not found", path); break;
		default: php_error(E_WARNING, "Couldn't open SMB directory %s: Unknown error (%d)", path, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_readdir)
{
	int dirhandle;
	struct smbc_dirent *dirent;
	char *type;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &dirhandle) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	errno = 0;
	dirent = smbc_readdir(dirhandle);
	if (dirent == NULL) {
		switch (errno) {
			case 0: RETURN_FALSE;
			case EBADF: php_error(E_WARNING, "Couldn't read SMB directory handle %d: Not a directory handle", dirhandle); break;
			case EINVAL: php_error(E_WARNING, "Couldn't read SMB directory handle %d: smbc_init not called", dirhandle); break;
			default: php_error(E_WARNING, "Couldn't read SMB directory handle %d: Unknown error (%d)", dirhandle, errno); break;
		}
		RETURN_FALSE;
	}
	if (array_init(return_value) != SUCCESS) {
		php_error(E_WARNING, "Couldn't initialize array");
		RETURN_FALSE;
	}
	switch (dirent->smbc_type) {
		case SMBC_WORKGROUP: type = "workgroup"; break;
		case SMBC_SERVER: type = "server"; break;
		case SMBC_FILE_SHARE: type = "file share"; break;
		case SMBC_PRINTER_SHARE: type = "printer share"; break;
		case SMBC_COMMS_SHARE: type = "communication share"; break;
		case SMBC_IPC_SHARE: type = "IPC share"; break;
		case SMBC_DIR: type = "directory"; break;
		case SMBC_FILE: type = "file"; break;
		case SMBC_LINK: type = "link"; break;
		default: type = "unknown"; break;
	}
	add_assoc_string(return_value, "type", type, 1);
	add_assoc_stringl(return_value, "comment", dirent->comment, dirent->commentlen, 1);
	add_assoc_stringl(return_value, "name", dirent->name, dirent->namelen, 1);
}

PHP_FUNCTION(smbclient_closedir)
{
	int dirhandle, retval;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &dirhandle) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	if (smbc_closedir(dirhandle) == 0) {
		RETURN_TRUE;
	}
	switch (errno) {
		case EBADF: php_error(E_WARNING, "Couldn't close SMB directory handle %d: Not a directory handle", dirhandle); break;
		default: php_error(E_WARNING, "Couldn't close SMB directory handle %d: Unknown error (%d)", dirhandle, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_rename)
{
	char *ourl, *nurl;
	int ourl_len, nurl_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &ourl, &ourl_len, &nurl, &nurl_len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	if (smbc_rename(ourl, nurl) == 0) {
		RETURN_TRUE;
	}
	hide_password(ourl, ourl_len);
	switch (errno) {
		case EISDIR: php_error(E_WARNING, "Couldn't rename SMB directory %s: existing url is not a directory", ourl); break;
		case EACCES: php_error(E_WARNING, "Couldn't open SMB directory %s: Permission denied", ourl); break;
		case EINVAL: php_error(E_WARNING, "Couldn't open SMB directory %s: Invalid URL", ourl); break;
		case ENOENT: php_error(E_WARNING, "Couldn't open SMB directory %s: Path does not exist", ourl); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't open SMB directory %s: Insufficient memory", ourl); break;
		case ENOTDIR: php_error(E_WARNING, "Couldn't open SMB directory %s: Not a directory", ourl); break;
		case EPERM: php_error(E_WARNING, "Couldn't open SMB directory %s: Workgroup not found", ourl); break;
		case EXDEV: php_error(E_WARNING, "Couldn't open SMB directory %s: Workgroup or server not found", ourl); break;
		case EEXIST: php_error(E_WARNING, "Couldn't rename SMB directory %s: new name already exists", ourl); break;
		default: php_error(E_WARNING, "Couldn't open SMB directory %s: Unknown error (%d)", ourl, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_unlink)
{
	char *url;
	int url_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &url, &url_len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	if (smbc_unlink(url) == 0) {
		RETURN_TRUE;
	}
	hide_password(url, url_len);
	switch (errno) {
		case EACCES: php_error(E_WARNING, "Couldn't delete %s: Permission denied", url); break;
		case EINVAL: php_error(E_WARNING, "Couldn't delete %s: Invalid URL", url); break;
		case ENOENT: php_error(E_WARNING, "Couldn't delete %s: Path does not exist", url); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't delete %s: Insufficient memory", url); break;
		case EPERM: php_error(E_WARNING, "Couldn't delete %s: Workgroup not found", url); break;
		case EISDIR: php_error(E_WARNING, "Couldn't delete %s: It is a Directory (use rmdir instead)", url); break;
		default: php_error(E_WARNING, "Couldn't delete %s: Unknown error (%d)", url, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_mkdir)
{
	char *path = NULL;
	int path_len;
	char *mode = "0700";

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &path, &path_len, &mode) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	if (smbc_mkdir(path, (mode_t)mode) == 0) {
		RETURN_TRUE;
	}
	hide_password(path, path_len);
	switch (errno) {
		case EACCES: php_error(E_WARNING, "Couldn't create SMB directory %s: Permission denied", path); break;
		case EINVAL: php_error(E_WARNING, "Couldn't create SMB directory %s: Invalid URL", path); break;
		case ENOENT: php_error(E_WARNING, "Couldn't create SMB directory %s: Path does not exist", path); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't create SMB directory %s: Insufficient memory", path); break;
		case EEXIST: php_error(E_WARNING, "Couldn't create SMB directory %s: Directory already exists", path); break;
		default: php_error(E_WARNING, "Couldn't create SMB directory %s: Unknown error (%d)", path, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_rmdir)
{
	char *url;
	int url_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &url, &url_len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	if (smbc_rmdir(url) == 0) {
		RETURN_TRUE;
	}
	hide_password(url, url_len);
	switch (errno) {
		case EACCES: php_error(E_WARNING, "Couldn't delete %s: Permission denied", url); break;
		case EINVAL: php_error(E_WARNING, "Couldn't delete %s: Invalid URL", url); break;
		case ENOENT: php_error(E_WARNING, "Couldn't delete %s: Path does not exist", url); break;
		case ENOMEM: php_error(E_WARNING, "Couldn't delete %s: Insufficient memory", url); break;
		case EPERM: php_error(E_WARNING, "Couldn't delete %s: Workgroup not found", url); break;
		case ENOTEMPTY: php_error(E_WARNING, "Couldn't delete %s: It is not empty", url); break;
		default: php_error(E_WARNING, "Couldn't delete %s: Unknown error (%d)", url, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_stat)
{
	char *file;
	struct stat statbuf;
	int retval, file_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &file, &file_len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	retval = smbc_stat(file, &statbuf);
	if (retval < 0) {
		hide_password(file, file_len);
		switch (errno) {
			case ENOENT: php_error(E_WARNING, "Couldn't stat %s: Does not exist", file); break;
			case EINVAL: php_error(E_WARNING, "Couldn't stat: null URL or smbc_init failed"); break;
			case EACCES: php_error(E_WARNING, "Couldn't stat %s: Permission denied", file); break;
			case ENOMEM: php_error(E_WARNING, "Couldn't stat %s: Out of memory", file); break;
			case ENOTDIR: php_error(E_WARNING, "Couldn't stat %s: Not a directory", file); break;
			default: php_error(E_WARNING, "Couldn't stat %s: Unknown error (%d)", file, errno); break;
		}
		RETURN_FALSE;
	}
	if (array_init(return_value) != SUCCESS) {
		php_error(E_WARNING, "Couldn't initialize array");
		RETURN_FALSE;
	}
	add_index_long(return_value, 0, statbuf.st_dev);
	add_index_long(return_value, 1, statbuf.st_ino);
	add_index_long(return_value, 2, statbuf.st_mode);
	add_index_long(return_value, 3, statbuf.st_nlink);
	add_index_long(return_value, 4, statbuf.st_uid);
	add_index_long(return_value, 5, statbuf.st_gid);
	add_index_long(return_value, 6, statbuf.st_rdev);
	add_index_long(return_value, 7, statbuf.st_size);
	add_index_long(return_value, 8, statbuf.st_atime);
	add_index_long(return_value, 9, statbuf.st_mtime);
	add_index_long(return_value, 10, statbuf.st_ctime);
	add_index_long(return_value, 11, statbuf.st_blksize);
	add_index_long(return_value, 12, statbuf.st_blocks);

	add_assoc_long(return_value, "dev", statbuf.st_dev);
	add_assoc_long(return_value, "ino", statbuf.st_ino);
	add_assoc_long(return_value, "mode", statbuf.st_mode);
	add_assoc_long(return_value, "nlink", statbuf.st_nlink);
	add_assoc_long(return_value, "uid", statbuf.st_uid);
	add_assoc_long(return_value, "gid", statbuf.st_gid);
	add_assoc_long(return_value, "rdev", statbuf.st_rdev);
	add_assoc_long(return_value, "size", statbuf.st_size);
	add_assoc_long(return_value, "atime", statbuf.st_atime);
	add_assoc_long(return_value, "mtime", statbuf.st_mtime);
	add_assoc_long(return_value, "ctime", statbuf.st_ctime);
	add_assoc_long(return_value, "blksize", statbuf.st_blksize);
	add_assoc_long(return_value, "blocks", statbuf.st_blocks);
}

PHP_FUNCTION(smbclient_open)
{
	char *file;
	int file_len;
	int handle;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &file, &file_len) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	if ((handle = smbc_open(file, O_RDONLY, 0666)) >= 0) {
		RETURN_LONG(handle);
	}
	hide_password(file, file_len);
	switch (errno) {
		case ENOMEM: php_error(E_WARNING, "Couldn't open %s: Out of memory", file); break;
		case EINVAL: php_error(E_WARNING, "Couldn't open %s: No file?", file); break;
		case EEXIST: php_error(E_WARNING, "Couldn't open %s: Pathname already exists and O_CREAT and O_EXECL were specified", file); break;
		case EISDIR: php_error(E_WARNING, "Couldn't open %s: Can't write to a directory", file); break;
		case EACCES: php_error(E_WARNING, "Couldn't open %s: Access denied", file); break;
		case ENODEV: php_error(E_WARNING, "Couldn't open %s: Requested share does not exist", file); break;
		case ENOTDIR: php_error(E_WARNING, "Couldn't open %s: Path component isn't a directory", file); break;
		case ENOENT: php_error(E_WARNING, "Couldn't open %s: Directory in path doesn't exist", file); break;
		default: php_error(E_WARNING, "Couldn't open %s: Unknown error (%d)", file, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_creat)
{
	char *file, *mode = "0700";
	int file_len;
	int handle;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &file, &file_len, &mode) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	if ((handle = smbc_creat(file, (mode_t)mode)) >= 0) {
		RETURN_LONG(handle);
	}
	hide_password(file, file_len);
	switch (errno) {
		case ENOMEM: php_error(E_WARNING, "Couldn't create %s: Out of memory", file); break;
		case EINVAL: php_error(E_WARNING, "Couldn't create %s: No file?", file); break;
		case EEXIST: php_error(E_WARNING, "Couldn't create %s: Pathname already exists and O_CREAT and O_EXECL were specified", file); break;
		case EISDIR: php_error(E_WARNING, "Couldn't create %s: Can't write to a directory", file); break;
		case EACCES: php_error(E_WARNING, "Couldn't create %s: Access denied", file); break;
		case ENODEV: php_error(E_WARNING, "Couldn't create %s: Requested share does not exist", file); break;
		case ENOENT: php_error(E_WARNING, "Couldn't create %s: Directory in path doesn't exist", file); break;
		default: php_error(E_WARNING, "Couldn't create %s: Unknown error (%d)", file, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_read)
{
	long file, count;
	ssize_t nbytes;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll", &file, &count) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	if (count < 0) {
		php_error(E_WARNING, "Negative byte count: %ld", count);
		RETURN_FALSE;
	}
	void *buf = emalloc(count);

	if ((nbytes = smbc_read(file, buf, count)) >= 0) {
		RETURN_STRINGL(buf, nbytes, 0);
	}
	efree(buf);
	switch (errno) {
		case EISDIR: php_error(E_WARNING, "Couldn't read from %d: Is a directory", file); break;
		case EBADF: php_error(E_WARNING, "Couldn't read from %d: Not a valid file descriptor or not open for reading", file); break;
		case EINVAL: php_error(E_WARNING, "Couldn't read from %d: Object not suitable for reading or bad buffer", file); break;
		default: php_error(E_WARNING, "Couldn't read from %d: Unknown error (%d)", file, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_write)
{
	long file, count;
	int str_len;
	char * str;
	ssize_t nbytes;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "lsl", &file, &str, &str_len, &count) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	if (count < 0) {
		php_error(E_WARNING, "Negative byte count: %ld", count);
		RETURN_FALSE;
	}
	if ((nbytes = smbc_write(file, str, count)) >= 0) {
		RETURN_LONG(nbytes);
	}
	switch (errno) {
		case EISDIR: php_error(E_WARNING, "Couldn't read from %d: Is a directory", file); break;
		case EBADF: php_error(E_WARNING, "Couldn't read from %d: Not a valid file descriptor or not open for reading", file); break;
		case EINVAL: php_error(E_WARNING, "Couldn't read from %d: Object not suitable for reading or bad buffer", file); break;
		default: php_error(E_WARNING, "Couldn't read from %d: Unknown error (%d)", file, errno); break;
	}
	RETURN_FALSE;
}

PHP_FUNCTION(smbclient_close)
{
	int file;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &file) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	if (smbc_close(file) == 0) {
		RETURN_TRUE;
	}
	switch (errno) {
		case EBADF: php_error(E_WARNING, "Couldn't close %d: Not a valid file descriptor or not open for reading", file); break;
		case EINVAL: php_error(E_WARNING, "Couldn't close %d: smbc_init not called", file); break;
		default: php_error(E_WARNING, "Couldn't close %d: Unknown error (%d)", file, errno); break;
	}
	RETURN_FALSE;
}
