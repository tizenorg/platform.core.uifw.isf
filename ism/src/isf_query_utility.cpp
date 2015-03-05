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
 * +-------+-------+-----------+----------+-------+----------+------+-------+-------+------+---------+
 * | appid | label | languages | iconpath | pkgid |  pkgtype | exec | mname | mpath | mode | options |
 * +-------+-------+-----------+----------+-------+----------+------+-------+-------+------+---------+
 *
 * CREATE TABLE ime_info (appid TEXT PRIMARY KEY NOT NULL, label TEXT, languages TEXT, iconpath TEXT, pkgid TEXT, pkgtype TEXT, exec TEXT mname TEXT, mpath TEXT, mode INTEGER, options INTEGER)
 *
 */

///////////////////////////////////////////////////////////////////////////////////////////////////
// DATABASE
///////////////////////////////////////////////////////////////////////////////////////////////////
#define DB_PATH "/opt/usr/dbspace/.ime_info.db"
static struct {
    const char* pPath;
    sqlite3* pHandle;
} databaseInfo = {
    DB_PATH, NULL
};

static int _filtered_app_list_cb (const pkgmgrinfo_appinfo_h handle, void *user_data);

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
    static const char* pQuery = "CREATE TABLE ime_info (appid TEXT PRIMARY KEY NOT NULL, label TEXT, languages TEXT, iconpath TEXT, pkgid TEXT, pkgtype TEXT, exec TEXT, mname TEXT, mpath TEXT, mode INTEGER, options INTEGER)";

    if (sqlite3_exec(databaseInfo.pHandle, pQuery, NULL, NULL, &pException) != SQLITE_OK) {
        LOGE("sqlite3_exec: %s", pException);
        sqlite3_free(pException);
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
        LOGE("db_util_open returned code: %d", ret);
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
 * @brief Select all ime_info table.
 *
 * @param ime_info The list to store ImeInfoDB
 *
 * @return the number of selected row.
 */
static int _db_select_all_ime_info(std::vector<ImeInfoDB> &ime_info)
{
    int ret = 0, i = 0;
    bool firsttry = true;
    ImeInfoDB info;
    sqlite3_stmt* pStmt = NULL;
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
            info.label = String((char *)sqlite3_column_text(pStmt, 1));
            info.languages = String((char *)sqlite3_column_text(pStmt, 2));
            info.iconpath = String((char *)sqlite3_column_text(pStmt, 3));
            info.pkgid = String((char *)sqlite3_column_text(pStmt, 4));
            info.pkgtype = String((char *)sqlite3_column_text(pStmt, 5));
            info.exec = String((char *)sqlite3_column_text(pStmt, 6));
            info.module_name = String((char *)sqlite3_column_text(pStmt, 7));
            info.module_path = String((char *)sqlite3_column_text(pStmt, 8));
            info.mode = (TOOLBAR_MODE_T)sqlite3_column_int(pStmt, 9);
            info.options = (uint32)sqlite3_column_int(pStmt, 10);

            SECURE_LOGD("appid=\"%s\", label=\"%s\", langs=\"%s\", icon=\"%s\", pkgid=\"%s\", pkgtype=\"%s\", exec=\"%s\", mname=\"%s\", mpath=\"%s\", mode=%d, options=%u",
                info.appid.c_str(),
                info.label.c_str(),
                info.languages.c_str(),
                info.iconpath.c_str(),
                info.pkgid.c_str(),
                info.pkgtype.c_str(),
                info.exec.c_str(),
                info.module_name.c_str(),
                info.module_path.c_str(),
                info.mode,
                info.options);

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
                    ret = pkgmgrinfo_appinfo_filter_foreach_appinfo(handle, _filtered_app_list_cb, NULL);
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
 * @brief Select ime_info table with appid.
 *
 * @param appid appid to select ime_info row.
 * @param pImeInfo The pointer of ImeInfoDB.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
static int _db_select_ime_info_by_appid(const char *appid, ImeInfoDB *pImeInfo)
{
    int ret = 0, i = 0;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "SELECT * FROM ime_info WHERE appid = ?";

    pImeInfo->appid.clear();
    pImeInfo->label.clear();
    pImeInfo->languages.clear();
    pImeInfo->iconpath.clear();
    pImeInfo->pkgid.clear();
    pImeInfo->pkgtype.clear();
    pImeInfo->exec.clear();
    pImeInfo->module_name.clear();
    pImeInfo->module_path.clear();
    pImeInfo->mode = TOOLBAR_KEYBOARD_MODE;
    pImeInfo->options = 0;

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
    pImeInfo->label = String((char*)sqlite3_column_text(pStmt, 1));
    pImeInfo->languages = String((char*)sqlite3_column_text(pStmt, 2));
    pImeInfo->iconpath = String((char*)sqlite3_column_text(pStmt, 3));
    pImeInfo->pkgid = String((char*)sqlite3_column_text(pStmt, 4));
    pImeInfo->pkgtype = String((char*)sqlite3_column_text(pStmt, 5));
    pImeInfo->exec = String((char*)sqlite3_column_text(pStmt, 6));
    pImeInfo->module_name = String((char *)sqlite3_column_text(pStmt, 7));
    pImeInfo->module_path = String((char *)sqlite3_column_text(pStmt, 8));
    pImeInfo->mode = (TOOLBAR_MODE_T)sqlite3_column_int(pStmt, 9);
    pImeInfo->options = (uint32)sqlite3_column_int(pStmt, 10);

    SECURE_LOGD("appid=\"%s\", label=\"%s\", langs=\"%s\", icon=\"%s\", pkgid=\"%s\", pkgtype=\"%s\", exec=\"%s\", mname=\"%s\", mpath=\"%s\", mode=%d, options=%u",
        pImeInfo->appid.c_str(),
        pImeInfo->label.c_str(),
        pImeInfo->languages.c_str(),
        pImeInfo->iconpath.c_str(),
        pImeInfo->pkgid.c_str(),
        pImeInfo->pkgtype.c_str(),
        pImeInfo->exec.c_str(),
        pImeInfo->module_name.c_str(),
        pImeInfo->module_path.c_str(),
        pImeInfo->mode,
        pImeInfo->options);

out:
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
    sqlite3_finalize(pStmt);
    return i;
}

/**
 * @brief Select ime_info mname data.
 *
 * @param mname The list to store module name
 * @param mode  ISE type
 *
 * @return the number of selected row.
 */
static int _db_select_module_name_by_mode(TOOLBAR_MODE_T mode, std::vector<String> &mname)
{
    int ret = 0, i = 0;
    bool firsttry = true;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "SELECT mname FROM ime_info WHERE mode = ?";

    do {
        if (i == 0) {
            ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
            if (ret != SQLITE_OK) {
                LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
                return 0;
            }

            ret = sqlite3_bind_int(pStmt, 1, mode);
            if (ret != SQLITE_OK) {
                LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
                goto out;
            }
        }

        ret = sqlite3_step(pStmt);
        if (ret == SQLITE_ROW) {
            mname.push_back(String((char *)sqlite3_column_text(pStmt, 0)));
            SECURE_LOGD("%s: \"%s\"", (mode? "Helper": "IMEngine"), mname.back().c_str());
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
                    ret = pkgmgrinfo_appinfo_filter_foreach_appinfo(handle, _filtered_app_list_cb, NULL);
                }
                pkgmgrinfo_appinfo_filter_destroy(handle);
            }
            ret = SQLITE_ROW;
        }
    } while (ret == SQLITE_ROW);

    if (ret != SQLITE_DONE) {
        LOGE("sqlite3_step: %s", sqlite3_errmsg(databaseInfo.pHandle));
    }

out:
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
    sqlite3_finalize(pStmt);
    return i;
}

