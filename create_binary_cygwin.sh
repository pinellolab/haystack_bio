rm precompiled_binary/Cygwin/haystack*
rm precompiled_binary/Cygwin/download_genome.exe
pyinstaller haystack_pipeline.spec --clean --distpath  precompiled_binary/Cygwin
mv precompiled_binary/Cygwin/haystack_pipeline precompiled_binary/Cygwin/haystack_pipeline.exe
pyinstaller download_genome.spec --clean --distpath precompiled_binary/Cygwin
mv precompiled_binary/Cygwin/download_genome precompiled_binary/Cygwin/download_genome.exe
pyinstaller haystack_hotspots.unix.spec --clean --distpath  precompiled_binary/Cygwin
mv precompiled_binary/Cygwin/haystack_hotspots precompiled_binary/Cygwin/haystack_hotspots.exe
pyinstaller haystack_motifs.spec --clean --distpath precompiled_binary/Cygwin
mv precompiled_binary/Cygwin/haystack_motifs precompiled_binary/Cygwin/haystack_motifs.exe
pyinstaller haystack_tf_activity_plane.unix.spec --clean --distpath  precompiled_binary/Cygwin
mv precompiled_binary/Cygwin/haystack_tf_activity_plane precompiled_binary/Cygwin/haystack_tf_activity_plane.exe
rm -Rf build
