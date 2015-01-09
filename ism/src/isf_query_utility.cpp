/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Haifeng Deng <haifeng.deng@samsung.com>, Hengliang Luo <hl.luo@samsung.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#define Uses_SCIM_DEBUG
#define Uses_SCIM_IMENGINE
#define Uses_SCIM_IMENGINE_MODULE
#define Uses_SCIM_HELPER_MODULE
#define Uses_SCIM_CONFIG
#define Uses_SCIM_CONFIG_MODULE
#define Uses_SCIM_CONFIG_PATH
#define Uses_C_LOCALE
#define Uses_SCIM_UTILITY
#define Uses_SCIM_PANEL_AGENT


#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <dlog.h>
#include "scim_private.h"
#include "scim.h"
#include "isf_query_utility.h"
#include "scim_helper.h"
#include <pkgmgr-info.h>
#include <db-util.h>


using namespace scim;


/////////////////////////////////////////////////////////////////////////////
// Declaration of macro.
/////////////////////////////////////////////////////////////////////////////
#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG                                         "ISF_QUERY"

/*!
 * \note
 * DB Table schema
 *
 * ime_info
 * +-------+------+-------+-----------+-----------+-------+-------------+-----------+-----------+
 * | appid | uuid | label | languages | iconpath  | pkgid | pkgrootpath |  pkgtype  |  kbdtype  |
 * +-------+------+-------+-----------+-----------+-------+-------------+-----------+-----------+
 * |   -   |  -   |   -   |     -     |   -       |   -   |     -       |     -     |      -    |
 * +-------+------+-------+-----------+-----------+-------+-------------+-----------+-----------+
 *
 * CREATE TABLE ime_info ( appid TEXT PRIMARY KEY NOT NULL, uuid TEXT, label TEXT, languages TEXT, iconpath TEXT, pkgid TEXT, pkgrootpath TEXT, pkgtype TEXT, kbdtype TEXT)
 *
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
// DATABASE
///////////////////////////////////////////////////////////////////////////////////////////////////
#define DB_PATH "/opt/usr/dbspace/.ime_parser.db"
static struct {
    const char* pPath;
    sqlite3* pHandle;
} databaseInfo = {
    DB_PATH, NULL
};

static inline int _begin_transaction(void)
{
	sqlite3_stmt* pStmt = NULL;

	int ret = sqlite3_prepare_v2(databaseInfo.pHandle, "BEGIN TRANSACTION", -1, &pStmt, NULL);
	if (ret != SQLITE_OK)
	{
		LOGE("sqlite3_prepare_v2: %s", sqlite3_errmsg(databaseInfo.pHandle));
		return EXIT_FAILURE;
	}

	if (sqlite3_step(pStmt) != SQLITE_DONE)
	{
		LOGE("sqlite3_step: %s", sqlite3_errmsg(databaseInfo.pHandle));
		sqlite3_finalize(pStmt);
		return EXIT_FAILURE;
	}

	sqlite3_finalize(pStmt);
	return EXIT_SUCCESS;
}

static inline int _rollback_transaction(void)
{
	sqlite3_stmt* pStmt = NULL;

	int ret = sqlite3_prepare_v2(databaseInfo.pHandle, "ROLLBACK TRANSACTION", -1, &pStmt, NULL);
	if (ret != SQLITE_OK)
	{
		LOGE("sqlite3_prepare_v2: %s", sqlite3_errmsg(databaseInfo.pHandle));
		return EXIT_FAILURE;
	}

	if (sqlite3_step(pStmt) != SQLITE_DONE)
	{
		LOGE("sqlite3_step: %s", sqlite3_errmsg(databaseInfo.pHandle));
		sqlite3_finalize(pStmt);
		return EXIT_FAILURE;
	}

	sqlite3_finalize(pStmt);
	return EXIT_SUCCESS;
}

static inline int _commit_transaction(void)
{
	sqlite3_stmt* pStmt = NULL;

	int ret = sqlite3_prepare_v2(databaseInfo.pHandle, "COMMIT TRANSACTION", -1, &pStmt, NULL);
	if (ret != SQLITE_OK)
	{
		LOGE("sqlite3_prepare_v2: %s", sqlite3_errmsg(databaseInfo.pHandle));
		return EXIT_FAILURE;
	}

	if (sqlite3_step(pStmt) != SQLITE_DONE)
	{
		LOGE("sqlite3_step: %s", sqlite3_errmsg(databaseInfo.pHandle));
		sqlite3_finalize(pStmt);
		return EXIT_FAILURE;
	}

	sqlite3_finalize(pStmt);
	return EXIT_SUCCESS;
}

static inline int _db_create_ime_info(void)
{
	char* pException = NULL;
	static const char* pQuery = "CREATE TABLE ime_info (appid TEXT PRIMARY KEY NOT NULL, uuid TEXT, label TEXT, languages TEXT, iconpath TEXT, pkgid TEXT, pkgrootpath TEXT, pkgtype TEXT, kbdtype TEXT)";

	if (sqlite3_exec(databaseInfo.pHandle, pQuery, NULL, NULL, &pException) != SQLITE_OK) {
		LOGE("%s", pException);
		return -EIO;
	}

	if (sqlite3_changes(databaseInfo.pHandle) == 0) {
		LOGD("The database is not changed.");
	}

	return 0;
}

static inline void _db_create_table(void)
{
	_begin_transaction();

	int ret = _db_create_ime_info();
	if (ret < 0) {
		_rollback_transaction();
		return;
	}

	_commit_transaction();
}

static inline int _db_init(void)
{
	struct stat stat;

	int ret = db_util_open(databaseInfo.pPath, &databaseInfo.pHandle, DB_UTIL_REGISTER_HOOK_METHOD);
	if (ret != SQLITE_OK) {
		LOGE("sqlite3 return code: %d", ret);
		return -EIO;
	}

	if (lstat(databaseInfo.pPath, &stat) < 0) {
		LOGE("%s", strerror(errno));
		db_util_close(databaseInfo.pHandle);
		databaseInfo.pHandle = NULL;
		return -EIO;
	}

	if (!S_ISREG(stat.st_mode)) {
		LOGE("S_ISREG failed.");
		db_util_close(databaseInfo.pHandle);
		databaseInfo.pHandle = NULL;
		return -EINVAL;
	}

	if (!stat.st_size) {
		LOGE("The RPM file has not been installed properly.");
		_db_create_table();
	}

	return 0;
}

static inline int _db_connect(void)
{
	if (!databaseInfo.pHandle) {
		int ret = _db_init();
		if (ret < 0)
		{
			LOGE("%d", ret);
			return -EIO;
		}
	}

	return 0;
}

static inline int _db_disconnect(void)
{
	if (!databaseInfo.pHandle)
		return 0;

	db_util_close(databaseInfo.pHandle);

	databaseInfo.pHandle = NULL;

	return 0;
}

/**
 * @brief Insert data to ime_info table.
 *
 * @param ime_info The list to store ImeInfoDB
 *
 * @return the number of inserted data.
 */
