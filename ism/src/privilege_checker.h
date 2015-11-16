#ifndef __PRIVILEGE_CHECKER_H
#define __PRIVILEGE_CHECKER_H

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

    bool m_creds_initialized;
    char *m_client;
    char *m_session;
    char *m_user;
    int m_sockfd;

    bool initializeCreditionals ();
    bool checkPrivilege (PrivilegeStatus &status, const char *privilege);
};

bool isf_cynara_initialize ();
void isf_cynara_finish ();

#endif //__PRIVILEGE_CHECKER_H
