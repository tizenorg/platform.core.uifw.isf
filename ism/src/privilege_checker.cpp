#include "privilege_checker.h"

#include <string.h>
#include <stdlib.h>

#include <cynara-client.h>
#include <cynara-error.h>
#include <cynara-creds-socket.h>
#include <cynara-session.h>

#include "scim.h"

namespace
{

cynara *p_cynara = NULL;

void
cynara_log (const char *string, int cynara_status) {
    const int buflen = 255;
    char buf[buflen];

    int ret = cynara_strerror(cynara_status, buf, buflen);
    if (ret != CYNARA_API_SUCCESS) {
        strncpy(buf, "cynara_strerror failed", buflen);
        buf[buflen - 1] = '\0';
    }

    SCIM_DEBUG_MAIN (cynara_status < 0 ? 1 : 3) << string << ": " << buf;
}

}

PrivilegeChecker::PrivilegeChecker (int sockfd)
{
    m_client = NULL;
    m_session = NULL;
    m_user = NULL;
    m_sockfd = sockfd;
}

PrivilegeChecker::~PrivilegeChecker ()
{
    if (m_client)
        free (m_client);

    if (m_session)
        free (m_session);

    if (m_user)
        free (m_user);
}

bool
PrivilegeChecker::initializeCreditionals ()
{
    if (m_client)
        return true;

    int ret;
    int pid;

    ret = cynara_creds_socket_get_client (m_sockfd, CLIENT_METHOD_DEFAULT, &m_client);
    cynara_log("cynara_creds_socket_get_client()", ret);
    if (ret != CYNARA_API_SUCCESS) {
        goto CLEANUP;
    }

    ret = cynara_creds_socket_get_user (m_sockfd, USER_METHOD_DEFAULT, &m_user);
    cynara_log("cynara_creds_socket_get_user()", ret);
    if (ret != CYNARA_API_SUCCESS) {
        goto CLEANUP;
    }

    ret = cynara_creds_socket_get_pid (m_sockfd, &pid);
    cynara_log("cynara_creds_socket_get_pid()", ret);
    if (ret != CYNARA_API_SUCCESS) {
        goto CLEANUP;
    }

    m_session = cynara_session_from_pid (pid);
    if (!m_session) {
        SCIM_DEBUG_MAIN(1) << "cynara_session_from_pid(): failed";
        goto CLEANUP;
    }

    return true;
CLEANUP:
    if (m_client)
        free (m_client);

    if (m_session)
        free (m_session);

    if (m_user)
        free (m_user);

    m_client = NULL;
    m_session = NULL;
    m_user = NULL;

    return false;
}


bool
PrivilegeChecker::checkPrivilege (const char *privilege)
{
    if (!initializeCreditionals ())
        return false;

    if (!p_cynara)
        return false;

    int ret = cynara_check (p_cynara, m_client, m_session, m_user, privilege);
    cynara_log("cynara_check()", ret);
    if (ret != CYNARA_API_ACCESS_ALLOWED)
        return false;
    return true;
}

bool
isf_cynara_initialize ()
{
    int ret = cynara_initialize (&p_cynara, NULL);
    cynara_log("cynara_initialize()", ret);
    return ret == CYNARA_API_SUCCESS;
}

void
isf_cynara_finish ()
{
    if (p_cynara)
        cynara_finish (p_cynara);

    p_cynara = NULL;
}