static inline int _db_insert_ime_info(std::vector<ImeInfoDB> &ime_info)
{
	int ret = 0, i = 0;
	sqlite3_stmt* pStmt = NULL;
	std::vector<ImeInfoDB>::iterator iter;
	static const char* pQuery = "INSERT INTO ime_info (appid, uuid, label, languages, iconpath, pkgid, pkgrootpath, pkgtype, kbdtype) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

	ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
	if (ret != SQLITE_OK) {
		LOGE("sqlite3_prepare_v2: %s", sqlite3_errmsg(databaseInfo.pHandle));
		return 0;
	}

	for (iter = ime_info.begin (); iter != ime_info.end (); iter++) {
		ret = sqlite3_bind_text(pStmt, 1, iter->appid.c_str(), -1, SQLITE_TRANSIENT);
		if (ret != SQLITE_OK) {
			LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
			goto out;
		}

		ret = sqlite3_bind_text(pStmt, 2, iter->uuid.c_str(), -1, SQLITE_TRANSIENT);
		if (ret != SQLITE_OK) {
			LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
			goto out;
		}

		ret = sqlite3_bind_text(pStmt, 3, iter->label.c_str(), -1, SQLITE_TRANSIENT);
		if (ret != SQLITE_OK) {
			LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
			goto out;
		}

		ret = sqlite3_bind_text(pStmt, 4, iter->languages.c_str(), -1, SQLITE_TRANSIENT);
		if (ret != SQLITE_OK) {
			LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
			goto out;
		}

		ret = sqlite3_bind_text(pStmt, 5, iter->iconpath.c_str(), -1, SQLITE_TRANSIENT);
		if (ret != SQLITE_OK) {
			LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
			goto out;
		}

		ret = sqlite3_bind_text(pStmt, 6, iter->pkgid.c_str(), -1, SQLITE_TRANSIENT);
		if (ret != SQLITE_OK) {
			LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
			goto out;
		}

		ret = sqlite3_bind_text(pStmt, 7, iter->pkgrootpath.c_str(), -1, SQLITE_TRANSIENT);
		if (ret != SQLITE_OK) {
			LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
			goto out;
		}

		ret = sqlite3_bind_text(pStmt, 8, iter->pkgtype.c_str(), -1, SQLITE_TRANSIENT);
		if (ret != SQLITE_OK) {
			LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
			goto out;
		}

		ret = sqlite3_bind_text(pStmt, 9, iter->kbdtype.c_str(), -1, SQLITE_TRANSIENT);
		if (ret != SQLITE_OK) {
			LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
			goto out;
		}

		if (sqlite3_step(pStmt) != SQLITE_DONE) {
			LOGE("sqlite3_step: %s", sqlite3_errmsg(databaseInfo.pHandle));
			ret = SQLITE_ERROR;
			goto out;
		}
		else {
			LOGD("Insert \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"",
				iter->appid.c_str(), iter->uuid.c_str(), iter->label.c_str(), iter->languages.c_str(),
				iter->iconpath.c_str(), iter->pkgid.c_str(), iter->pkgrootpath.c_str(), iter->pkgtype.c_str(),
				iter->kbdtype.c_str());
		}
		sqlite3_reset(pStmt);
		sqlite3_clear_bindings(pStmt);
		i++;
	}

out:
	if (ret != SQLITE_OK) {
		sqlite3_reset(pStmt);
		sqlite3_clear_bindings(pStmt);
	}
	sqlite3_finalize(pStmt);

	return i;
}

