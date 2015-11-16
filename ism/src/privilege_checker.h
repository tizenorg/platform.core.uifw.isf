#ifndef __PRIVILEGE_CHECKER_H
#define __PRIVILEGE_CHECKER_H

#define IME_PRIVILEGE "http://tizen.org/privilege/imemanager"

class PrivilegeChecker {
public:
    PrivilegeChecker (int sockfd);
    ~PrivilegeChecker ();

    bool checkPrivilege (const char *privilege);

private:
    char *m_client;
    char *m_session;
    char *m_user;
    int m_sockfd;

    bool initializeCreditionals ();
};

bool isf_cynara_initialize ();
void isf_cynara_finish ();

#endif //__PRIVILEGE_CHECKER_H
