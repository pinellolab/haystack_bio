# -*- mode: python -*-
def extra_datas(mydir):
    def rec_glob(p, files):
        import os
        import glob
        for d in glob.glob(p):
            if os.path.isfile(d) and 'py' not in d:
                files.append(d)
            rec_glob("%s/*" % d, files)
    files = []
    rec_glob("%s/*" % mydir, files)
    extra_datas = []
    for f in files:
        extra_datas.append((f, f, 'DATA'))

    return extra_datas
a = Analysis(['haystack_modules/haystack_hotspots.py'],
             pathex=['/Users/luca/Desktop/HAYSTACK/PACKAGE_SETUP_PYINSTALLER'],
             hiddenimports=['scipy.special._ufuncs_cxx'],
             hookspath=None,
             runtime_hooks=None,
             excludes=['IPython','jinja2','PyQt4']	)

a.datas += extra_datas('haystack_modules')             
             
pyz = PYZ(a.pure)
exe = EXE(pyz,
          a.scripts,
          exclude_binaries=True,
          name='haystack_hotspots',
          debug=False,
          strip=None,
          upx=True,
          console=True )
coll = COLLECT(exe,
               a.binaries,
               a.zipfiles,
               a.datas,
               strip=None,
               upx=True,
               name='haystack_hotspots_core')