/**
 * @brief Select ime_info mpath data.
 *
 * @param mpath The list to store module path
 * @param mode  ISE type
 *
 * @return the number of selected row.
 */
static int _db_select_module_path_by_mode(TOOLBAR_MODE_T mode, std::vector<String> &mpath)
{
    int ret = 0, i = 0;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "SELECT mpath FROM ime_info WHERE mode = ?";

    ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        return 0;
    }

    ret = sqlite3_bind_int(pStmt, 1, mode);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    do {
        ret = sqlite3_step(pStmt);
        if (ret == SQLITE_ROW) {
            mpath.push_back(String((char *)sqlite3_column_text(pStmt, 0)));
            SECURE_LOGD("%s: \"%s\"", (mode? "Helper": "IMEngine"), mpath.back().c_str());
            i++;
        }
    } while (ret == SQLITE_ROW);

    if (ret != SQLITE_DONE) {
        LOGE("sqlite3_step: %s", sqlite3_errmsg(databaseInfo.pHandle));
    }

out:
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
    sqlite3_finalize(pStmt);
    return i;
}

/**
 * @brief Select appids by pkgid in ime_info table.
 *
 * @param pkgid pkgid to get appid data.
 * @param appids    The list to store appid; there might be multiple appids for one pkgid.
 *
 * @return the number of selected row.
 */
