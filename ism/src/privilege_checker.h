#ifndef __PRIVILEGE_CHECKER_H
#define __PRIVILEGE_CHECKER_H

#include <cynara-client.h>
#include <cynara-error.h>
#include <cynara-creds-socket.h>
#include <cynara-session.h>
#include <string.h>
#define IME_PRIVILEGE "http://tizen.org/privilege/imemanager"

class PrivilegeChecker {
public:
    PrivilegeChecker (int sockfd);
    ~PrivilegeChecker ();

    inline bool imePrivilege ()
    {
        return checkPrivilege (m_imePrivilege, IME_PRIVILEGE);
    }

private:
    enum PrivilegeStatus {
        PRIVILEGE_NOT_CHECKED,
        PRIVILEGE_ALLOWED,
        PRIVILEGE_DENIED,
        PRIVILEGE_NOT_RESOLVED
    };

    PrivilegeStatus m_imePrivilege;

    bool m_creds_initialised;
    char *m_client;
    char *m_session;
    char *m_user;
    int m_sockfd;

    bool initialiseCreditionals ();
    bool checkPrivilege (PrivilegeStatus &status, const char *privilege);
};

bool cynara_initialise ();
void cynara_finalise ();
void cynara_log (const char *string, int cynara_status);
#endif //__PRIVILEGE_CHECKER_H