/**
 * @brief Update label data by appid in ime_info table.
 *
 * @param appid appid in ime_info table.
 * @param label label to be updated in ime_info table.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
static inline int _db_update_label_ime_info(const char *appid, const char *label)
{
	int ret = 0;
	sqlite3_stmt* pStmt = NULL;
	static const char* pQuery = "UPDATE ime_info SET label = ? WHERE appid = ?";

	if (appid == NULL || label == NULL) {
		LOGE("input is NULL.");
		return 0;
	}

	ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
	if (ret != SQLITE_OK) {
		LOGE("sqlite3_prepare_v2: %s", sqlite3_errmsg(databaseInfo.pHandle));
		return 0;
	}

	ret = sqlite3_bind_text(pStmt, 1, label, -1, SQLITE_TRANSIENT);
	if (ret != SQLITE_OK) {
		LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
		ret = 0;
		goto out;
	}

	ret = sqlite3_bind_text(pStmt, 2, appid, -1, SQLITE_TRANSIENT);
	if (ret != SQLITE_OK) {
		LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
		ret = 0;
		goto out;
	}

	ret = sqlite3_step(pStmt);
	if (ret != SQLITE_DONE) {
		LOGE("sqlite3_step: %s", sqlite3_errmsg(databaseInfo.pHandle));
		ret = 0;
	}
	else {
		LOGD("UPDATE ime_info SET label = %s WHERE appid = %s", label, appid);
		ret = 1;
	}

out:
	sqlite3_reset(pStmt);
	sqlite3_clear_bindings(pStmt);
	sqlite3_finalize(pStmt);
	return ret;
}

static int filtered_app_list_cb (const pkgmgrinfo_appinfo_h handle, void *user_data)
{
	int ret = 0;
	char *appid = NULL, *icon = NULL, *pkgid = NULL, *pkgtype = NULL, *label = NULL, *path = NULL;
	pkgmgrinfo_pkginfo_h  pkginfo_handle = NULL;
	ImeInfoDB ime_db;
	std::vector<ImeInfoDB> ime_info;

	/* appid */
	ret = pkgmgrinfo_appinfo_get_appid(handle, &appid);
	if (ret == PMINFO_R_OK)
		ime_db.appid = String(appid);

	/* iconpath */
	ret = pkgmgrinfo_appinfo_get_icon( handle, &icon);
	if (ret == PMINFO_R_OK)
		ime_db.iconpath = String(icon);

	/* pkgid */
	ret = pkgmgrinfo_appinfo_get_pkgid(handle, &pkgid);
	if (ret == PMINFO_R_OK)
		ime_db.pkgid = String(pkgid);

	/* get pkgmgrinfo_pkginfo_h */
	ret = pkgmgrinfo_pkginfo_get_pkginfo(pkgid, &pkginfo_handle);
	if (ret == PMINFO_R_OK && pkginfo_handle) {
		/* pkgtype */
		ret = pkgmgrinfo_pkginfo_get_type(pkginfo_handle, &pkgtype);
		if (ret == PMINFO_R_OK)
			ime_db.pkgtype = String(pkgtype);

		/* label */
		ret = pkgmgrinfo_pkginfo_get_label(pkginfo_handle, &label);
		if (ret == PMINFO_R_OK)
			ime_db.label = String(label);

		/* pkgrootpath */
		ret = pkgmgrinfo_pkginfo_get_root_path(pkginfo_handle, &path);
		if (ret == PMINFO_R_OK)
			ime_db.pkgrootpath = String(path);

		pkgmgrinfo_pkginfo_destroy_pkginfo(pkginfo_handle);
	}

	ime_db.languages = String("en");

	ime_info.push_back(ime_db);
	_db_insert_ime_info(ime_info);

	return 0;
}

