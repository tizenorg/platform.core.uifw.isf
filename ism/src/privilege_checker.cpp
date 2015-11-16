#include "privilege_checker.h"
#include "scim.h"
#include <stdlib.h>

namespace
{

cynara *p_cynara = NULL;

}

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

PrivilegeChecker::PrivilegeChecker (int sockfd)
{
    m_imePrivilege = PRIVILEGE_NOT_CHECKED;
    m_creds_initialised = false;
    m_client = NULL;
    m_session = NULL;
    m_user = NULL;
    m_sockfd = sockfd;
}

PrivilegeChecker::~PrivilegeChecker ()
{
    free (m_client);
    free (m_session);
    free (m_user);
}

bool PrivilegeChecker::initialiseCreditionals ()
{
    if (m_creds_initialised)
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

    m_creds_initialised = true;
    return true;
CLEANUP:
    free (m_client);
    free (m_session);
    free (m_user);

    m_client = NULL;
    m_session = NULL;
    m_user = NULL;

    return false;
}


bool PrivilegeChecker::checkPrivilege (PrivilegeStatus &status, const char *privilege)
{
    int ret;

    switch (status) {

	case PRIVILEGE_NOT_CHECKED:
	    if (!initialiseCreditionals ())
		return false;

	    ret = cynara_simple_check (p_cynara, m_client, m_session, m_user, privilege);
	    cynara_log("cynara_simple_check()", ret);
	    switch (ret) {

		case CYNARA_API_ACCESS_ALLOWED:
		    status = PRIVILEGE_ALLOWED;
		    return true;

		case CYNARA_API_ACCESS_DENIED:
		    status = PRIVILEGE_DENIED;
		    return false;

		case CYNARA_API_ACCESS_NOT_RESOLVED:
		    status = PRIVILEGE_NOT_RESOLVED;
		    break;

		default:
		    return false;

	    }

	case PRIVILEGE_NOT_RESOLVED:
	    ret = cynara_check (p_cynara, m_client, m_session, m_user, privilege);
	    cynara_log("cynara_check()", ret);
	    if (ret != CYNARA_API_ACCESS_ALLOWED)
		return false;
	    return true;

	case PRIVILEGE_ALLOWED:
	    return true;

	case PRIVILEGE_DENIED:
	    return false;

    }

    return false;
}
