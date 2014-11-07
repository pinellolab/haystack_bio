## $Id$
##
## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = no-dependencies

SUBDIRS = src scripts tests website doc etc

BUILT_SOURCES = ARCHIVE_REVISION ARCHIVE_DATE
EXTRA_DIST = ARCHIVE_REVISION ARCHIVE_DATE

# Set the archive revision and date to refer to the changeset tagged as
# the original release or the current changeset if there is no such older tag.
ARCHIVE_DATE:
	hg log -r "min(present(meme_$(VERSION)_0) or .)" --template "{date|date}\n" > ARCHIVE_DATE

ARCHIVE_REVISION:
	hg log -r "min(present(meme_$(VERSION)_0) or .)" --template "{node}\n" > ARCHIVE_REVISION

install-data-hook: dbdir
if WEBSERVICE
	@echo "*******************************************************************************"
	@echo "*********** Remember to run 'make deploy' to deploy the webservice. ***********"
	@echo "*******************************************************************************"
endif

dbdir:
	mkdir -p $(MEME_DB)

install-data-local:
	mkdir -p $(MEME_LOGS)
	chmod a+w $(MEME_LOGS)

distdir = $(PACKAGE)_$(VERSION)

distclean-local: clean-local
clean-local: clean-ac
clean-ac:
	rm -rf autom4te.cache

test:
	cd scripts; $(MAKE) check

# Do not include .hg directories in the distribution
# Update the archive revision and date to refer to the changeset tagged as
# the original release. Hopefully they will already be set correctly.
dist-hook:
	rm -rf `find $(distdir) -name .hg`
	chmod u+w $(distdir)/ARCHIVE_REVISION $(distdir)/ARCHIVE_DATE
	hg log -r "min(present(meme_$(VERSION)_0) or .)" --template "{date|date}\n" > $(distdir)/ARCHIVE_DATE
	hg log -r "min(present(meme_$(VERSION)_0) or .)" --template "{node}\n" > $(distdir)/ARCHIVE_REVISION


deploy:
if WEBSERVICE
	if test -f $(OPAL_XML); then \
	  $(ANT) -f $(OPAL_XML) deploy -DserviceName=CENTRIMO_${VERSION} -DappConfig=${sysconfdir}/centrimo_config.xml; \
	  $(ANT) -f $(OPAL_XML) deploy -DserviceName=DREME_${VERSION} -DappConfig=${sysconfdir}/dreme_config.xml; \
	  $(ANT) -f $(OPAL_XML) deploy -DserviceName=FIMO_${VERSION} -DappConfig=${sysconfdir}/fimo_config.xml; \
	  $(ANT) -f $(OPAL_XML) deploy -DserviceName=GOMO_${VERSION} -DappConfig=${sysconfdir}/gomo_config.xml; \
	  $(ANT) -f $(OPAL_XML) deploy -DserviceName=MAST_${VERSION} -DappConfig=${sysconfdir}/mast_config.xml; \
	  $(ANT) -f $(OPAL_XML) deploy -DserviceName=MCAST_${VERSION} -DappConfig=${sysconfdir}/mcast_config.xml; \
	  $(ANT) -f $(OPAL_XML) deploy -DserviceName=MEME_${VERSION} -DappConfig=${sysconfdir}/meme_config.xml; \
	  $(ANT) -f $(OPAL_XML) deploy -DserviceName=MEMECHIP_${VERSION} -DappConfig=${sysconfdir}/meme-chip_config.xml; \
	  $(ANT) -f $(OPAL_XML) deploy -DserviceName=GLAM2_${VERSION} -DappConfig=${sysconfdir}/glam2_config.xml; \
	  $(ANT) -f $(OPAL_XML) deploy -DserviceName=GLAM2SCAN_${VERSION} -DappConfig=${sysconfdir}/glam2scan_config.xml; \
	  $(ANT) -f $(OPAL_XML) deploy -DserviceName=TOMTOM_${VERSION} -DappConfig=${sysconfdir}/tomtom_config.xml; \
	  $(ANT) -f $(OPAL_XML) deploy -DserviceName=SPAMO_${VERSION} -DappConfig=${sysconfdir}/spamo_config.xml; \
	else \
	  mkdir -vp $(OPAL_XML); \
	  cp -v ${sysconfdir}/centrimo_config.xml $(OPAL_XML)/CENTRIMO_$(VERSION).xml; \
	  cp -v ${sysconfdir}/dreme_config.xml $(OPAL_XML)/DREME_$(VERSION).xml; \
	  cp -v ${sysconfdir}/fimo_config.xml $(OPAL_XML)/FIMO_$(VERSION).xml; \
	  cp -v ${sysconfdir}/gomo_config.xml $(OPAL_XML)/GOMO_$(VERSION).xml; \
	  cp -v ${sysconfdir}/mast_config.xml $(OPAL_XML)/MAST_$(VERSION).xml; \
	  cp -v ${sysconfdir}/mcast_config.xml $(OPAL_XML)/MCAST_$(VERSION).xml; \
	  cp -v ${sysconfdir}/meme_config.xml $(OPAL_XML)/MEME_$(VERSION).xml; \
	  cp -v ${sysconfdir}/meme-chip_config.xml $(OPAL_XML)/MEMECHIP_$(VERSION).xml; \
	  cp -v ${sysconfdir}/glam2_config.xml $(OPAL_XML)/GLAM2_$(VERSION).xml; \
	  cp -v ${sysconfdir}/glam2scan_config.xml $(OPAL_XML)/GLAM2SCAN_$(VERSION).xml; \
	  cp -v ${sysconfdir}/tomtom_config.xml $(OPAL_XML)/TOMTOM_$(VERSION).xml; \
	  cp -v ${sysconfdir}/spamo_config.xml $(OPAL_XML)/SPAMO_$(VERSION).xml; \
	fi