/**
 * @brief Select all ime_info table.
 *
 * @param ime_info The list to store ImeInfoDB
 *
 * @return the number of selected row.
 */
static inline int _db_select_all_ime_info(std::vector<ImeInfoDB> &ime_info)
{
	int ret = 0, i = 0;
	bool firsttry = true;
	ImeInfoDB info;
	sqlite3_stmt* pStmt = NULL;
	std::vector <String> module_list;
	String module_name;
	size_t found = 0;
	static const char* pQuery = "SELECT * FROM ime_info";


	do {
		if (i == 0) {
			ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
			if (ret != SQLITE_OK) {
				LOGE("sqlite3_prepare_v2: %s", sqlite3_errmsg(databaseInfo.pHandle));
				return 0;
			}
		}

		ret = sqlite3_step(pStmt);
		if (ret == SQLITE_ROW) {
			info.appid = String((char *)sqlite3_column_text(pStmt, 0));
			info.uuid = String((char *)sqlite3_column_text(pStmt, 1));
			info.label = String((char *)sqlite3_column_text(pStmt, 2));
			info.languages = String((char *)sqlite3_column_text(pStmt, 3));
			info.iconpath = String((char *)sqlite3_column_text(pStmt, 4));
			info.pkgid = String((char *)sqlite3_column_text(pStmt, 5));
			info.pkgrootpath = String((char *)sqlite3_column_text(pStmt, 6));
			info.pkgtype = String((char *)sqlite3_column_text(pStmt, 7));
			info.kbdtype = String((char *)sqlite3_column_text(pStmt, 8));

			LOGD("appid=\"%s\", uuid=\"%s\", label=\"%s\", languages=\"%s\", iconpath=\"%s\", pkgid=\"%s\", pkgrootpath=\"%s\", pkgtype=\"%s\", kbdtype=\"%s\"",
				info.appid.c_str(),
				info.uuid.c_str(),
				info.label.c_str(),
				info.languages.c_str(),
				info.iconpath.c_str(),
				info.pkgid.c_str(),
				info.pkgrootpath.c_str(),
				info.pkgtype.c_str(),
				info.kbdtype.c_str());

			if (info.kbdtype.compare("SOFTWARE_KEYBOARD_ISE") == 0) {
				info.mode = TOOLBAR_HELPER_MODE;
				info.options = SCIM_HELPER_STAND_ALONE | SCIM_HELPER_NEED_SCREEN_INFO | SCIM_HELPER_AUTO_RESTART;

				if (info.pkgtype.compare("wgt") == 0) {
					info.options = SCIM_HELPER_STAND_ALONE | SCIM_HELPER_NEED_SCREEN_INFO | SCIM_HELPER_NEED_SPOT_LOCATION_INFO | SCIM_HELPER_AUTO_RESTART;
					info.module_name = String("ise-web-helper-agent");
				}
				else 	{
					info.options = SCIM_HELPER_STAND_ALONE | SCIM_HELPER_NEED_SCREEN_INFO | SCIM_HELPER_AUTO_RESTART;

					found = info.appid.find_last_of('.');
					if (found != String::npos)
						info.module_name = info.appid.substr(found+1);
					else
						info.module_name = info.appid;

					if (scim_get_helper_module_list(module_list)) {
						if (std::find (module_list.begin(), module_list.end(), info.module_name) == module_list.end())
							LOGE("Module name \"%s\" can't be found in scim_get_helper_module_list().", info.module_name.c_str());
					}
				}
			}
			else /*if (info.kbdtype.compare("HARDWARE_KEYBOARD_ISE") == 0)*/ {
				info.mode = TOOLBAR_KEYBOARD_MODE;
				info.options = 0;

				found = info.appid.find_last_of('.');
				if (found != String::npos)
					info.module_name = info.appid.substr(found+1);
				else
					info.module_name = info.appid;

				if (scim_get_imengine_module_list(module_list)) {
					if (std::find (module_list.begin(), module_list.end(), info.module_name) == module_list.end())
						LOGE("Module name \"%s\" can't be found in scim_get_imengine_module_list().", info.module_name.c_str());
				}
			}

			ime_info.push_back(info);
			i++;
		}
		else if (i == 0 && firsttry) {
			LOGD("sqlite3_step returned %d, empty DB", ret);
			firsttry = false;

			sqlite3_reset(pStmt);
			sqlite3_clear_bindings(pStmt);
			sqlite3_finalize(pStmt);

			pkgmgrinfo_appinfo_filter_h handle;
			ret = pkgmgrinfo_appinfo_filter_create(&handle);
			if (ret == PMINFO_R_OK) {
				ret = pkgmgrinfo_appinfo_filter_add_string(handle, PMINFO_APPINFO_PROP_APP_CATEGORY, "http://tizen.org/category/ime");
				if (ret == PMINFO_R_OK) {
					ret = pkgmgrinfo_appinfo_filter_foreach_appinfo(handle, filtered_app_list_cb, NULL);
				}
				pkgmgrinfo_appinfo_filter_destroy(handle);
			}
			ret = SQLITE_ROW;
		}
	} while (ret == SQLITE_ROW);

	if (ret != SQLITE_DONE) {
		LOGE("sqlite3_step: %s", sqlite3_errmsg(databaseInfo.pHandle));
	}

	sqlite3_reset(pStmt);
	sqlite3_clear_bindings(pStmt);
	sqlite3_finalize(pStmt);
	return i;
}