static int _db_select_appids_by_pkgid(const char *pkgid, std::vector<String> &appids)
{
    int ret = 0, i = 0;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "SELECT appid FROM ime_info WHERE pkgid = ?";

    appids.clear();

    ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        return 0;
    }

    ret = sqlite3_bind_text(pStmt, 1, pkgid, -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    do {
        ret = sqlite3_step(pStmt);
        if (ret == SQLITE_ROW) {
            appids.push_back(String((char *)sqlite3_column_text(pStmt, 0)));
            SECURE_LOGD("appid=\"%s\"", appids.back().c_str());
            i++;
        }
    } while (ret == SQLITE_ROW);

    if (ret != SQLITE_DONE) {
        LOGE("sqlite3_step: %s", sqlite3_errmsg(databaseInfo.pHandle));
    }

out:
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
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
static int _db_update_label_by_appid(const char *appid, const char *label)
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
        ISF_SAVE_LOG("sqlite3_step returned %d, %s", ret, sqlite3_errmsg(databaseInfo.pHandle));
        LOGE("sqlite3_step returned %d, %s", ret, sqlite3_errmsg(databaseInfo.pHandle));
        ret = 0;
    }
    else {
        SECURE_LOGD("UPDATE ime_info SET label = %s WHERE appid = %s", label, appid);
        ret = 1;
    }

out:
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
    sqlite3_finalize(pStmt);
    return ret;
}

/**
 * @brief Insert data to ime_info table.
 *
 * @param ime_info The list to store ImeInfoDB
 *
 * @return the number of inserted data.
 */
