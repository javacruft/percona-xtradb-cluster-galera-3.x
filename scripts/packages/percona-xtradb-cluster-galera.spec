# Copyright (c) 2011,  Percona Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 3 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING. If not, write to the
# Free Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston
# MA  02110-1301  USA.

%define src_dir percona-xtradb-cluster-galera-3
%define docs /usr/share/doc/%{src_dir}

%define src_dir2 percona-xtradb-cluster-garbd-3
%define docs2 /usr/share/doc/%{src_dir2}
Prefix: %{_prefix}

%define rhelver %(rpm -qf --qf '%%{version}\\n' /etc/redhat-release | sed -e 's/^\\([0-9]*\\).*/\\1/g')
%if "%rhelver" == "5"
 %define boost_req boost141-devel
 %define gcc_req gcc44-c++
%else
 %define boost_req boost-devel
 %define gcc_req gcc-c++
%endif

%if %{undefined scons_args}
 %define scons_args %{nil}
%endif

%if %{undefined galera_version}
 %define galera_version 3.6
%endif

%if %{undefined galera_revision}
 %define galera_revision %{revision}
%endif

%if %{undefined pxcg_revision}
 %define pxcg_revision %{revno}
%endif

%ifarch i686
 %define scons_arch arch=i686
%else
 %define scons_arch %{nil}
%endif

%define redhatversion %(lsb_release -rs | awk -F. '{ print $1}')
%define distribution  rhel%{redhatversion}

Name:		Percona-XtraDB-Cluster-galera-3
Version:	%{galera_version}
Release:	1.%{pxcg_revision}.%{?distribution}
Summary:	Galera libraries of Percona XtraDB Cluster
Group:		Applications/Databases
License:	GPLv3
URL:		http://www.percona.com/
Source0:        percona-xtradb-cluster-galera-3.tar.gz
BuildRoot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
Provides: Percona-XtraDB-Cluster-galera-25 galera3
Obsoletes: Percona-XtraDB-Cluster-galera-56 
Conflicts: Percona-XtraDB-Cluster-galera-2
BuildRequires:	scons check-devel glibc-devel %{gcc_req} openssl-devel %{boost_req} check-devel

%description
This package contains the Galera library required by Percona XtraDB Cluster.

%package -n Percona-XtraDB-Cluster-garbd-3
Summary:	Garbd component of Percona XtraDB Cluster
Group:		Applications/Databases
Provides:       garbd3
Requires:       nc chkconfig

%description -n Percona-XtraDB-Cluster-garbd-3
This package contains the garb binary and init scripts.

%prep
%setup -q -n %{src_dir}

%build
%if "%rhelver" == "5"
export CC=gcc44
export CXX=g++44
%endif
scons %{?_smp_mflags}  revno=%{galera_revision} version=%{galera_version} boost_pool=0 garb/garbd libgalera_smm.so %{scons_arch} %{scons_args}

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p "$RPM_BUILD_ROOT"

install -d $RPM_BUILD_ROOT%{_sysconfdir}/{init.d,sysconfig}
install -m 644 $RPM_BUILD_DIR/%{src_dir}/garb/files/garb.cnf \
    $RPM_BUILD_ROOT%{_sysconfdir}/sysconfig/garb
install -m 755 $RPM_BUILD_DIR/%{src_dir}/garb/files/garb.sh \
    $RPM_BUILD_ROOT%{_sysconfdir}/init.d/garb

install -d "$RPM_BUILD_ROOT/%{_bindir}"
install -d "$RPM_BUILD_ROOT/%{_libdir}"
install -m 755 "$RPM_BUILD_DIR/%{src_dir}/garb/garbd" \
	"$RPM_BUILD_ROOT/%{_bindir}/"
install -d "$RPM_BUILD_ROOT/%{_libdir}/galera3"
install -m 755 "$RPM_BUILD_DIR/%{src_dir}/libgalera_smm.so" \
	"$RPM_BUILD_ROOT/%{_libdir}/galera3/"
ln -s "galera3/libgalera_smm.so" "$RPM_BUILD_ROOT/%{_libdir}/"

install -d $RPM_BUILD_ROOT%{docs}
install -m 644 $RPM_BUILD_DIR/%{src_dir}/COPYING                     \
    $RPM_BUILD_ROOT%{docs}/COPYING
install -m 644 $RPM_BUILD_DIR/%{src_dir}/scripts/packages/README     \
    $RPM_BUILD_ROOT%{docs}/README
install -m 644 $RPM_BUILD_DIR/%{src_dir}/scripts/packages/README-MySQL \
    $RPM_BUILD_ROOT%{docs}/README-MySQL

install -d $RPM_BUILD_ROOT%{docs2}
install -m 644 $RPM_BUILD_DIR/%{src_dir}/COPYING                     \
    $RPM_BUILD_ROOT%{docs2}/COPYING
install -m 644 $RPM_BUILD_DIR/%{src_dir}/scripts/packages/README     \
    $RPM_BUILD_ROOT%{docs2}/README

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
# This is a symlink
%attr(0755,root,root) %{_libdir}/libgalera_smm.so
%attr(0755,root,root) %{_libdir}/galera3/libgalera_smm.so
%attr(0755,root,root) %dir %{docs}
%doc %attr(0644,root,root) %{docs}/COPYING
%doc %attr(0644,root,root) %{docs}/README
%doc %attr(0644,root,root) %{docs}/README-MySQL

%files -n Percona-XtraDB-Cluster-garbd-3
%defattr(-,root,root,-)
%config(noreplace,missingok) %{_sysconfdir}/sysconfig/garb
%attr(0755,root,root) %{_sysconfdir}/init.d/garb
%attr(0755,root,root) %{_bindir}/garbd
%doc %attr(0644,root,root) %{docs2}/COPYING
%doc %attr(0644,root,root) %{docs2}/README

%changelog
* Thu May 15 2014 Raghavendra Prabhu <raghavendra.prabhu@percona.com>
- Split the packaging for garbd.
- Library is now installed in /usr/lib/galera2 with a symlink to /usr/lib/ for compatibility.
- Few cleanups.