/**
 * @brief Select all ime_info table.
 *
 * @param appid appid to select ime_info row.
 * @param pImeInfo The pointer of ImeInfoDB.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
static inline int _db_select_ime_info(const char *appid, ImeInfoDB *pImeInfo)
{
	int ret = 0, i = 0;
	sqlite3_stmt* pStmt = NULL;
	static const char* pQuery = "SELECT * FROM ime_info WHERE appid = ?";

	if (appid == NULL || pImeInfo == NULL) {
		LOGE("invalid parameter.");
		return 0;
	}

	pImeInfo->appid.clear();
	pImeInfo->uuid.clear();
	pImeInfo->label.clear();
	pImeInfo->languages.clear();
	pImeInfo->iconpath.clear();
	pImeInfo->pkgid.clear();
	pImeInfo->pkgrootpath.clear();
	pImeInfo->pkgtype.clear();
	pImeInfo->kbdtype.clear();

	ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
	if (ret != SQLITE_OK) {
		LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
		return 0;
	}

	ret = sqlite3_bind_text(pStmt, 1, appid, -1, SQLITE_TRANSIENT);
	if (ret != SQLITE_OK) {
		LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
		goto out;
	}

	ret = sqlite3_step(pStmt);
	if (ret != SQLITE_ROW) {
		LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
		goto out;
	}
	i = 1;

	pImeInfo->appid = String((char*)sqlite3_column_text(pStmt, 0));
	pImeInfo->uuid = String((char*)sqlite3_column_text(pStmt, 1));
	pImeInfo->label = String((char*)sqlite3_column_text(pStmt, 2));
	pImeInfo->languages = String((char*)sqlite3_column_text(pStmt, 3));
	pImeInfo->iconpath = String((char*)sqlite3_column_text(pStmt, 4));
	pImeInfo->pkgid = String((char*)sqlite3_column_text(pStmt, 5));
	pImeInfo->pkgrootpath = String((char*)sqlite3_column_text(pStmt, 6));
	pImeInfo->pkgtype = String((char*)sqlite3_column_text(pStmt, 7));
	pImeInfo->kbdtype = String((char*)sqlite3_column_text(pStmt, 8));

	LOGD("appid=\"%s\", uuid=\"%s\", label=\"%s\", languages=\"%s\", iconpath=\"%s\", pkgid=\"%s\", pkgrootpath=\"%s\", pkgtype=\"%s\", kbdtype=\"%s\"",
		pImeInfo->appid.c_str(),
		pImeInfo->uuid.c_str(),
		pImeInfo->label.c_str(),
		pImeInfo->languages.c_str(),
		pImeInfo->iconpath.c_str(),
		pImeInfo->pkgid.c_str(),
		pImeInfo->pkgrootpath.c_str(),
		pImeInfo->pkgtype.c_str(),
		pImeInfo->kbdtype.c_str());

out:
	sqlite3_reset(pStmt);
	sqlite3_clear_bindings(pStmt);
	sqlite3_finalize(pStmt);
	return i;
}

/**
 * @brief Select all ime_info table.
 *
 * @param ime_info The list to store ImeInfoDB
 *
 * @return the number of selected row.
 */
