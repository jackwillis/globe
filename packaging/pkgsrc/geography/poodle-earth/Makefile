# $NetBSD$

DISTNAME=	poodle-earth-0.1.3
CATEGORIES=	geography x11
MASTER_SITES=	${MASTER_SITE_GITHUB:=jackwillis/}
GITHUB_PROJECT=	poodle-earth
GITHUB_TAG=	v${PKGVERSION_NOREV}

MAINTAINER=	pkgsrc-users@NetBSD.org
HOMEPAGE=	https://github.com/jackwillis/poodle-earth/
COMMENT=	Small X11 globe and atlas viewer with built-in data
# The program is GPL-3.0-or-later. The embedded geographic data is separate:
# Natural Earth (public domain), ETOPO 2022 and Wikidata (CC0), RESOLVE Ecoregions
# (CC BY 4.0); see DATA-CREDITS.txt, installed with the package.
LICENSE=	gnu-gpl-v3 AND cc0-1.0-universal AND cc-by-v4.0 AND public-domain

USE_LANGUAGES=	c++
USE_CXX_FEATURES=	c++20
USE_CMAKE=	yes

# Python 3 only runs at build time, to embed the data tables into the executable.
PYTHON_FOR_BUILD_ONLY=	tool
CMAKE_CONFIGURE_ARGS+=	-DPython3_EXECUTABLE:FILEPATH=${PYTHONBIN}
CMAKE_CONFIGURE_ARGS+=	-DGLOBE_BUILD_TESTS=OFF

# The build installs bin/poodle-earth and share/doc/poodle-earth/ (CMake GNUInstallDirs);
# the executable carries its data, so no data files are installed.

.include "../../lang/python/tool.mk"
.include "../../x11/libX11/buildlink3.mk"
.include "../../mk/bsd.pkg.mk"