static int _db_insert_ime_info(std::vector<ImeInfoDB> &ime_info)
{
    int ret = 0, i = 0;
    sqlite3_stmt* pStmt = NULL;
    std::vector<ImeInfoDB>::iterator iter;
    static const char* pQuery = "INSERT INTO ime_info (appid, label, languages, iconpath, pkgid, pkgtype, exec, mname, mpath, mode, options) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

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

        ret = sqlite3_bind_text(pStmt, 2, iter->label.c_str(), -1, SQLITE_TRANSIENT);
        if (ret != SQLITE_OK) {
            LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
            goto out;
        }

        ret = sqlite3_bind_text(pStmt, 3, iter->languages.c_str(), -1, SQLITE_TRANSIENT);
        if (ret != SQLITE_OK) {
            LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
            goto out;
        }

        ret = sqlite3_bind_text(pStmt, 4, iter->iconpath.c_str(), -1, SQLITE_TRANSIENT);
        if (ret != SQLITE_OK) {
            LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
            goto out;
        }

        ret = sqlite3_bind_text(pStmt, 5, iter->pkgid.c_str(), -1, SQLITE_TRANSIENT);
        if (ret != SQLITE_OK) {
            LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
            goto out;
        }

        ret = sqlite3_bind_text(pStmt, 6, iter->pkgtype.c_str(), -1, SQLITE_TRANSIENT);
        if (ret != SQLITE_OK) {
            LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
            goto out;
        }

        ret = sqlite3_bind_text(pStmt, 7, iter->exec.c_str(), -1, SQLITE_TRANSIENT);
        if (ret != SQLITE_OK) {
            LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
            goto out;
        }

        ret = sqlite3_bind_text(pStmt, 8, iter->module_name.c_str(), -1, SQLITE_TRANSIENT);
        if (ret != SQLITE_OK) {
            LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
            goto out;
        }

        ret = sqlite3_bind_text(pStmt, 9, iter->module_path.c_str(), -1, SQLITE_TRANSIENT);
        if (ret != SQLITE_OK) {
            LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
            goto out;
        }

        ret = sqlite3_bind_int(pStmt, 10, iter->mode);
        if (ret != SQLITE_OK) {
            LOGE("sqlite3_bind_int: %s", sqlite3_errmsg(databaseInfo.pHandle));
            goto out;
        }

        ret = sqlite3_bind_int(pStmt, 11, (int)iter->options);
        if (ret != SQLITE_OK) {
            LOGE("sqlite3_bind_int: %s", sqlite3_errmsg(databaseInfo.pHandle));
            goto out;
        }

        ret = sqlite3_step(pStmt);
        if (ret != SQLITE_DONE) {
            ISF_SAVE_LOG("sqlite3_step returned %d, %s", ret, sqlite3_errmsg(databaseInfo.pHandle));
            LOGE("sqlite3_step returned %d, %s", ret, sqlite3_errmsg(databaseInfo.pHandle));
            ret = SQLITE_ERROR;
            goto out;
        }
        else {
            SECURE_LOGD("Insert \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" %d %u",
                iter->appid.c_str(), iter->label.c_str(), iter->languages.c_str(), iter->iconpath.c_str(), iter->pkgid.c_str(),
                iter->pkgtype.c_str(), iter->module_name.c_str(), iter->exec.c_str(), iter->module_path.c_str(), iter->mode, iter->options);
            ret = SQLITE_OK;
            i++;
        }
        sqlite3_reset(pStmt);
        sqlite3_clear_bindings(pStmt);
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
 * @brief Delete ime_info data with pkgid.
 *
 * @param pkgid pkgid to delete ime_info table.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
static int _db_delete_ime_info_by_pkgid(const char *pkgid)
{
    int i = 0, ret = 0;
    sqlite3_stmt* pStmt = NULL;
    static const char* pQuery = "DELETE FROM ime_info WHERE pkgid = ?"; // There might be more than one appid for one pkgid.

    ret = sqlite3_prepare_v2(databaseInfo.pHandle, pQuery, -1, &pStmt, NULL);
    if (ret != SQLITE_OK) {
        LOGE("%s", sqlite3_errmsg(databaseInfo.pHandle));
        return 0;
    }

    ret = sqlite3_bind_text(pStmt, 1, pkgid, -1, SQLITE_TRANSIENT);
    if (ret != SQLITE_OK) {
        LOGE("sqlite3_bind_text: %s", sqlite3_errmsg(databaseInfo.pHandle));
        goto out;
    }

    ret = sqlite3_step(pStmt);
    if (ret != SQLITE_DONE) {
        LOGE("sqlite3_step returned %d, %s", ret, sqlite3_errmsg(databaseInfo.pHandle));
    }
    else {
        // If there is no pkgid to delete, ret is still SQLITE_DONE.
        SECURE_LOGD("DELETE FROM ime_info WHERE pkgid = %s", pkgid);
        i = 1;
    }

out:
    sqlite3_reset(pStmt);
    sqlite3_clear_bindings(pStmt);
    sqlite3_finalize(pStmt);
    return i;
}

/**
 * @brief Read data from ime category manifest and insert initial db
 *
 * @param handle    pkgmgrinfo_appinfo_h pointer
 * @param user_data The data to pass to this callback.
 *
 * @return 0 if success, negative value(<0) if fail. Callback is not called if return value is negative.
 */
static int _filtered_app_list_cb (const pkgmgrinfo_appinfo_h handle, void *user_data)
{
    int ret = 0;
    char *appid = NULL, *icon = NULL, *pkgid = NULL, *pkgtype = NULL, *exec = NULL, *label = NULL, *path = NULL;
    pkgmgrinfo_pkginfo_h  pkginfo_handle = NULL;
    ImeInfoDB ime_db;
    std::vector<ImeInfoDB> ime_info;
    bool *result = static_cast<bool*>(user_data);

    if (result) /* in this case, need to check category */ {
        bool exist = true;
        ret = pkgmgrinfo_appinfo_is_category_exist(handle, "http://tizen.org/category/ime", &exist);
        if (ret != PMINFO_R_OK || !exist) {
            LOGD("ime category is not available!");
            return 0;
        }
    }

    /* appid */
    ret = pkgmgrinfo_appinfo_get_appid(handle, &appid);
    if (ret == PMINFO_R_OK)
        ime_db.appid = String(appid);
    else {
        LOGE("appid is not available!");
        return 0;
    }

    /* iconpath */
    ret = pkgmgrinfo_appinfo_get_icon( handle, &icon);
    if (ret == PMINFO_R_OK)
        ime_db.iconpath = String(icon);

    /* pkgid */
    ret = pkgmgrinfo_appinfo_get_pkgid(handle, &pkgid);
    if (ret == PMINFO_R_OK)
        ime_db.pkgid = String(pkgid);
    else {
        LOGE("pkgid is not available!");
        return 0;
    }


    /* exec path */
    ret = pkgmgrinfo_appinfo_get_exec(handle, &exec);
    if (ret == PMINFO_R_OK)
        ime_db.exec = String(exec);
    else {
        LOGE("exec is not available!");
        return 0;
    }

    /* label */
    ret = pkgmgrinfo_appinfo_get_label(handle, &label);
    if (ret == PMINFO_R_OK)
        ime_db.label = String(label);

    /* get pkgmgrinfo_pkginfo_h */
    ret = pkgmgrinfo_pkginfo_get_pkginfo(pkgid, &pkginfo_handle);
    if (ret == PMINFO_R_OK && pkginfo_handle) {
        /* pkgrootpath */
        ret = pkgmgrinfo_pkginfo_get_root_path(pkginfo_handle, &path);

        /* pkgtype */
        ret = pkgmgrinfo_pkginfo_get_type(pkginfo_handle, &pkgtype);

        pkgmgrinfo_pkginfo_destroy_pkginfo(pkginfo_handle);

        if (ret == PMINFO_R_OK)
            ime_db.pkgtype = String(pkgtype);
        else {
            LOGE("pkgtype is not available!");
            return 0;
        }
    }

    ime_db.languages = String("en");

    if (ime_db.pkgtype.compare("rpm") == 0 &&   //1 Inhouse IMEngine ISE(IME)
        //(ime_db.pkgid.find("ise-engine") != String::npos || ime_db.appid.find("ise-engine") != String::npos)) // FIXME: this checking code is a temporary one, but...
        ime_db.exec.find("scim-launcher") != String::npos)  // Some IMEngine's pkgid doesn't have "ise-engine" prefix.
    {
        ime_db.mode = TOOLBAR_KEYBOARD_MODE;
        ime_db.options = 0;
        ime_db.module_path = String(SCIM_MODULE_PATH) + String(SCIM_PATH_DELIM_STRING) + String(SCIM_BINARY_VERSION) + String(SCIM_PATH_DELIM_STRING) + String("IMEngine");
        ime_db.module_name = ime_db.pkgid;
    }
    else {
        ime_db.mode = TOOLBAR_HELPER_MODE;
        if (ime_db.pkgtype.compare("rpm") == 0) //1 Inhouse Helper ISE(IME)
        {
            ime_db.options = SCIM_HELPER_STAND_ALONE | SCIM_HELPER_NEED_SCREEN_INFO | SCIM_HELPER_AUTO_RESTART;
            ime_db.module_path = String(SCIM_MODULE_PATH) + String(SCIM_PATH_DELIM_STRING) + String(SCIM_BINARY_VERSION) + String(SCIM_PATH_DELIM_STRING) + String("Helper");
            ime_db.module_name = ime_db.pkgid;
        }
        else if (ime_db.pkgtype.compare("wgt") == 0)    //1 Download Web IME
        {
            ime_db.options = SCIM_HELPER_STAND_ALONE | SCIM_HELPER_NEED_SCREEN_INFO | SCIM_HELPER_AUTO_RESTART | SCIM_HELPER_NEED_SPOT_LOCATION_INFO | ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT;
            ime_db.module_path = String(SCIM_MODULE_PATH) + String(SCIM_PATH_DELIM_STRING) + String(SCIM_BINARY_VERSION) + String(SCIM_PATH_DELIM_STRING) + String("Helper");
            ime_db.module_name = String("ise-web-helper-agent");
        }
        else if (ime_db.pkgtype.compare("coretpk") == 0)    //1 Download Native IME
        {
            ime_db.options = SCIM_HELPER_STAND_ALONE | SCIM_HELPER_NEED_SCREEN_INFO | SCIM_HELPER_AUTO_RESTART | ISM_HELPER_PROCESS_KEYBOARD_KEYEVENT;
            if (path)
                ime_db.module_path = String(path);
            else
                ime_db.module_path = String("/opt/usr/apps/") + ime_db.pkgid + String("/lib");
            ime_db.module_name = ime_db.appid;
        }
        else {
            LOGE("Unsupported pkgtype(%s)", ime_db.pkgtype.c_str());
            return 0;
        }
    }

    ime_info.push_back(ime_db);
    ret = _db_insert_ime_info(ime_info);

    if (result && ret)
        *result = true;

    return 0;
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
 * @brief Select ime_info table with appid.
 *
 * @param appid Application ID to get ime_info data
 * @param pImeInfo The pointer of ImeInfoDB.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
EAPI int isf_db_select_ime_info_by_appid(const char *appid, ImeInfoDB *pImeInfo)
{
    int ret = 0;

    if (!appid || !pImeInfo) {
        LOGW("invalid parameter.");
        return 0;
    }

    if (_db_connect() == 0) {
        ret = _db_select_ime_info_by_appid(appid, pImeInfo);
        _db_disconnect();
    }
    return ret;
}

/**
 * @brief Select ime_info mname data.
 *
 * @param mode  ISE type
 * @param mname The list to store module name
 *
 * @return the number of selected row.
 */
EAPI int isf_db_select_module_name_by_mode(TOOLBAR_MODE_T mode, std::vector<String> &mname)
{
    int ret = 0;

    mname.clear();

    if (_db_connect() == 0) {
        ret = _db_select_module_name_by_mode(mode, mname);
        _db_disconnect();
    }

    return ret;
}

/**
 * @brief Select ime_info mpath data.
 *
 * @param mode  ISE type
 * @param mpath The list to store module path
 *
 * @return the number of selected row.
 */
EAPI int isf_db_select_module_path_by_mode(TOOLBAR_MODE_T mode, std::vector<String> &mpath)
{
    int ret = 0;

    mpath.clear();

    if (_db_connect() == 0) {
        ret = _db_select_module_path_by_mode(mode, mpath);
        _db_disconnect();
    }

    return ret;
}

/**
 * @brief Select appids by pkgid in ime_info table.
 *
 * @param pkgid pkgid to get appid data.
 * @param appids    The list to store appid; there might be multiple appids for one pkgid.
 *
 * @return the number of selected row.
 */
EAPI int isf_db_select_appids_by_pkgid(const char *pkgid, std::vector<String> &appids)
{
    int ret = 0;

    if (!pkgid) {
        LOGW("pkgid is null.");
        return 0;
    }

    if (_db_connect() == 0) {
        ret = _db_select_appids_by_pkgid(pkgid, appids);
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
EAPI int isf_db_update_label_by_appid(const char *appid, const char *label)
{
    int ret = 0;
    if (_db_connect() == 0) {
        ret = _db_update_label_by_appid(appid, label);
        _db_disconnect();
    }
    return ret;
}

/**
 * @brief Insert ime_info data with pkgid.
 *
 * @param pkgid pkgid to insert/update ime_info table.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
EAPI int isf_db_insert_ime_info_by_pkgid(const char *pkgid)
{
    int ret = 0;
    pkgmgrinfo_pkginfo_h handle = NULL;
    bool isImePkg = false;

    if (!pkgid) {
        LOGW("pkgid is null.");
        return 0;
    }

    ret = pkgmgrinfo_pkginfo_get_pkginfo(pkgid, &handle);
    if (ret != PMINFO_R_OK) {
        LOGW("pkgmgrinfo_pkginfo_get_pkginfo(\"%s\",~) returned %d", pkgid, ret);
        return 0;
    }

    if (_db_connect() == 0) {
        ret = pkgmgrinfo_appinfo_get_list(handle, PMINFO_UI_APP, _filtered_app_list_cb, (void *)&isImePkg);
        if (ret != PMINFO_R_OK) {
            LOGW("pkgmgrinfo_appinfo_get_list failed(%d)", ret);
            ret = 0;
        }
        else if (isImePkg)
            ret = 1;

        _db_disconnect();
    }

    pkgmgrinfo_pkginfo_destroy_pkginfo(handle);

    return ret;
}

/**
 * @brief Delete ime_info data with pkgid.
 *
 * @param pkgid pkgid to delete ime_info table.
 *
 * @return 1 if it is successful, otherwise return 0.
 */
EAPI int isf_db_delete_ime_info_by_pkgid(const char *pkgid)
{
    int ret = 0;

    if (!pkgid) {
        LOGW("pkgid is null.");
        return 0;
    }

    if (_db_connect() == 0) {
        ret = _db_delete_ime_info_by_pkgid(pkgid);
        _db_disconnect();
    }

    return ret;
}

/*
vi:ts=4:nowrap:ai:expandtab
*/

