# New ports collection makefile for:    ourmon
# Date created: 01 May 2004
# Whom:         Charlie Schluting <manos@cs.pdx.edu>
#
# $FreeBSD$

PORTNAME=       ourmon
PORTVERSION=    2.4
CATEGORIES=     net-mgmt
MASTER_SITES=   http://ourmon.cat.pdx.edu/ourmon/
DISTNAME=       ourmon24

MAINTAINER=     manos@cs.pdx.edu
COMMENT=        A libpcap-based network monitoring and anomaly detection system

BUILD_DEPENDS=  ${LOCALBASE}/lib/libpcap.a:${PORTSDIR}/net/libpcap
LIB_DEPENDS=    pcap.3:${PORTSDIR}/net/libpcap
LIB_DEPENDS+=   gd:$(PORTSDIR)/graphics/gd
RUN_DEPENDS=    rrdtool:$(PORTSDIR)/net/rrdtool
RUN_DEPENDS+=   wget:$(PORTSDIR)/ftp/wget

PLIST_DIRS=

IS_INTERACTIVE= yes
USE_PERL5=
USE_APACHE=
WRKSRC=         ${WRKDIR}/mrourmon
NO_INSTALL_MANPAGES=
NO_PACKAGE=

pre-build:
	${MKDIR} ${PREFIX}/mrourmon
do-build:
	cd ${WRKSRC} && ${PERL5} configure.pl FREEBSD_PORT_INSTALL ${PREFIX}

#pre-install:

do-install:
	${CP} -P ${WRKSRC}/README ${PREFIX}/mrourmon/
	${CP} -P ${WRKSRC}/INSTALL ${PREFIX}/mrourmon/
	${CP} -P ${WRKSRC}/CHANGES ${PREFIX}/mrourmon/
	${CP} -P ${WRKSRC}/TODO ${PREFIX}/mrourmon/
	${CP} -P ${WRKSRC}/VERSION ${PREFIX}/mrourmon/
.for DIR in bin etc rrddata scripts src tmp
	${CP} -rp ${WRKSRC}/${DIR} ${PREFIX}/mrourmon/
.endfor
.if !exists(${PREFIX}/mrourmon/logs)
	${MV} ${WRKSRC}/logs/ ${PREFIX}/mrourmon/logs/
.endif
.include <bsd.port.mk>
