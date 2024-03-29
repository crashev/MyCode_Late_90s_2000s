%define name       pure-ftpd
%define version    1.0.20
%define builddir   $RPM_BUILD_DIR/%{name}-%{version}
%define no_install_post_compress_docs    1
%define con_pam    0
%define con_pgsql  0
%define con_mysql  0
%define con_ldap   0
%define con_extauth 1
%define con_pcap   0
%define con_ascii  1
%define con_sendfile 1
%define con_puredb 1
%define con_quotas 1
%define con_uploadscript 1
%define con_virtualhosts 1
%define con_virtualchroot 1
%define con_altlog 1
%define con_cookie 1
%define con_throttling 1
%define con_ratios 1
%define con_ftpwho 1
%define con_diraliases 1
%define con_peruserlimits 1
%define con_largefile 1
%define con_boring 0
%define con_privsep 1
%define con_tls 0
%define con_sysquotas 1
%define con_rendezvous 0

%define prefixdef  /usr/local
%define sysconfdef /etc
%define certfile   /etc/ssl/private/pure-ftpd.pem

%if %{con_tls}
%define release    1.tls
%else
%define release    1
%endif

#You can override all kinds of default with --define. Like this:
#rpm -ba|--rebuild --define 'with_pam 1'

#dont change these. Use --define instead.
%{?with_pam:%define con_pam 1}
%{?with_pgsql:%define con_pgsql 1}
%{?with_mysql:%define con_mysql 1}
%{?with_ldap:%define con_ldap 1}
%{?with_extauth:%define con_extauth 1}
%{?with_pcap:%define con_pcap 1}
%{?with_ascii:%define con_ascii 1}
%{?with_sendfile:%define con_sendfile 1}
%{?with_puredb:%define con_puredb 1}
%{?with_quotas:%define con_quotas 1}
%{?with_uploadscript:%define con_uploadscript 1}
%{?with_virtualhosts:%define con_virtualhosts 1}
%{?with_virtualchroot:%define con_virtualchroot 1}
%{?with_altlog:%define con_altlog 1}
%{?with_cookie:%define con_cookie 1}
%{?with_throttling:%define con_throttling 1}
%{?with_ratios:%define con_ratios 1}
%{?with_ftpwho:%define con_ftpwho 1}
%{?with_diraliases:%define con_diraliases 1}
%{?with_peruserlimits:%define con_peruserlimits 1}
%{?with_largefile:%define con_largefile 1}
%{?with_boring:%define con_boring 1}
%{?with_privsep:%define con_privsep 1}
%{?with_tls:%define con_tls 1}
%{?with_sysquotas:%define con_sysquotas 1}
%{?with_rendezvous:%define con_rendezvous 1}

#If you don't like the prefix '/usr/local' you can override it like this:
#rpm -ba|--rebuild --define 'prefix /usr'
%{?!prefix:%define prefix %{prefixdef}}

#If you don't like the sysconfdir '/etc' you can override it like this:
#rpm -ba|--rebuild --define 'sysconfdir /usr/local/etc'
%{?!sysconfdir:%define sysconfdir %{sysconfdef}}


Name:              %{name}
Version:           %{version}
Release:           %{release}
Vendor:            Generic
Packager:          Frank DENIS <j@pureftpd.org>
URL:               http://www.pureftpd.org/
Source:            ftp://ftp.pureftpd.org/pub/pure-ftpd/releases/%{name}-%{version}.tar.gz
Group:             System Environment/Daemons
License:           BSD
Provides:     	   ftp-server
BuildRoot:         %{_tmppath}/%{name}-%{version}
Summary:           Lightweight, fast and secure FTP server
Conflicts:         wu-ftpd proftpd ftpd in.ftpd anonftp publicfile wuftpd ftpd-BSD

%description
Pure-FTPd is a fast, production-quality, standard-comformant FTP server,
based upon Troll-FTPd. Unlike other popular FTP servers, it has no known
security flaw, it is really trivial to set up and it is especially designed
for modern Linux and FreeBSD kernels (setfsuid, sendfile, capabilities) .
Features include chroot()ed and/or virtual chroot()ed home directories,
virtual domains, built-in 'ls', anti-warez system, bounded ports for passive
downloads, FXP protocol, bandwidth throttling, ratios, LDAP / MySQL /
PostgreSQL-based authentication, fortune files, Apache-like log files, fast
standalone mode, text / HTML / XML real-time status report, virtual users,
virtual quotas, privilege separation and more.

