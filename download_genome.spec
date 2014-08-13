# -*- mode: python -*-
a = Analysis(['haystack_modules/download_genome.py'],
             pathex=None,
             hiddenimports=['bx.seq.twobit','bx','shutil'],
             hookspath=None,
             runtime_hooks=None)

pyz = PYZ(a.pure)
exe = EXE(pyz,
          a.scripts,
          a.binaries,
          a.zipfiles,
          a.datas,
          name='download_genome',
          debug=False,
          strip=None,
          upx=True,
          console=True )
