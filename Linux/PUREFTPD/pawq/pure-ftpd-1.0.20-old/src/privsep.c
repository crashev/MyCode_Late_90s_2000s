#include <config.h>

#ifdef WITH_PRIVSEP
# include "ftpd.h"
# include "dynamic.h"
# include "ftpwho-update.h"
# include "globals.h"
# include "privsep_p.h"
# include "privsep.h"

# ifdef WITH_DMALLOC
#  include <dmalloc.h>
# endif

static int privsep_sendcmd(const int psfd, const void * const cmdarg,
                           const size_t cmdarg_len)
{
    ssize_t sent;
    
    while ((sent = send(psfd, cmdarg, cmdarg_len, 0)) == (ssize_t) -1 &&
           errno == EINTR);
    if (sent != (ssize_t) cmdarg_len) {
    return -1;
    }    
    return 0;
}

static int privsep_recvcmd(const int psfd, void * const cmdarg,
                           const size_t cmdarg_len)
{
    ssize_t received;
        
    while ((received = recv(psfd, cmdarg, cmdarg_len, 0)) == (ssize_t) -1 &&
           errno == EINTR);
    if (received != (ssize_t) cmdarg_len) {
        return -1;
    }    
    return 0;
}

int privsep_sendfd(const int psfd, const int fd)
{    
    char *buf;
    int *fdptr;
    struct cmsghdr *cmsg;    
    struct msghdr msg;
    struct iovec vec;
    const size_t sizeof_buf = CMSG_SPACE(sizeof *fdptr);
    PrivSepCmd fodder = PRIVSEPCMD_ANSWER_FD;
    ssize_t sent;
    
    if ((buf = ALLOCA(sizeof_buf)) == NULL) {
    return -1;
    }
    memset(&msg, 0, sizeof msg);
    vec.iov_base = (void *) &fodder;
    vec.iov_len = sizeof fodder;
    msg.msg_name = NULL;
    msg.msg_namelen = (socklen_t) 0;
    msg.msg_iov = &vec;
    msg.msg_iovlen = (size_t) 1U;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof_buf;
    msg.msg_flags = 0;
    if ((cmsg = CMSG_FIRSTHDR(&msg)) == NULL) {
    ALLOCA_FREE(buf);
    return -1;
    }
    cmsg->cmsg_len = CMSG_LEN(sizeof fd);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    if ((fdptr = (int *) CMSG_DATA(cmsg)) == NULL) {
    ALLOCA_FREE(buf);    
    return -1;
    }
    *fdptr = fd;
    msg.msg_controllen = cmsg->cmsg_len;
    while ((sent = sendmsg(psfd, &msg, 0)) == (ssize_t) -1 && errno == EINTR);
    ALLOCA_FREE(buf);    
    if (sent != (ssize_t) sizeof fodder) {
    return -1;
    }
    return 0;
}    

int privsep_recvfd(const int psfd)
{
    char *buf;
    int *fdptr;    
    struct cmsghdr *cmsg;
    struct msghdr msg;
    struct iovec vec;
    const size_t sizeof_buf = CMSG_SPACE(sizeof *fdptr);
    PrivSepCmd fodder;    
    ssize_t received;
    
    if ((buf = ALLOCA(sizeof_buf)) == NULL) {
    return -1;
    }
    memset(&msg, 0, sizeof msg);
    vec.iov_base = (void *) &fodder;
    vec.iov_len = sizeof fodder;
    msg.msg_name = NULL;
    msg.msg_namelen = (socklen_t) 0;
    msg.msg_iov = &vec;
    msg.msg_iovlen = (size_t) 1U;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof_buf;
    msg.msg_flags = 0;
    if ((cmsg = CMSG_FIRSTHDR(&msg)) == NULL ||
    (fdptr = (int *) CMSG_DATA(cmsg)) == NULL) {    
    ALLOCA_FREE(buf);
    return -1;    
    }    
    *fdptr = -1;
    while ((received = recvmsg(psfd, &msg, 0)) == (ssize_t) -1 && 
           errno == EINTR);
    if (received != (ssize_t) sizeof fodder ||
    fodder != PRIVSEPCMD_ANSWER_FD ||
    (cmsg = CMSG_FIRSTHDR(&msg)) == NULL ||
    (fdptr = (int *) CMSG_DATA(cmsg)) == NULL) {
    ALLOCA_FREE(buf);
    return -1;
    }
    return *fdptr;
}

# ifdef FTPWHO
static int privsep_privpart_removeftpwhoentry(const int psfd)
{
    PrivSepAnswer answer;
    
    if (scoreboardfile == NULL || unlink(scoreboardfile) != 0) {
        answer.removeftpwhoentry.cmd = PRIVSEPCMD_ANSWER_ERROR;
    } else {
        answer.removeftpwhoentry.cmd = PRIVSEPCMD_ANSWER_REMOVEFTPWHOENTRY;
    }
    return privsep_sendcmd(psfd, &answer, sizeof answer);
}