%prep
%setup 	           -n %{name}-%{version} 


%build
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%{prefix} \
%if %{con_pam}
  --with-pam \
%endif  
%if %{con_pgsql}
  --with-pgsql \
%endif  
%if %{con_mysql}
  --with-mysql \
%endif
%if %{con_ldap}
  --with-ldap \
%endif  
%if %{con_extauth}
  --with-extauth \
%endif  
%if %{con_pcap}
  --with-capabilities \
%else 
  --without-capabilities \
%endif
%if %{con_ascii}
  --with-ascii \
%else 
  --without-ascii \
%endif
%if %{con_sendfile}
  --with-sendfile \
%else 
  --without-sendfile \
%endif
%if %{con_puredb}
  --with-puredb \
%endif  
%if %{con_quotas}
  --with-quotas \
%endif  
%if %{con_uploadscript}
  --with-uploadscript \
%endif  
%if %{con_virtualhosts}
  --with-virtualhosts \
%endif  
%if %{con_virtualchroot}
  --with-virtualchroot \
%endif  
%if %{con_altlog}
  --with-altlog \
%endif  
%if %{con_cookie}
  --with-cookie \
%endif  
%if %{con_throttling}
  --with-throttling \
%endif  
%if %{con_ratios}
  --with-ratios \
%endif  
%if %{con_ftpwho}
  --with-ftpwho \
%endif  
%if %{con_diraliases}
  --with-diraliases \
%endif  
%if %{con_peruserlimits}
  --with-peruserlimits \
%endif  
%if %{con_largefile}
  --with-largefile \
%endif  
%if %{con_boring}
  --with-boring \
%endif  
%if %{con_privsep}
  --with-privsep \
%endif  
%if %{con_tls}
  --with-tls \
%endif  
%if %{con_sysquotas}
  --with-sysquotas \
%endif  
%if %{con_rendezvous}
  --with-rendezvous \
%endif  
  --with-paranoidmsg \
  --mandir=%{_mandir} --sysconfdir=%{sysconfdir} \
  --with-certfile=%{certfile}
  
if [ "$SMP" != "" ]; then
  (make "MAKE=make -k -j $SMP"; exit 0)
  make
else
  make
fi

%install
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf "$RPM_BUILD_ROOT"
make DESTDIR="$RPM_BUILD_ROOT" install-strip

if [ ! -d $RPM_BUILD_ROOT%{prefix}/sbin ]; then
  mkdir -p $RPM_BUILD_ROOT%{prefix}/sbin
fi
if [ ! -d $RPM_BUILD_ROOT%{prefix}/bin ]; then
  mkdir -p $RPM_BUILD_ROOT%{prefix}/bin
fi
if [ ! -d $RPM_BUILD_ROOT%{sysconfdir} ]; then
  mkdir -p $RPM_BUILD_ROOT%{sysconfdir}
fi
if [ ! -d $RPM_BUILD_ROOT%{sysconfdir}/rc.d/init.d ]; then
  mkdir -p $RPM_BUILD_ROOT%{sysconfdir}/rc.d/init.d
fi
if [ ! -d $RPM_BUILD_ROOT%{_mandir}/man8 ]; then
  mkdir -p $RPM_BUILD_ROOT%{_mandir}/man8
fi

