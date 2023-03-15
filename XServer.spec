Summary       : The FreeMiNT X11 server for GEM
Name          : XServer
Version       : 0.14
Release       : 1
License       : GPL
Group         : User Interface/X

Packager      : Ralph Lowinski <AltF4@freemint.de>
Vendor        : Sparemint
URL           : http://X11.freemint.de/

Requires      : /bin/sh freemint-net

BuildRequires : bash gcc make mintbin binutils
BuildRequires : sh-utils fileutils

Prefix        : %{_prefix}
Docdir        : %{_prefix}/doc
BuildRoot     : %{_tmppath}/%{name}-root

Source: %{name}-%{version}.tar.gz
Patch0: XServer-0.12-make.patch


%description
This is the FreeMiNT X11 server for GEM. It runs as GEM application
and accepts X11 client requests from the local or a remote machines.

This is still a beta version. The XServer is under development and
not completly finished (especially the often requested Netscape
don't run properly yet).

Please read the README, especially the font handling possibly needs
some adjustment.


%prep
%setup -q
%patch0 -p1 -b .make


%build
cd src
make


%install
[ "${RPM_BUILD_ROOT}" != "/" ] && rm -rf ${RPM_BUILD_ROOT}

mkdir -p ${RPM_BUILD_ROOT}%{_prefix}/X11R6/bin
mkdir -p ${RPM_BUILD_ROOT}/etc/X11
ln    -s X11R6 ${RPM_BUILD_ROOT}%{_prefix}/X11

install -m 755 src/X.app         ${RPM_BUILD_ROOT}%{_prefix}/X11R6/bin/
install -m 644 src/Xapp.rsc      ${RPM_BUILD_ROOT}%{_prefix}/X11R6/bin/
ln      -fs    X.app             ${RPM_BUILD_ROOT}%{_prefix}/X11R6/bin/XServer

install -m 644 Xmodmap.EXMPL     ${RPM_BUILD_ROOT}/etc/X11/Xmodmap
install -m 644 fonts.alias.EXMPL ${RPM_BUILD_ROOT}/etc/X11/fonts.alias

# strip down anything
find ${RPM_BUILD_ROOT} -perm 755 -type f | xargs strip ||:


%clean
[ "${RPM_BUILD_ROOT}" != "/" ] && rm -rf ${RPM_BUILD_ROOT}

%pre
mkdir -p %{_prefix}/X11R6/bin 2>/dev/null ||:
mkdir -p /etc/X11 2>/dev/null ||:
mkdir -p /var/lib/Xapp 2>/dev/null ||:


%files
%defattr(-,root,root)
%doc ChangeLog* README *.EXMPL
%{_prefix}/X11R6/bin/*
%config(noreplace) /etc/X11/*


%changelog
* Fri Sep 21 2001  Ralph Lowinski <AltF4@freemint.de
- if the server can't load its RSC file the normal way, it also searches in
  /usr/X11/bin/ now.
- heaviely reworked handling of output stream buffer to avoid crashes due to
  buffer overflows, especially in GetImage and ListFonts request.
- speeded up Get/PutImage request for color depth >= 16bit.
- corrected drawing of window decor (border).
- some internal code tidy ups.
