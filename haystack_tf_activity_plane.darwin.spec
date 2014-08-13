# -*- mode: python -*-
a = Analysis(['haystack_modules/haystack_tf_activity_plane.py'],
             pathex=None,
             hiddenimports=[],
             hookspath=None,
             runtime_hooks=None,
	     excludes=['IPython','jinja2']	)

pyz = PYZ(a.pure)
exe = EXE(pyz,
          a.scripts,
          exclude_binaries=True,
          name='haystack_tf_activity_plane',
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
               name='haystack_tf_activity_plane_core')