int privsep_removeftpwhoentry(void)
{
    PrivSepQuery query;
    PrivSepQuery answer;
    
    query.removeftpwhoentry.cmd = PRIVSEPCMD_REMOVEFTPWHOENTRY;
    if (privsep_sendcmd(psfd, &query, sizeof query) != 0 ||
        privsep_recvcmd(psfd, &answer, sizeof answer) != 0 ||
        answer.removeftpwhoentry.cmd != PRIVSEPCMD_ANSWER_REMOVEFTPWHOENTRY) {
        return -1;
    }
    return 0;    
}
# endif

int privsep_privpart_bindresport(const int psfd, 
                                 const PrivSepQuery * const query)
{
    static const unsigned short portlist[] = FTP_ACTIVE_SOURCE_PORTS;
    const unsigned short *portlistpnt = portlist;    
    int fd;
    int on = 1;
    int ret;
    
    if ((fd = socket(query->bindresport.protocol,
                     SOCK_STREAM, IPPROTO_TCP)) == -1) {
        goto bye;
    }
# ifdef SO_REUSEPORT
    (void) setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *) &on, sizeof on);
# else
    (void) setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof on);
# endif
    for (;;) {
        if (query->bindresport.protocol == PF_INET6) {
            STORAGE_PORT6(query->bindresport.ss) = htons(*portlistpnt);
        } else {
            STORAGE_PORT(query->bindresport.ss) = htons(*portlistpnt);
        }
        if (bind(fd, (struct sockaddr *) &query->bindresport.ss,
                 STORAGE_LEN(query->bindresport.ss)) == 0) {
            break;
        }
# ifdef USE_ONLY_FIXED_DATA_PORT
        (void) sleep(1U);
# else
        if (*portlistpnt == (unsigned short) 0U) {
            break;
        }
        portlistpnt++;
# endif        
    }
    
    bye:    
    ret = privsep_sendfd(psfd, fd);
    ret |= close(fd);
    
    return ret;
}

int privsep_bindresport(const int protocol, const struct sockaddr_storage ss)
{
    PrivSepQuery query;
    
    query.bindresport.cmd = PRIVSEPCMD_BINDRESPORT;
    query.bindresport.protocol = protocol;
    query.bindresport.ss = ss;
    if (privsep_sendcmd(psfd, &query, sizeof query) != 0) {
        return -1;
    }
    return privsep_recvfd(psfd);
}

static int privsep_privpart_waitcmd(const int psfd)
{
    PrivSepQuery query;
    
    if (privsep_recvcmd(psfd, &query, sizeof query) != 0) {
        return -1;
    }
    switch (query.cmd.cmd) {
# ifdef FTPWHO
    case PRIVSEPCMD_REMOVEFTPWHOENTRY:
        return privsep_privpart_removeftpwhoentry(psfd);
# endif
    case PRIVSEPCMD_BINDRESPORT:
        return privsep_privpart_bindresport(psfd, &query);
    default:
        return -1;
    }
    return 0;
}

static int privsep_privpart_main(void)
{
    int ret;
    
    while ((ret = privsep_privpart_waitcmd(psfd)) == 0);
    
    if (ret != 1) {
        return -1;
    }
    return 0;
}

static int privsep_privpart_closejunk(void)
{
    int ret = 0;
    
# if defined(WITH_UPLOAD_SCRIPT)    
    if (upload_pipe_fd != -1) {
        ret |= close(upload_pipe_fd);
    }
    if (upload_pipe_lock != -1) {
        ret |= close(upload_pipe_lock);
    }
# endif
# ifdef WITH_ALTLOG    
    if (altlog_fd != -1) {
        ret |= close(altlog_fd);
    }
# endif
# ifndef SAVE_DESCRIPTORS
    if (no_syslog == 0) {
        closelog();
    }
# endif
    (void) close(0);
    (void) close(1);
    
    return ret;
}

int privsep_init(void)
{
    int sv[2];
    pid_t pid;
    
    if (socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, sv) != 0) {
        return -1;
    }
    if ((pid = fork()) == (pid_t) -1) {
        (void) close(sv[0]);
        (void) close(sv[1]);
        
        return -1;
    }
    if (pid != (pid_t) 0) {
        (void) close(sv[1]);        
        psfd = sv[0];
        setprogname("pure-ftpd (PRIV)");
        (void) privsep_privpart_closejunk();
        _exit(privsep_privpart_main());
    }
    (void) close(sv[0]);
    psfd = sv[1];
    
    return 0;
}

#endif