else
	@echo "This is not a webservice installation, the 'make deploy' step is not required."
endif

undeploy:
if WEBSERVICE
	if test -f $(OPAL_XML); then \
	  $(ANT) -f $(OPAL_XML) undeploy -DserviceName=CENTRIMO_${VERSION} -DappConfig=${sysconfdir}/centrimo_config.xml; \
	  $(ANT) -f $(OPAL_XML) undeploy -DserviceName=DREME_${VERSION} -DappConfig=${sysconfdir}/dreme_config.xml; \
	  $(ANT) -f $(OPAL_XML) undeploy -DserviceName=FIMO_${VERSION} -DappConfig=${sysconfdir}/fimo_config.xml; \
	  $(ANT) -f $(OPAL_XML) undeploy -DserviceName=GOMO_${VERSION} -DappConfig=${sysconfdir}/gomo_config.xml; \
	  $(ANT) -f $(OPAL_XML) undeploy -DserviceName=MAST_${VERSION} -DappConfig=${sysconfdir}/mast_config.xml; \
	  $(ANT) -f $(OPAL_XML) undeploy -DserviceName=MCAST_${VERSION} -DappConfig=${sysconfdir}/mcast_config.xml; \
	  $(ANT) -f $(OPAL_XML) undeploy -DserviceName=MEME_${VERSION} -DappConfig=${sysconfdir}/meme_config.xml; \
	  $(ANT) -f $(OPAL_XML) undeploy -DserviceName=MEMECHIP_${VERSION} -DappConfig=${sysconfdir}/meme-chip_config.xml; \
	  $(ANT) -f $(OPAL_XML) undeploy -DserviceName=GLAM2_${VERSION} -DappConfig=${sysconfdir}/glam2_config.xml; \
	  $(ANT) -f $(OPAL_XML) undeploy -DserviceName=GLAM2SCAN_${VERSION} -DappConfig=${sysconfdir}/glam2scan_config.xml; \
	  $(ANT) -f $(OPAL_XML) undeploy -DserviceName=TOMTOM_${VERSION} -DappConfig=${sysconfdir}/tomtom_config.xml; \
	  $(ANT) -f $(OPAL_XML) undeploy -DserviceName=SPAMO_${VERSION} -DappConfig=${sysconfdir}/spamo_config.xml; \
	elif test -d $(OPAL_XML); then \
	  rm -vf $(OPAL_XML)/CENTRIMO_$(VERSION).xml; \
	  rm -vf $(OPAL_XML)/DREME_$(VERSION).xml; \
	  rm -vf $(OPAL_XML)/FIMO_$(VERSION).xml; \
	  rm -vf $(OPAL_XML)/GOMO_$(VERSION).xml; \
	  rm -vf $(OPAL_XML)/MAST_$(VERSION).xml; \
	  rm -vf $(OPAL_XML)/MCAST_$(VERSION).xml; \
	  rm -vf $(OPAL_XML)/MEME_$(VERSION).xml; \
	  rm -vf $(OPAL_XML)/MEMECHIP_$(VERSION).xml; \
	  rm -vf $(OPAL_XML)/GLAM2_$(VERSION).xml; \
	  rm -vf $(OPAL_XML)/GLAM2SCAN_$(VERSION).xml; \
	  rm -vf $(OPAL_XML)/TOMTOM_$(VERSION).xml; \
	  rm -vf $(OPAL_XML)/SPAMO_$(VERSION).xml; \
	fi
else
	@echo "This is not a webservice installation, the 'make undeploy' step is not required."
endif
