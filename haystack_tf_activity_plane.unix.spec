# -*- mode: python -*-
a = Analysis(['haystack_modules/haystack_tf_activity_plane.py'],
             pathex=None,
             hiddenimports=[],
             hookspath=None,
             runtime_hooks=None,
	     excludes=['IPython','jinja2','PyQt4']	)
pyz = PYZ(a.pure)
exe = EXE(pyz,
          a.scripts,
          a.binaries,
          a.zipfiles,
          a.datas,
          name='haystack_tf_activity_plane',
          debug=False,
          strip=None,
          upx=True,
          console=True )