gzip --best $RPM_BUILD_ROOT/%{_mandir}/man8/*.8

%if ! %{con_puredb}
  rm $RPM_BUILD_ROOT%{prefix}/bin/pure-pw $RPM_BUILD_ROOT%{prefix}/bin/pure-pwconvert
  rm $RPM_BUILD_ROOT%{_mandir}/man8/pure-pw.* $RPM_BUILD_ROOT%{_mandir}/man8/pure-pwconvert.*
%endif

%if ! %{con_uploadscript}
  rm $RPM_BUILD_ROOT%{prefix}/sbin/pure-uploadscript
  rm $RPM_BUILD_ROOT%{_mandir}/man8/pure-uploadscript.*
%endif

%if ! %{con_extauth}
  rm $RPM_BUILD_ROOT%{prefix}/sbin/pure-authd
  rm $RPM_BUILD_ROOT%{_mandir}/man8/pure-authd.*
%endif

%if ! %{con_quotas}
  rm $RPM_BUILD_ROOT%{prefix}/sbin/pure-quotacheck
  rm $RPM_BUILD_ROOT%{_mandir}/man8/pure-quotacheck.*
%endif

%if ! %{con_ftpwho}
  rm $RPM_BUILD_ROOT%{prefix}/sbin/pure-ftpwho
  rm $RPM_BUILD_ROOT%{_mandir}/man8/pure-ftpwho.*
%endif

%if %{con_virtualhosts}
  mkdir $RPM_BUILD_ROOT%{sysconfdir}/pure-ftpd/
%endif

install -m 755 configuration-file/pure-config.pl $RPM_BUILD_ROOT%{prefix}/sbin/
install -m 644 configuration-file/pure-ftpd.conf $RPM_BUILD_ROOT%{sysconfdir}/

# replace some occurences of prefix and sysconfig:
sed "s|%{prefixdef}|%{prefix}|g; s|%{sysconfdef}/sysconfig|%{sysconfdir}/sysconfig|g" < contrib/redhat.init > contrib/redhat.init_replaced
install -m 755 contrib/redhat.init_replaced $RPM_BUILD_ROOT/%{sysconfdir}/rc.d/init.d/pure-ftpd
sed "s|\(\$prefix *= *['\"]\)%{prefixdef}|\1%{prefix}|g" < configuration-file/pure-config.pl > configuration-file/pure-config.pl_replaced
install -m 755 configuration-file/pure-config.pl_replaced $RPM_BUILD_ROOT%{prefix}/sbin/pure-config.pl

%if %{con_pam}
  install -d $RPM_BUILD_ROOT/%{sysconfdir}/pam.d/
  install -m 644 pam/pure-ftpd $RPM_BUILD_ROOT/%{sysconfdir}/pam.d/
%endif
%if %{con_ldap}
  install -m 600 pureftpd-ldap.conf $RPM_BUILD_ROOT%{sysconfdir}/
%endif
%if %{con_mysql}
  install -m 600 pureftpd-mysql.conf $RPM_BUILD_ROOT%{sysconfdir}/
%endif
%if %{con_pgsql}
  install -m 600 pureftpd-pgsql.conf $RPM_BUILD_ROOT%{sysconfdir}/
%endif

%clean
[ -n "$RPM_BUILD_ROOT" -a "$RPM_BUILD_ROOT" != / ] && rm -rf "$RPM_BUILD_ROOT"

%files

%defattr(0755, root, root)
%{prefix}/sbin/*
%{prefix}/bin/*
%config(noreplace) %{sysconfdir}/rc.d/init.d/pure-ftpd

%defattr(0644, root, root)
%{_mandir}/man8/*

%defattr(-, root, root)
%doc FAQ THANKS README.MacOS-X README.Authentication-Modules README.Windows README.Virtual-Users README.Debian README README.Contrib README.Configuration-File pureftpd.schema AUTHORS CONTACT HISTORY NEWS README.LDAP README.PGSQL README.MySQL README.Netfilter README.TLS pureftpd-ldap.conf pureftpd-mysql.conf pureftpd-pgsql.conf

%config(noreplace) %{sysconfdir}/*.conf
%if %{con_pam}
%config(noreplace) %{sysconfdir}/pam.d/pure-ftpd
%endif
%if %{con_virtualhosts}
%{sysconfdir}/pure-ftpd/
%endif

%changelog
* Sun Feb 29 2004 Frank DENIS <j@pureftpd.org>
- Add a knob for rendezvous.

* Wed Nov 20 2002 Frank DENIS <j@pureftpd.org>
- Remove --with-everything and add lotsa new knobs.

* Mon Aug  5 2002 Johannes Erdfelt <johannes@erdfelt.com>
- Added con_puredb, con_quotas, con_uploadscript, con_virtualhosts
- Wrap some config files around con_ options
- Simplify file lists by using globbing where possible
- Remove some files that aren't necessary when given appropriate con_ options

* Tue Jun 11 2002 Bernhard Weisshuhn <bkw@weisshuhn.de>
- Added with_pcap

* Tue Dec 26 2001 Frank DENIS <j@pureftpd.org>
- Added with_ldap, with_pgsql and with_mysql
