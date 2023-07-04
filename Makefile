# Makefile for tar

JUSTSOURCE=FICS-distrib/Makefile FICS-distrib/Makefile.common FICS-distrib/Makefile.ultrix FICS-distrib/Makefile.next FICS-distrib/Makefile.usl FICS-distrib/*.[c,h]

SOURCE=FICS/FICS-distrib/Makefile FICS/FICS-distrib/Makefile.common FICS/FICS-distrib/Makefile.ultrix FICS/FICS-distrib/Makefile.next  FICS/FICS-distrib/Makefile.usl FICS/FICS-distrib/*.[c,h] FICS/README

DFILES=FICS/GNU_LICENSE FICS/README_LEGAL FICS/Makefile FICS/TODO

DDIRS=FICS/data FICS/players FICS/games \
      FICS/config/hostinfo.client.format

all:
        @ echo You can make 'clean' 'distrib' 'source' 'help'

clean:
        find . -name \*~ -exec rm \{\} \;
        rm FICS.tar.Z.uu FICS.tar.Z FICS-source.tar.Z FICS-help.tar.Z

distrib:
        cd ..; tar cvf - ${SOURCE} ${DFILES} ${DDIRS} | compress > FICS/FICS.tar.Z; cd FICS

source:
        tar cvf - ${JUSTSOURCE} | compress > FICS-source.tar.Z

help:
        tar cvf - data/help data/boards | compress > FICS-help.tar.Z