EAPI int isf_db_select_all_ime_info(std::vector<ImeInfoDB> &ime_info)
{
	int ret = 0;

	ime_info.clear();

	if (_db_connect() == 0) {
		ret = _db_select_all_ime_info(ime_info);
		_db_disconnect();
	}

	return ret;
}

/**
 * @brief Insert data to ime_info table.
 *
 * @param ime_info The list to store ImeInfoDB
 *
 * @return the number of inserted data.
 */
EAPI int isf_db_insert_ime_info(std::vector<ImeInfoDB> &ime_info)
{
	int ret = 0;

	if (ime_info.size() < 1) {
		LOGE("Nothing to insert.");
		return ret;
	}

	if (_db_connect() == 0) {
		ret = _db_insert_ime_info(ime_info);
		_db_disconnect();
	}

	return ret;
}

/**
 * @brief Update label data by appid in ime_info table.
 *
 * @param appid appid in ime_info table.
 * @param label label to be updated in ime_info table.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
EAPI int isf_db_update_label_ime_info(const char *appid, const char *label)
{
	int ret = 0;
	if (_db_connect() == 0) {
		ret = _db_update_label_ime_info(appid, label);
		_db_disconnect();
	}
	return ret;
}

/*
vi:ts=4:nowrap:ai:expandtab
*/
