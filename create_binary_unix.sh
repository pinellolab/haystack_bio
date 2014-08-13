rm precompiled_binary/Linux/haystack*
rm precompiled_binary/Linux/download_genome
pyinstaller haystack_pipeline.spec --clean --distpath  precompiled_binary/Linux/
pyinstaller download_genome.spec --clean --distpath precompiled_binary/Linux/
pyinstaller haystack_hotspots.unix.spec --clean --distpath  precompiled_binary/Linux/
pyinstaller haystack_motifs.spec --clean --distpath  precompiled_binary/Linux/
pyinstaller haystack_tf_activity_plane.unix.spec --clean --distpath  precompiled_binary/Linux/
rm -Rf build
